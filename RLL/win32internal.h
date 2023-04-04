#pragma once
#include "win32impl.h"
#include "Interfaces.h"

#include "windows.h"
#include "shellscalingapi.h"
#include "dwmapi.h"
#include "windowsx.h"
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dcomp.h>
#include <wrl.h>
#include "d3dx12.h"


using namespace Microsoft::WRL;

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
inline void SUCCESS(HRESULT hr)
{
	if (hr != S_OK)
		throw(hr);
}

class Frame : public RLL::IFrame
{
	DWORD extStyles = 0;
	HWND hWnd;
	ComPtr<ID3D12Fence> gFence;
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	ComPtr<IDXGISwapChain1> swapChain;
	ComPtr<IDCompositionTarget> dcompTarget;
	ComPtr<IDCompositionVisual> dcompVisual;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12Resource> scBuffer[2];
	ComPtr<ID3D12Resource> maBuffer[2];
	UINT rtvDescriptorSize;
	int currentBuffer = 0;
	int cFence = 0;

	CD3DX12_VIEWPORT viewport;
	CD3DX12_RECT scissorRect;

	Math3D::Vector4 AquireWindowRect();
	void SetViewRect(RECT&);
	void FlushCommandQueue();
	void ResizeView();
	void CreateRenderTargets();
public:
	Frame(Frame* parent, Math3D::Vector2 size, Math3D::Vector2 pos);
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	LRESULT NextProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	void Show();
	void Run();
};

struct PathBuffer;

ID3D12Device5* GetD3DDevice();
IDCompositionDevice* GetDCompDevice();
PathBuffer* GetPathBuffer();
struct ResourceBlob
{
	int size;
	void* data;
	ID3D12Resource* gpuBuffer;
	void Sync(void* data)
	{
		memcpy(this->data, data, size);
	}
	template<typename T>
	void Sync(T& st)
	{
		Sync(&st);
	}
};
