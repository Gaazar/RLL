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
	"rll language  \
c++ intergration lmao";

	cbObjTest.objToWorld = Matrix4x4::Identity();

	gpuCbObject.Sync(cbObjTest);
	transformCache.push_back(gpuCbObject);

}
void  D3D12PaintContext::Flush()
{
	device->CommitChange();
	SUCCESS(device->cmdList->Close());
	std::vector<ID3D12CommandList*> lists({ device->cmdList.Get(),cmdList.Get() });
	device->cmdQueue->ExecuteCommandLists(lists.size(), lists.data());//将命令从命令列表传至命令队列

	device->Wait();
	SUCCESS(cmdAllocator->Reset());//重复使用记录命令的相关内存
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//复用命令列表及其内存
	device->PostFlush();
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
	Flush();

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


D3D12FramePaintContext::D3D12FramePaintContext(D3D12PaintDevice* pdev, Frame* f) : D3D12PaintContext(pdev)
{
	state = D3D12_RESOURCE_STATE_PRESENT;
	frame = f;
	//CreateCommandThings
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	SUCCESS(device->d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	//SUCCESS(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmdAllocatorCopy)));
	SUCCESS(device->d3dDevice->CreateCommandList(0, //掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //命令列表类型
		cmdAllocator.Get(),	//命令分配器接口指针
		nullptr,	//流水线状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&cmdList)));	//返回创建的命令列表
	//SUCCESS(d3dDevice->CreateCommandList(0, //掩码值为0，单GPU
	//	D3D12_COMMAND_LIST_TYPE_COPY, //命令列表类型
	//	cmdAllocatorCopy.Get(),	//命令分配器接口指针
	//	nullptr,	//流水线状态对象PSO，这里不绘制，所以空指针
	//	IID_PPV_ARGS(&cmdListCopy)));	//返回创建的命令列表
	cmdList->Close();

	//CreateSwapChain
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	RECT rc;
	GetClientRect(f->hWnd, &rc);
	scDesc.Width = rc.right - rc.left;
	scDesc.Height = rc.bottom - rc.top;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = 2;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.Stereo = false;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
	SUCCESS(device->dxgiFactory->CreateSwapChainForComposition(device->cmdQueue.Get(), &scDesc, nullptr, &swapChain));
	ResizeView();

	SUCCESS(DCompositionCreateDevice3(nullptr, IID_PPV_ARGS(&dCompDevice)));

	SUCCESS(dCompDevice->CreateTargetForHwnd(f->hWnd, true, &dcompTarget));
	SUCCESS(dCompDevice->CreateVisual(&dcompVisual));
	SUCCESS(dcompVisual->SetContent(swapChain.Get()));
	SUCCESS(dcompTarget->SetRoot(dcompVisual.Get()));
	SUCCESS(dCompDevice->Commit());

}

void D3D12FramePaintContext::CreateRenderTargets()
{
	auto& msaaEnable = device->msaaEnable;
	auto& msaaQuality = device->msaaQuality;
	auto d3dDevice = device->d3dDevice;
	rtvHeap.Create(d3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	D3D12_CLEAR_VALUE clearValue;    // Performance tip: Tell the runtime at resource creation the desired clear value.
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 1;
	clearValue.Color[1] = 1;
	clearValue.Color[2] = 1;
	clearValue.Color[3] = 1;

	for (int i = 0; i < 2; i++)
	{
		//获得存于交换链中的后台缓冲区资源
		swapChain->GetBuffer(i, IID_PPV_ARGS(&scBuffer[i]));
		if (msaaEnable)
		{
			SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), //创建上传堆类型的堆
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, viewport.Width, viewport.Height,
					1, 1, 4, msaaQuality - 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),//变体的构造函数，传入byteSize，其他均为默认值，简化书写
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				&clearValue,	//不是深度模板资源，不用指定优化值
				IID_PPV_ARGS(&maBuffer[i])));

			//创建RTV
			d3dDevice->CreateRenderTargetView(maBuffer[i].Get(),
				nullptr,	//在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
				rtvHeap.hCPU(i));	//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
			//偏移到描述符堆中的下一个缓冲区
		}
		else
		{
			d3dDevice->CreateRenderTargetView(scBuffer[i].Get(),
				nullptr,	//在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
				rtvHeap.hCPU(i));	//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）

		}
	}
	currentBuffer = 0;
}
void D3D12FramePaintContext::ResizeView()
{
	auto sz = SizeI(frame->viewRect.width(), frame->viewRect.height());
	auto pw = viewport.Width, ph = viewport.Height;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = sz.x;
	scissorRect.bottom = sz.y;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = sz.x;
	viewport.Height = sz.y;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	if (pw != viewport.Width || ph != viewport.Height)
	{
		for (int i = 0; i < 2; i++)
		{
			scBuffer[i].Reset();
		}
		SUCCESS(swapChain->ResizeBuffers(2, viewport.Width, viewport.Height, DXGI_FORMAT_UNKNOWN, 0));
		CreateRenderTargets();
	}
}
void  D3D12FramePaintContext::Flush()
{
	device->CommitChange();
	std::vector<ID3D12CommandList*> lists({ cmdList.Get() });
	device->cmdQueue->ExecuteCommandLists(lists.size(), lists.data());//将命令从命令列表传至命令队列
	SUCCESS(swapChain->Present(0, 0));
	currentBuffer = (currentBuffer + 1) % 2;

	device->Wait();

	device->PostFlush();
 }

void D3D12FramePaintContext::BeginDraw()
{
	flushAvaliable = false;
	cTransform = 1;
	cbFrame.gWorldViewProj = Matrix4x4::LookTo({ 0,0,-500 }, { 0,0,1 }) *
		Matrix4x4::Scaling({ 1,-1,1 }) *
		Matrix4x4::Orthographic(viewport.Width, viewport.Height, 1, 1000);
	cbFrame.vwh = { (unsigned int)viewport.Width,(unsigned int)viewport.Height };
	cbFrame.time = TimeSecond();
	gpuCbFrame.Sync(cbFrame);

	bool& msaa = device->msaaEnable;
	auto& cbuf = currentBuffer;
	SUCCESS(cmdAllocator->Reset());//重复使用记录命令的相关内存
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//复用命令列表及其内存
	if (msaa)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(maBuffer[cbuf].Get(),//转换资源为后台缓冲区资源
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));//从呈现到渲染目标转换
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));//从渲染目标到呈现
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));//从渲染目标到呈现
	}
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);
	cmdList->OMSetRenderTargets(1,//待绑定的RTV数量
		&rtvHeap.hCPU(cbuf),	//指向RTV数组的指针
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
	auto& cbuf = currentBuffer;
	if (msaa)
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(maBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));//从渲染目标到呈现
		cmdList->ResolveSubresource(scBuffer[cbuf].Get(), 0, maBuffer[cbuf].Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));//从渲染目标到呈现
	}
	else
	{
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[cbuf].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));//从渲染目标到呈现
	}
	cmdList->Close();
	flushAvaliable = true;
	Flush();
}

void D3D12FramePaintContext::Clear(RLL::Color clear)
{
	auto& cbuf = currentBuffer;
	cmdList->ClearRenderTargetView(rtvHeap.hCPU(cbuf), (FLOAT*)&clear, 0, nullptr);
}
