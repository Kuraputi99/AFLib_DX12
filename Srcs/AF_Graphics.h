#ifndef AF_GRAPHICS_H
#define AF_GRAPHICS_H

namespace AFLib {
	class Graphics {
	public:
		Graphics() = default;
		~Graphics();

		Graphics(const Graphics&) = delete;
		Graphics& operator=(const Graphics&) = delete;

		int Init(HWND hwnd, int width, int height);

		void BeginFrame();

		void EndFrame();

	private:
		void WaitGPU();
		void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	private:
		static constexpr UINT buffCount = 2;

		ComPtr<ID3D12Device> device;
		ComPtr<ID3D12CommandQueue> cmdQueue;
		ComPtr<IDXGISwapChain4> swapChain;
		ComPtr<ID3D12DescriptorHeap> rtvHeap;
		ComPtr<ID3D12Resource> backBuffers[buffCount];
		ComPtr<ID3D12CommandAllocator> cmdAllocator;
		ComPtr<ID3D12GraphicsCommandList> cmdList;
		ComPtr<ID3D12Fence> fence;

		UINT64 fenceValue = 0;
		HANDLE fenceEvent = nullptr;
		UINT rtvDescSize = 0;
		UINT backIdx = 0;
		bool isFrameStarted = false;

	};
}

#endif // !AF_GRAPHICS_H
