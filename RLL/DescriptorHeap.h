#pragma once
#include "win32internal.h"
using namespace Microsoft::WRL;

class DescriptorHeap
{

	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	UINT incrementSize;
	ComPtr<ID3D12DescriptorHeap> heap;
	int refOffset = -1;

public:
	HRESULT Create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible = false);
	DescriptorHeap() = default;
	DescriptorHeap(const DescriptorHeap& d, int offsetIndex = 0)
	{
		heap = d.heap;
		CPUHandle = d.CPUHandle;
		GPUHandle = d.GPUHandle;
		incrementSize = d.incrementSize;
		CPUHandle = hCPU(offsetIndex);
		GPUHandle = hGPU(offsetIndex);
		refOffset = offsetIndex;
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE hCPU(int index = 0)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE h;
		h.ptr = CPUHandle.ptr + index * incrementSize;
		return h;
	}
	inline D3D12_GPU_DESCRIPTOR_HANDLE hGPU(int index = 0)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE h;
		h.ptr = GPUHandle.ptr + index * incrementSize;
		return h;
	}
	inline ID3D12DescriptorHeap* Get()
	{
		return heap.Get();
	}
	inline ID3D12DescriptorHeap** Ptr()
	{
		return &heap;
	}
	inline void Reset()
	{
		heap.Reset();
	}
};