#include "win32paint_d12.h"
using namespace RLL;
using namespace std;
using namespace Math3D;

D3D12PaintContext::D3D12PaintContext(D3D12PaintDevice* pdev) : device(pdev)
{
	auto d3dDevice = pdev->d3dDevice.Get();
	state = D3D12_RESOURCE_STATE_COMMON;
	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	SUCCESS(d3dDevice->CreateCommandList(0, //掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //命令列表类型
		cmdAllocator.Get(),	//命令分配器接口指针
		nullptr,	//流水线状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&cmdList)));	//返回创建的命令列表
	cmdList->Close();

	gpuCbFrame = device->CreateUploadBuffer(sizeof(cbFrame));
	gpuCbObject = device->CreateUploadBuffer(sizeof(cbObjTest));


	cbObjTest.objToWorld = Matrix4x4::Identity();

	gpuCbObject.Sync(cbObjTest);
	transformCache.push_back(gpuCbObject);

}
void D3D12PaintContext::SetRenderTarget(RLL::IRenderTarget* rt)
{
	cmdList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
}
void D3D12PaintContext::BeginDraw()
{
	flushAvaliable = false;
	cTransform = 1;

	SUCCESS(cmdAllocator->Reset());//重复使用记录命令的相关内存
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//复用命令列表及其内存

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rt.Get(), state, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);
	cmdList->OMSetRenderTargets(1,//待绑定的RTV数量
		&rtvHandle,	//指向RTV数组的指针
		true,	//RTV对象在堆内存中是连续存放的
		nullptr);	//指向DSV的指针
	cmdList->SetGraphicsRootSignature(device->coreSignature.Get());
	cmdList->SetPipelineState(device->corePSO.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootConstantBufferView(0, gpuCbObject.gpuBuffer->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, gpuCbFrame.gpuBuffer->GetGPUVirtualAddress());
	ID3D12DescriptorHeap* heaps[] = { device->gpuTextureHeap.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);
	cmdList->SetGraphicsRootDescriptorTable(2, device->gpuTextureHeap.hGPU(0));
	cmdList->SetGraphicsRootShaderResourceView(3, device->gpuPath->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootShaderResourceView(4, device->gpuCurve->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootShaderResourceView(5, device->gpuBrush->GetGPUVirtualAddress());

}
void D3D12PaintContext::Clear(RLL::Color clear)
{
	cmdList->ClearRenderTargetView(rtvHandle, (FLOAT*)&clear, 0, nullptr);
}
void D3D12PaintContext::PushClip(RLL::Rectangle clip)
{
	NOIMPL;
}
void D3D12PaintContext::PopClip()
{
	NOIMPL;
}
void D3D12PaintContext::EndDraw()
{
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rt.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, state));
	cmdList->Close();
	flushAvaliable = true;
}
void D3D12PaintContext::SetTransform(Math3D::Matrix4x4& tfCache)
{
	if (cTransform >= transformCache.size())
	{
		ResourceBlob rb = device->CreateUploadBuffer(sizeof(CBObject));;
		transformCache.push_back(rb);
	}
	CBObject cbo;
	cbo.objToWorld = tfCache;
	transformCache[cTransform].Sync(cbo);
	cmdList->SetGraphicsRootConstantBufferView(0, transformCache[cTransform].gpuBuffer->GetGPUVirtualAddress());
	cTransform++;
}
void D3D12PaintContext::DrawSVG(RLL::ISVG* svg)
{
	auto s = dynamic_cast<D3D12SVG*>(svg);
	if (!s) { SetError("Input svg is not created by this context's device."); return; }
	auto m = s->mesh;
	cmdList->IASetIndexBuffer(&m->ibv);
	cmdList->IASetVertexBuffers(0, 4, m->vbv);

	cmdList->DrawIndexedInstanced(m->idxCount, 1, 0, 0, 0);
}


D3D12FramePaintContext::D3D12FramePaintContext(D3D12PaintDevice* pdev) : D3D12PaintContext(pdev)
{
	state = D3D12_RESOURCE_STATE_PRESENT;
}

void D3D12FramePaintContext::BeginDraw()
{
	flushAvaliable = false;
	cTransform = 1;
	cbFrame.gWorldViewProj = Matrix4x4::LookTo({ 0,0,-500 }, { 0,0,1 }) *
		Matrix4x4::Scaling({ 1,-1,1 }) *
		Matrix4x4::Orthographic(device->viewport.Width, device->viewport.Height, 1, 1000);
	cbFrame.vwh = { (unsigned int)device->viewport.Width,(unsigned int)device->viewport.Height };
	gpuCbFrame.Sync(cbFrame);

	bool& msaa = device->msaaEnable;
	auto& cbuf = device->currentBuffer;
	SUCCESS(cmdAllocator->Reset());//重复使用记录命令的相关内存
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//复用命令列表及其内存
	if (msaa)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->maBuffer[cbuf].Get(),//转换资源为后台缓冲区资源
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));//从呈现到渲染目标转换
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));//从渲染目标到呈现
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));//从渲染目标到呈现
	}
	cmdList->RSSetViewports(1, &device->viewport);
	cmdList->RSSetScissorRects(1, &device->scissorRect);
	cmdList->OMSetRenderTargets(1,//待绑定的RTV数量
		&device->rtvHeap.hCPU(cbuf),	//指向RTV数组的指针
		true,	//RTV对象在堆内存中是连续存放的
		nullptr);	//指向DSV的指针
	cmdList->SetGraphicsRootSignature(device->coreSignature.Get());
	cmdList->SetPipelineState(device->corePSO.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->SetGraphicsRootConstantBufferView(0, gpuCbObject.gpuBuffer->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, gpuCbFrame.gpuBuffer->GetGPUVirtualAddress());
	ID3D12DescriptorHeap* heaps[] = { device->gpuTextureHeap.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);
	cmdList->SetGraphicsRootDescriptorTable(2, device->gpuTextureHeap.hGPU(0));
	cmdList->SetGraphicsRootShaderResourceView(3, device->gpuPath->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootShaderResourceView(4, device->gpuCurve->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootShaderResourceView(5, device->gpuBrush->GetGPUVirtualAddress());
}
void D3D12FramePaintContext::EndDraw()
{
	bool& msaa = device->msaaEnable;
	auto& cbuf = device->currentBuffer;
	if (msaa)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->maBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));//从渲染目标到呈现
		cmdList->ResolveSubresource(device->scBuffer[cbuf].Get(), 0, device->maBuffer[cbuf].Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));//从渲染目标到呈现
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));//从渲染目标到呈现
	}
	cmdList->Close();
	flushAvaliable = true;
}

void D3D12FramePaintContext::Clear(RLL::Color clear)
{
	auto& cbuf = device->currentBuffer;
	cmdList->ClearRenderTargetView(device->rtvHeap.hCPU(cbuf), (FLOAT*)&clear, 0, nullptr);
}
