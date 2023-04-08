#include "win32paint_d12.h"
#include <d3dcompiler.h>

using namespace RLL;
using namespace std;
using namespace Math3D;
#define SHADER_CORE L"shader\\core.hlsl"
#define ARRLEN(x) (sizeof(x)/sizeof(*x))

IPaintDevice* RLL::CreatePaintDevice()
{
	return new D3D12PaintDevice();
}


inline ComPtr<ID3DBlob>  CompileShader(
	const wchar_t* fileName,
	const D3D_SHADER_MACRO* defines,
	const char* enteryPoint,
	const char* target)
{
	//�����ڵ���ģʽ����ʹ�õ��Ա�־
	UINT compileFlags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if defined(DEBUG) || defined(_DEBUG)
	//�õ���ģʽ��������ɫ�� | ָʾ�����������Ż��׶�
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(fileName, //hlslԴ�ļ���
		defines,	//�߼�ѡ�ָ��Ϊ��ָ��
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	//�߼�ѡ�����ָ��Ϊ��ָ��
		enteryPoint,	//��ɫ������ڵ㺯����
		target,		//ָ��������ɫ�����ͺͰ汾���ַ���
		compileFlags,	//ָʾ����ɫ���ϴ���Ӧ����α���ı�־
		0,	//�߼�ѡ��
		&byteCode,	//����õ��ֽ���
		&errors);	//������Ϣ

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	//ThrowIfFailed(hr);

	return byteCode;
}
void GetHardwareAdapterA(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter = false)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;

	ComPtr<IDXGIFactory6> factory6;

	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter));
			++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}
	else
	{
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}

constexpr UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}
void D3D12PaintDevice::CreateDevices(int flags)
{
	UINT dxgiFactoryFlags = 0;
	if (flags & INIT_DIRECTX_DEBUG_LAYER)
	{
		ComPtr<ID3D12Debug> debugController;
		if (D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)) == S_OK)
		{
			debugController->EnableDebugLayer();
			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
	SUCCESS(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
	bool m_useWarpDevice = false;
	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		SUCCESS(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		SUCCESS(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&d3dDevice)
		));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapterA(dxgiFactory.Get(), &hardwareAdapter);

		SUCCESS(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(&d3dDevice)
		));
	}

	LoadShaders();

}
D3D12PaintDevice::D3D12PaintDevice()
{
	paths.brushes.push_back(D3D12GPUBrush::Foreground());
	CreateDevices(INIT_DIRECTX_DEBUG_LAYER);
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	SUCCESS(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	SUCCESS(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	//SUCCESS(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&cmdAllocatorCopy)));
	SUCCESS(d3dDevice->CreateCommandList(0, //����ֵΪ0����GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //�����б�����
		cmdAllocator.Get(),	//����������ӿ�ָ��
		nullptr,	//��ˮ��״̬����PSO�����ﲻ���ƣ����Կ�ָ��
		IID_PPV_ARGS(&cmdList)));	//���ش����������б�
	SUCCESS(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)));

}

void D3D12PaintDevice::LoadShaders()
{
	D3D12_STATIC_SAMPLER_DESC staticSampler;
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.MinLOD = 0;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.MipLODBias = 0.0f;
	staticSampler.MaxAnisotropy = 1;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.ShaderRegister = 0;
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSig;
	CD3DX12_ROOT_PARAMETER rootParams[6] = {};
	auto range = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, -1, 0, 0);
	rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParams[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
	rootParams[2].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParams[3].InitAsShaderResourceView(0, 1);
	rootParams[4].InitAsShaderResourceView(1, 1);
	rootParams[5].InitAsShaderResourceView(1, 2);
	rootSig.Init
	(ARRLEN(rootParams), //������������
		rootParams, //������ָ��
		1, &staticSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSig, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	SUCCESS(hr);

	//std::cout << serializedRootSig->GetBufferPointer() << "||" << serializedRootSig->GetBufferSize() << std::endl;
	SUCCESS(d3dDevice->CreateRootSignature(0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&coreSignature)));

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc;
	inputLayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "PNB", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UVTF", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	ComPtr<ID3DBlob> vbc = CompileShader(SHADER_CORE, nullptr, "VS", "vs_5_1");
	ComPtr<ID3DBlob> pbc = CompileShader(SHADER_CORE, nullptr, "PS", "ps_5_1");

	auto rs = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rs.CullMode = D3D12_CULL_MODE_NONE;
	//rs.FillMode = D3D12_FILL_MODE_WIREFRAME;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	SUCCESS(d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	msaaQuality = msQualityLevels.NumQualityLevels;
	assert(msaaQuality > 0 && "Unexpected MSAA quality level.");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { inputLayoutDesc.data(), (UINT)inputLayoutDesc.size() };
	psoDesc.pRootSignature = coreSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(vbc->GetBufferPointer()), vbc->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(pbc->GetBufferPointer()), pbc->GetBufferSize() };
	psoDesc.RasterizerState = rs;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,ȫ��������û������
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//��һ�����޷�������
	//psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;	//��һ�����޷�������
	//psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = msaaEnable ? 4 : 1;	//��ʹ��4XMSAA
	psoDesc.SampleDesc.Quality = msaaEnable ? msaaQuality - 1 : 0;	////��ʹ��4XMSAA

		//Aplha Blend Settings
	D3D12_BLEND_DESC& desc = psoDesc.BlendState;
	desc.AlphaToCoverageEnable = false;
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//psoDesc.BlendState = desc;
	psoDesc.DepthStencilState.DepthEnable = false;

	SUCCESS(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&corePSO)));

	gpuTextureHeap.Create(d3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048, true);
	///dbg
	pbc = CompileShader(L"shader\\dbgWire.hlsl", nullptr, "PS", "ps_5_1");
	vbc = CompileShader(L"shader\\dbgWire.hlsl", nullptr, "VS", "vs_5_1");
	rs.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.PS = { reinterpret_cast<BYTE*>(pbc->GetBufferPointer()), pbc->GetBufferSize() };
	psoDesc.VS = { reinterpret_cast<BYTE*>(vbc->GetBufferPointer()), vbc->GetBufferSize() };
	psoDesc.RasterizerState = rs;
	SUCCESS(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dbgWirePSO)));

}

IPaintContext* D3D12PaintDevice::CreateContext(int flags)
{
	auto ctx = new D3D12PaintContext(this);
	contexs.push_back(ctx);
	return ctx;
}
RLL::IPaintContext* D3D12PaintDevice::CreateContextForFrame(RLL::IFrame* f, int flags)
{
	auto wf = dynamic_cast<Frame*>(f);
	if (!wf)
	{
		SetError("No capable with this IFrame.");
		return nullptr;
	}
	auto ctx = new D3D12FramePaintContext(this, wf);
	contexs.push_back(ctx);
	return ctx;
}
int D3D12PaintDevice::AllocateBrush()
{
	for (auto i = paths.brushes.begin(); i != paths.brushes.end(); i++)
	{
		if (i->pad == 1)
		{
			return i - paths.brushes.begin();
		}
	}
	paths.brushes.push_back({});
	return paths.brushes.size() - 1;
}
int D3D12PaintDevice::AllocatePath(D3D12GPUPath& path)
{
	for (auto i = paths.paths.begin(); i != paths.paths.end(); i++)
	{
		if (i->hi == -1)
		{
			*i = path;
			return i - paths.paths.begin();
		}
	}
	paths.paths.push_back(path);
	return paths.paths.size() - 1;
}
int D3D12PaintDevice::AllocateMesh()
{
	for (auto i = paths.meshes.begin(); i != paths.meshes.end(); i++)
	{
		if (*i == nullptr)
		{
			*i = new CoreMesh();
			return i - paths.meshes.begin();
		}
	}
	paths.meshes.push_back(new CoreMesh());
	return paths.meshes.size() - 1;
}
IBrush* D3D12PaintDevice::CreateSolidColorBrush(RLL::Color c)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = AllocateBrush();
	//paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Solid(c);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IBrush* D3D12PaintDevice::CreateDirectionalBrush(Math3D::Vector2 direction, RLL::ColorGradient* grad)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = AllocateBrush();
	//paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Directional(direction,
		grad->positions[0], grad->colors[0],
		grad->positions[1], grad->colors[1],
		grad->positions[2], grad->colors[2],
		grad->positions[3], grad->colors[3],
		grad->positions[4], grad->colors[4],
		grad->positions[5], grad->colors[5],
		grad->positions[6], grad->colors[6],
		grad->positions[7], grad->colors[7]);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IBrush* D3D12PaintDevice::CreateRadialBrush(Math3D::Vector2 center, float radius, RLL::ColorGradient* grad)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = AllocateBrush();
	//paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Radial(center, radius,
		grad->positions[0], grad->colors[0],
		grad->positions[1], grad->colors[1],
		grad->positions[2], grad->colors[2],
		grad->positions[3], grad->colors[3],
		grad->positions[4], grad->colors[4],
		grad->positions[5], grad->colors[5],
		grad->positions[6], grad->colors[6],
		grad->positions[7], grad->colors[7]);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IBrush* D3D12PaintDevice::CreateSweepBrush(Math3D::Vector2 center, float degree, RLL::ColorGradient* grad)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = AllocateBrush();
	//paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Sweep(center, degree,
		grad->positions[0], grad->colors[0],
		grad->positions[1], grad->colors[1],
		grad->positions[2], grad->colors[2],
		grad->positions[3], grad->colors[3],
		grad->positions[4], grad->colors[4],
		grad->positions[5], grad->colors[5],
		grad->positions[6], grad->colors[6],
		grad->positions[7], grad->colors[7]);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IGeometryBuilder* D3D12PaintDevice::CreateGeometryBuilder()
{
	return new D3D12GeometryBuilder(this);
}
ISVGBuilder* D3D12PaintDevice::CreateSVGBuilder()
{
	return new D3D12SVGBuilder(this);

}
ID3D12Resource* D3D12PaintDevice::CreateDefaultBuffer(const void* initData, UINT64 size, D3D12_RESOURCE_FLAGS flag)
{
	ID3D12Resource* uploadBuffer = nullptr;
	//�����ϴ��ѣ������ǣ�д��CPU�ڴ����ݣ��������Ĭ�϶�
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), //�����ϴ������͵Ķ�
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),//����Ĺ��캯��������byteSize��������ΪĬ��ֵ������д
		D3D12_RESOURCE_STATE_GENERIC_READ,	//�ϴ��������Դ��Ҫ���Ƹ�Ĭ�϶ѣ������ǿɶ�״̬
		nullptr,	//�������ģ����Դ������ָ���Ż�ֵ
		IID_PPV_ARGS(&uploadBuffer)));

	//����Ĭ�϶ѣ���Ϊ�ϴ��ѵ����ݴ������
	ID3D12Resource* defaultBuffer = nullptr;
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//����Ĭ�϶����͵Ķ�
		D3D12_HEAP_FLAG_SHARED,
		&CD3DX12_RESOURCE_DESC::Buffer(size
			, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_COMMON,//Ĭ�϶�Ϊ���մ洢���ݵĵط���������ʱ��ʼ��Ϊ��ͨ״̬
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));



	//����Դ��COMMON״̬ת����COPY_DEST״̬��Ĭ�϶Ѵ�ʱ��Ϊ�������ݵ�Ŀ�꣩
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	//�����ݴ�CPU�ڴ濽����GPU����
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = size;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	//���ĺ���UpdateSubresources�������ݴ�CPU�ڴ濽�����ϴ��ѣ��ٴ��ϴ��ѿ�����Ĭ�϶ѡ�1����������Դ���±꣨ģ���ж��壬��Ϊ��2������Դ��
	UpdateSubresources<1>(cmdList.Get(), defaultBuffer, uploadBuffer, 0, 0, 1, &subResourceData);

	//�ٴν���Դ��COPY_DEST״̬ת����GENERIC_READ״̬(����ֻ�ṩ����ɫ������)
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));



	return defaultBuffer;

}
ResourceBlob D3D12PaintDevice::CreateUploadBuffer(UINT64 size)
{
	ResourceBlob blob;
	ID3D12Resource* gpuBuffer = nullptr;
	UINT elementByteSize;
	blob.size = size;
	elementByteSize = CalcConstantBufferByteSize(size);
	blob.size = elementByteSize;
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&gpuBuffer)));
	//������������Դ��ָ��
	gpuBuffer->Map(0,//����Դ���������ڻ�������˵����������Դ�����Լ�
		nullptr,//��������Դ����ӳ��
		&blob.data);//���ش�ӳ����Դ���ݵ�Ŀ���ڴ��
	blob.gpuBuffer = gpuBuffer;

	return blob;

}
ID3D12Resource* D3D12PaintDevice::MakeDefaultBuffer(ID3D12Resource* upload, int size)
{
	//����Ĭ�϶ѣ���Ϊ�ϴ��ѵ����ݴ������
	ID3D12Resource* defaultBuffer = nullptr;
	size = CalcConstantBufferByteSize(size);
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//����Ĭ�϶����͵Ķ�
		D3D12_HEAP_FLAG_SHARED,
		&CD3DX12_RESOURCE_DESC::Buffer(size
			, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_COMMON,//Ĭ�϶�Ϊ���մ洢���ݵĵط���������ʱ��ʼ��Ϊ��ͨ״̬
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));


	//����Դ��COMMON״̬ת����COPY_DEST״̬��Ĭ�϶Ѵ�ʱ��Ϊ�������ݵ�Ŀ�꣩
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	cmdList->CopyResource(defaultBuffer, upload);
	//�ٴν���Դ��COPY_DEST״̬ת����GENERIC_READ״̬(����ֻ�ṩ����ɫ������)
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;

}
void D3D12PaintDevice::Wait()
{
	cFence++;	//CPU��������رպ󣬽���ǰΧ��ֵ+1
	cmdQueue->Signal(gFence.Get(), cFence);	//��GPU������CPU���������󣬽�fence�ӿ��е�Χ��ֵ+1����fence->GetCompletedValue()+1
	if (gFence->GetCompletedValue() < cFence)	//���С�ڣ�˵��GPUû�д�������������
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");	//�����¼�
		gFence->SetEventOnCompletion(cFence, eventHandle);//��Χ���ﵽmCurrentFenceֵ����ִ�е�Signal����ָ���޸���Χ��ֵ��ʱ������eventHandle�¼�
		WaitForSingleObject(eventHandle, INFINITE);//�ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������ע���Enent���������ٵȴ���
							   //���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
		CloseHandle(eventHandle);
	}

}

void D3D12PaintDevice::CommitChange()
{
	if (flag & FLAG_D12PD_PATH_DIRTY)
	{
		gpuPath = CreateDefaultBuffer(&paths.paths[0], sizeof(D3D12GPUPath) * paths.paths.size(), D3D12_RESOURCE_FLAG_NONE);
		flag &= ~FLAG_D12PD_PATH_DIRTY;
	}
	if (flag & FLAG_D12PD_CURVE_DIRTY)
	{
		gpuCurve = CreateDefaultBuffer(&paths.curves[0], sizeof(D3D12GPUCurve) * paths.curves.size(), D3D12_RESOURCE_FLAG_NONE);
		flag &= ~FLAG_D12PD_CURVE_DIRTY;

	}
	if (flag & FLAG_D12PD_BRUSH_DIRTY)
	{
		gpuBrush = CreateDefaultBuffer(&paths.brushes[0], sizeof(D3D12GPUBrush) * paths.brushes.size(), D3D12_RESOURCE_FLAG_NONE);
		flag &= ~FLAG_D12PD_BRUSH_DIRTY;

	}
	if (flag & FLAG_D12PD_MESH_DIRTY)
	{
		if (cMeshGroup.meshs.size() != 0)
		{
			int ind = 0, vex = 0;
			for (auto& i : cMeshGroup.meshs)
			{
				ind += i->idxCount;
				vex += i->vertCount;
			}
			cMeshGroup.uploads[0] = CreateUploadBuffer(ind * sizeof(*CoreMesh::indices));
			cMeshGroup.uploads[1] = CreateUploadBuffer(vex * sizeof(*CoreMesh::vertices));
			cMeshGroup.uploads[2] = CreateUploadBuffer(vex * sizeof(*CoreMesh::uvs));
			cMeshGroup.uploads[3] = CreateUploadBuffer(vex * sizeof(*CoreMesh::pnbs));
			cMeshGroup.uploads[4] = CreateUploadBuffer(vex * sizeof(*CoreMesh::tfs));
			ind = 0; vex = 0;
#define arrcpyi(dst,src,di,sz) memcpy( (( (decltype(src))dst ) + (di) ) ,(src),sizeof(*src)*(sz))
			for (auto i : cMeshGroup.meshs)
			{
				arrcpyi(cMeshGroup.uploads[0].data, i->indices, ind, i->idxCount);
				arrcpyi(cMeshGroup.uploads[1].data, i->vertices, vex, i->vertCount);
				arrcpyi(cMeshGroup.uploads[2].data, i->uvs, vex, i->vertCount);
				arrcpyi(cMeshGroup.uploads[3].data, i->pnbs, vex, i->vertCount);
				arrcpyi(cMeshGroup.uploads[4].data, i->tfs, vex, i->vertCount);

				ind += i->idxCount;
				vex += i->vertCount;
			}
			cMeshGroup.resource[0] = MakeDefaultBuffer(cMeshGroup.uploads[0].gpuBuffer, ind * sizeof(*CoreMesh::indices));
			cMeshGroup.resource[1] = MakeDefaultBuffer(cMeshGroup.uploads[1].gpuBuffer, vex * sizeof(*CoreMesh::vertices));
			cMeshGroup.resource[2] = MakeDefaultBuffer(cMeshGroup.uploads[2].gpuBuffer, vex * sizeof(*CoreMesh::uvs));
			cMeshGroup.resource[3] = MakeDefaultBuffer(cMeshGroup.uploads[3].gpuBuffer, vex * sizeof(*CoreMesh::pnbs));
			cMeshGroup.resource[4] = MakeDefaultBuffer(cMeshGroup.uploads[4].gpuBuffer, vex * sizeof(*CoreMesh::tfs));

			ind = 0; vex = 0;
			for (auto i : cMeshGroup.meshs)
			{
				i->Uploaded(cMeshGroup.resource, ind, vex);

				ind += i->idxCount;
				vex += i->vertCount;
			}


			cMeshGroup.uploads[0].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[1].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[2].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[3].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[4].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[0].data = nullptr;
			cMeshGroup.uploads[1].data = nullptr;
			cMeshGroup.uploads[2].data = nullptr;
			cMeshGroup.uploads[3].data = nullptr;
			cMeshGroup.uploads[4].data = nullptr;
		}
	}

	SUCCESS(cmdList->Close());
	std::vector<ID3D12CommandList*> lists({ cmdList.Get() });
	cmdQueue->ExecuteCommandLists(lists.size(), lists.data());//������������б����������

	//flag = 0;
}
void D3D12PaintDevice::PostFlush()
{
	if (flag & FLAG_D12PD_MESH_DIRTY)
	{
		for (auto& i : cMeshGroup.uploads)
		{
			i.gpuBuffer->Release();
		}
		meshGroups.push_back(cMeshGroup);
		cMeshGroup.meshs.clear();
		flag &= ~FLAG_D12PD_MESH_DIRTY;
	}

	SUCCESS(cmdAllocator->Reset());//�ظ�ʹ�ü�¼���������ڴ�
	SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//���������б����ڴ�

}
void D3D12PaintDevice::UploadMesh(CoreMesh* m)
{
	flag |= FLAG_D12PD_MESH_DIRTY;
	cMeshGroup.meshs.push_back(m);
}
