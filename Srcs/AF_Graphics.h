#ifndef AF_GRAPHICS_H
#define AF_GRAPHICS_H

namespace AFLib {
	class Graphics {
	public:
		Graphics() = default;
		~Graphics();

		Graphics(const Graphics&) = delete;
		Graphics& operator=(const Graphics&) = delete;

		// DX12の初期化
		int Init(HWND hwnd, int width, int height);

		// 画面をクリア
		void ClearScreen();

		// 画面に描画
		void DrawScreen();

		// 四角形を描画
		int DrawBox(int x1, int y1, int x2, int y2, unsigned int color);

	private:
		// GPUの処理がすべて終わるまで待機
		void WaitGPU();

		// 描画状態の切り替え
		void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

		// シェーダーとパイプラインの作成
		int CreatePipeline();

		// 頂点バッファの作成
		int CreateVertexBuffer();

	private:
		static constexpr UINT buffCount = 2;          // バックバッファ数
		static constexpr UINT maxVertices = 6 * 4096; // 1フレームに描画できる最大頂点数

		struct Vertex {                               // 頂点データ
			float x, y;                               // 座標
			float r, g, b, a;                         // 色
		};

		ComPtr<ID3D12Device> device;
		ComPtr<ID3D12CommandQueue> cmdQueue;
		ComPtr<IDXGISwapChain4> swapChain;
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12Resource> backBuffers[buffCount];
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		ComPtr<ID3D12Fence> fence;
		ComPtr<ID3D12RootSignature> rootSignature;
		ComPtr<ID3D12PipelineState> pipelineState;
		ComPtr<ID3D12Resource> vertexBuffer;

		D3D12_VERTEX_BUFFER_VIEW vbView = {};
		Vertex* vertexData = nullptr;
		UINT vertexCount = 0;

		int screenWidth = 0;
		int screenHeight = 0;
		D3D12_VIEWPORT viewport = {};
		D3D12_RECT scissor = {};

		UINT64 fenceValue = 0;
		HANDLE fenceEvent = nullptr;
		UINT rtvDescSize = 0;
		UINT backIdx = 0;
		bool isFrameStarted = false;

	};

	Graphics* GetGraphics();
}

#endif // !AF_GRAPHICS_H
