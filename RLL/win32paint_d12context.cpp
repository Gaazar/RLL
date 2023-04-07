#include "win32paint_d12.h"
using namespace RLL;
using namespace std;
using namespace Math3D;

D3D12PaintContext::D3D12PaintContext(D3D12PaintDevice* pdev) : device(pdev)
{
	auto d3dDevice = pdev->d3dDevice.Get();
	state = D3D12_RESOURCE_STATE_COMMON;
	d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	SUCCESS(d3dDevice->CreateCommandList(0, //����ֵΪ0����GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //�����б�����
		cmdAllocator.Get(),	//����������ӿ�ָ��
		nullptr,	//��ˮ��״̬����PSO�����ﲻ���ƣ����Կ�ָ��
		IID_PPV_ARGS(&cmdList)));	//���ش����������б�
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

	SUCCESS(cmdAllocator->Reset());//�ظ�ʹ�ü�¼���������ڴ�
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//���������б����ڴ�

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rt.Get(), state, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);
	cmdList->OMSetRenderTargets(1,//���󶨵�RTV����
		&rtvHandle,	//ָ��RTV�����ָ��
		true,	//RTV�����ڶ��ڴ�����������ŵ�
		nullptr);	//ָ��DSV��ָ��
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
	SUCCESS(cmdAllocator->Reset());//�ظ�ʹ�ü�¼���������ڴ�
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//���������б����ڴ�
	if (msaa)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->maBuffer[cbuf].Get(),//ת����ԴΪ��̨��������Դ
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));//�ӳ��ֵ���ȾĿ��ת��
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));//����ȾĿ�굽����
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));//����ȾĿ�굽����
	}
	cmdList->RSSetViewports(1, &device->viewport);
	cmdList->RSSetScissorRects(1, &device->scissorRect);
	cmdList->OMSetRenderTargets(1,//���󶨵�RTV����
		&device->rtvHeap.hCPU(cbuf),	//ָ��RTV�����ָ��
		true,	//RTV�����ڶ��ڴ�����������ŵ�
		nullptr);	//ָ��DSV��ָ��
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
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));//����ȾĿ�굽����
		cmdList->ResolveSubresource(device->scBuffer[cbuf].Get(), 0, device->maBuffer[cbuf].Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));//����ȾĿ�굽����
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(device->scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));//����ȾĿ�굽����
	}
	cmdList->Close();
	flushAvaliable = true;
}

void D3D12FramePaintContext::Clear(RLL::Color clear)
{
	auto& cbuf = device->currentBuffer;
	cmdList->ClearRenderTargetView(device->rtvHeap.hCPU(cbuf), (FLOAT*)&clear, 0, nullptr);
}
