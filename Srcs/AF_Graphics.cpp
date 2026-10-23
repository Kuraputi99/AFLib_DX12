#include "AF_PCH.h"
#include "AF_Graphics.h"
#include "AF_String.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace {
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 背景色

	// ディフォルトのシェーダ
	constexpr char shaderSource[] = R"(
		struct VSInput {
			float2 pos   : POSITION;
			float4 color : COLOR;
		};
		
		struct PSInput {
			float4 pos   : SV_POSITION;
			float4 color : COLOR;
		};
		
		PSInput VSMain(VSInput input) {
			PSInput output;
			output.pos   = float4(input.pos, 0.0f, 1.0f);
			output.color = input.color;
			return output;
		};
		
		float4 PSMain(PSInput input) : SV_TARGET {
			return input.color;
		}
	)";

	// HRESULTが失敗した場合のログ出力処理
	bool Failed(HRESULT hr, std::string_view what) {
		if (SUCCEEDED(hr)) return false;
		AFLib::DebugLog(std::format("{}に失敗しました(HRESULT: 0x{:08X})", what, static_cast<unsigned long>(hr)));
		return true;
	}

	bool CompileShader(const char* entry, const char* target, ComPtr<ID3DBlob>& out) {
		UINT flags = 0;
#ifdef _DEBUG
		flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

		ComPtr<ID3DBlob> err;
		const HRESULT hr = D3DCompile(
			shaderSource,
			sizeof(shaderSource) - 1,
			nullptr, nullptr, nullptr,
			entry,target, flags, 0, &out, &err
		);
		if (FAILED(hr)) {
			AFLib::DebugLog(std::format("シェーダー({})のコンパイルに失敗しました(HRESULT: 0x{:08X})", entry, static_cast<unsigned long>(hr)));
			if (err) AFLib::DebugLog(static_cast<const char*>(err->GetBufferPointer()));
			return false;
			
		}
		
		return true;
	}
}

namespace AFLib {
	Graphics::~Graphics() {
		WaitGPU();
		if (fenceEvent) CloseHandle(fenceEvent);
	}

	// DX12の初期化
	int Graphics::Init(HWND hwnd, int width, int height) {
		UINT factoryFlag = 0;

		/*-Debugレイヤー-*/
#ifdef _DEBUG
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
			debug->EnableDebugLayer();
			factoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
		}
		else {
			DebugLog("デバッグレイヤーを有効にできませんでした");
		}
#endif // _DEBUG

		/*-ファクトリーの準備-*/
		ComPtr<IDXGIFactory6> factory;
		if (Failed(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&factory)), "DXGIファクトリーの作成")) return -1;

		/*-性能の高いGPUから優先にデバイスを作成-*/
		ComPtr<IDXGIAdapter1> adapter;
		for (UINT i = 0; factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 adapterDesc = {};
			adapter->GetDesc1(&adapterDesc);
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) break;
		}

		/*-デバイスを作成できなかった場合-*/
		if (!device) {
			DebugLog("DirectX12に対応するGPUが見つかりませんでした");
			return -1;
		}

		/*-コマンドキューの準備-*/
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		if (Failed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&cmdQueue)), "コマンドキューの作成")) return -1;

		/*-スワップチェーンの準備-*/
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = static_cast<UINT>(width);
		scDesc.Height = static_cast<UINT>(height);
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.SampleDesc.Count = 1;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = buffCount;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		ComPtr<IDXGISwapChain1> swapChain1;
		if (Failed(factory->CreateSwapChainForHwnd(cmdQueue.Get(), hwnd, &scDesc, nullptr, nullptr, &swapChain1), "スワップチェーンの作成")) return -1;
		if (Failed(swapChain1.As(&swapChain), "スワップチェーンの取得")) return -1;

		/*-RTVの準備-*/
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		heapDesc.NumDescriptors = buffCount;
		if (Failed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeap)), "RTVヒープの作成")) return -1;
		rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < buffCount; ++i) {
			if (Failed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])), "バックバッファの取得")) return -1;
			device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescSize;
		}

		/*-コマンドリスト・アロケータの準備-*/
		if (Failed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)), "コマンドアロケータの作成")) return -1;
		if (Failed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&cmdList)), "コマンドリストの作成")) return -1;
		cmdList->Close();

		/*-フェンスの準備-*/
		if (Failed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "フェンスの作成")) return -1;
		fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
		if (!fenceEvent) {
			DebugLog("フェンス用イベントの作成に失敗しました");
			return -1;
		}

		/*-画面サイズと描画範囲-*/
		screenWidth = width;
		screenHeight = height;
		viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
		scissor = { 0, 0, width, height };

		/*-図形描画の準備-*/
		if (CreatePipeline() != 0) return -1;
		if (CreateVertexBuffer() != 0) return -1;

		return 0;
	}

	// 画面をクリア
	void Graphics::ClearScreen() {
		if (!swapChain) return;

		backIdx = swapChain->GetCurrentBackBufferIndex();
		cmdAllocator->Reset();
		cmdList->Reset(cmdAllocator.Get(), nullptr);

		/*-表画面→裏画面にする-*/
		Transition(backBuffers[backIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtv.ptr += static_cast<SIZE_T>(backIdx) * rtvDescSize;

		cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
		cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

		/*-図形描画の共通設定-*/
		cmdList->SetGraphicsRootSignature(rootSignature.Get());
		cmdList->SetPipelineState(pipelineState.Get());
		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &scissor);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->IASetVertexBuffers(0, 1, &vbView);
		vertexCount = 0;

		isFrameStarted = true;
	}

	// 画面に描画
	void Graphics::DrawScreen() {
		if (!isFrameStarted) return;

		/*-裏画面→表画面にする-*/
		Transition(backBuffers[backIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cmdList->Close();

		ID3D12CommandList* lists[] = { cmdList.Get() };
		cmdQueue->ExecuteCommandLists(1, lists);

		/*-垂直同期を有効にして描画が安定-*/
		swapChain->Present(1, 0);
		WaitGPU();
		isFrameStarted = false;
	}

	// GPUの処理がすべて終わるまで待機
	void Graphics::WaitGPU() {
		if (!cmdQueue || !fence || !fenceEvent) return;

		++fenceValue;
		cmdQueue->Signal(fence.Get(), fenceValue);
		if (fence->GetCompletedValue() < fenceValue) {
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}

	// 描画状態の切り替え
	void Graphics::Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		cmdList->ResourceBarrier(1, &barrier);
	}

	// 塗りつぶした四角形を描く
	int Graphics::DrawBox(int x1, int y1, int x2, int y2, unsigned int color) {
		if (!isFrameStarted) return -1;

		if (vertexCount + 6 > maxVertices) {
			static bool logged = false;   // 毎回出すとログが埋まるので1回だけ
			if (!logged) {
				DebugLog("1フレームで描ける図形の数の上限を超えました");
				logged = true;
			}
			return -1;
		}

		/*-ピクセル座標 → 画面座標(左端 -1 〜 右端 1、上端 1 〜 下端 -1)-*/
		const float left = static_cast<float>(x1) / screenWidth * 2.0f - 1.0f;
		const float right = static_cast<float>(x2) / screenWidth * 2.0f - 1.0f;
		const float top = 1.0f - static_cast<float>(y1) / screenHeight * 2.0f;
		const float bottom = 1.0f - static_cast<float>(y2) / screenHeight * 2.0f;

		/*-色(0xRRGGBB)を 0〜1 に変換-*/
		const float r = ((color >> 16) & 0xFF) / 255.0f;
		const float g = ((color >> 8) & 0xFF) / 255.0f;
		const float b = (color & 0xFF) / 255.0f;

		/*-三角形 2枚で四角形を作る-*/
		Vertex* v = vertexData + vertexCount;
		v[0] = { left,  top,    r, g, b, 1.0f };   // 左上
		v[1] = { right, top,    r, g, b, 1.0f };   // 右上
		v[2] = { left,  bottom, r, g, b, 1.0f };   // 左下
		v[3] = { left,  bottom, r, g, b, 1.0f };   // 左下
		v[4] = { right, top,    r, g, b, 1.0f };   // 右上
		v[5] = { right, bottom, r, g, b, 1.0f };   // 右下

		cmdList->DrawInstanced(6, 1, vertexCount, 0);
		vertexCount += 6;

		return 0;
	}

	// シェーダーとパイプラインの作成
	int Graphics::CreatePipeline() {
		/*-ルートシグネチャ(シェーダーに渡すデータの一覧。今は何も渡さない)-*/
		D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> rsBlob;
		ComPtr<ID3DBlob> rsError;
		if (Failed(D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &rsError), "ルートシグネチャの作成準備")) return -1;
		if (Failed(device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)), "ルートシグネチャの作成")) return -1;

		/*-シェーダーのコンパイル-*/
		ComPtr<ID3DBlob> vs;
		ComPtr<ID3DBlob> ps;
		if (!CompileShader("VSMain", "vs_5_0", vs)) return -1;
		if (!CompileShader("PSMain", "ps_5_0", ps)) return -1;

		/*-頂点データの並び(Vertex 構造体と一致させる)-*/
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		/*-パイプラインステート(描き方の設定一式)-*/
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
		psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleMask = UINT_MAX;

		// 塗り方(2D なので表裏を気にせず両面描く)
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState.DepthClipEnable = TRUE;

		// 半透明の合成(今は不透明だけだが、画像の描画に備えて有効にしておく)
		D3D12_RENDER_TARGET_BLEND_DESC& blend = psoDesc.BlendState.RenderTarget[0];
		blend.BlendEnable = TRUE;
		blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blend.BlendOp = D3D12_BLEND_OP_ADD;
		blend.SrcBlendAlpha = D3D12_BLEND_ONE;
		blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blend.LogicOp = D3D12_LOGIC_OP_NOOP;
		blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// 奥行き(深度)は 2D なので使わない
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;

		if (Failed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)), "パイプラインステートの作成")) return -1;

		return 0;
	}

	// 頂点バッファの作成
	int Graphics::CreateVertexBuffer() {
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;   // CPU から書き込める場所に作る

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(Vertex) * maxVertices;
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		if (Failed(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer)), "頂点バッファの作成")) return -1;

		/*-書き込み先のアドレスを取得(終了まで開いたまま使う)-*/
		D3D12_RANGE readRange = { 0, 0 };   // CPU からは読まない
		if (Failed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexData)), "頂点バッファのマップ")) return -1;

		vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vbView.SizeInBytes = sizeof(Vertex) * maxVertices;
		vbView.StrideInBytes = sizeof(Vertex);

		return 0;
	}
}