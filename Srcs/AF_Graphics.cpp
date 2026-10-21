#include "AF_PCH.h"
#include "AF_Graphics.h"
#include "AF_String.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace {
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 背景色

	// HRESULTが失敗した場合のログ出力処理
	bool Failed(HRESULT hr, std::string_view what) {
		if (SUCCEEDED(hr)) return false;
		AFLib::DebugLog(std::format("{}に失敗しました(HRESULT: 0x{:08X})", what, static_cast<unsigned long>(hr)));
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
	}
}