#include "DescriptorHeap.h"

HRESULT DescriptorHeap::Create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	heapDesc.Type = type;
	heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.NodeMask = 0;
	auto hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
	if (SUCCEEDED(hr))
	{
		CPUHandle = heap->GetCPUDescriptorHandleForHeapStart();		
		GPUHandle = heap->GetGPUDescriptorHandleForHeapStart();
	}
	incrementSize = device->GetDescriptorHandleIncrementSize(type);
	return hr;

}