#include "win32internal.h"
#include "DirectXColors.h"
#include <d3dcompiler.h>
#include "DescriptorHeap.h"
#include "win32core_mesh.h"
#include "win32paint_d12.h"
#include "fi_ft.h"
#include "rlltest.h"

using namespace Math3D;
#define ARRLEN(x) (sizeof(x)/sizeof(*x))
#define SHADER_CORE L"shader\\core.hlsl"
#define MSAA 0

Vector2 dpiScaleFactor = { 1,1 };
wchar_t locale[LOCALE_NAME_MAX_LENGTH];
ComPtr<ID3D12Device5> d3dDevice;
ComPtr<IDCompositionDevice> dCompDevice;
ComPtr<IDXGIDevice1> dxgiDevice;
ComPtr<IDXGIFactory5> dxgiFactory;
ComPtr<ID3D12RootSignature> coreSignature;
ComPtr<ID3D12PipelineState> corePSO;
ComPtr<ID3D12PipelineState> dbgWirePSO;
DescriptorHeap bmapsHeap;
D3D12PaintDevice paintDevice;

//TestField
CBObject cbo_t_glyph;
CBObject cbo_t_circ;
CBFrame cbf_root;
ResourceBlob rbo_glyph;
ResourceBlob rbo_circ;
ResourceBlob rbf_root;
CoreMesh* cm_t_glfs;
CoreMesh* cm_t_cir;
RLL::ISVGBuilder* sb;
RLL::IGeometryBuilder* gb;
UINT m4xMsaaQuality;
Vector2 cam_t = { 0,0 };
Vector cam_s = 1;
Vector2 cam_md = { -1,0 };

#pragma region util
bool PointInRect(Math3D::Vector2 pt, Vector4 r)
{
	if (pt.x < r.l) return false;
	if (pt.y < r.t) return false;
	if (pt.x > r.r) return false;
	if (pt.y > r.b) return false;
	return true;
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
struct DWM_COLORIZATION_PARAMS
{
	COLORREF clrColor;
	COLORREF clrAfterGlow;
	UINT nIntensity;
	UINT clrAfterGlowBalance;
	UINT clrBlurBalance;
	UINT clrGlassReflectionIntensity;
	BOOL fOpaque;
};
#include "win32whelper.h"
#pragma endregion
ComPtr<ID3DBlob> CompileShader(
	const wchar_t* fileName,
	const D3D_SHADER_MACRO* defines,
	const char* enteryPoint,
	const char* target)
{
	//若处于调试模式，则使用调试标志
	UINT compileFlags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if defined(DEBUG) || defined(_DEBUG)
	//用调试模式来编译着色器 | 指示编译器跳过优化阶段
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(fileName, //hlsl源文件名
		defines,	//高级选项，指定为空指针
		D3D_COMPILE_STANDARD_FILE_INCLUDE,	//高级选项，可以指定为空指针
		enteryPoint,	//着色器的入口点函数名
		target,		//指定所用着色器类型和版本的字符串
		compileFlags,	//指示对着色器断代码应当如何编译的标志
		0,	//高级选项
		&byteCode,	//编译好的字节码
		&errors);	//错误信息

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	//ThrowIfFailed(hr);

	return byteCode;
}
void LoadShader()
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
	(ARRLEN(rootParams), //根参数的数量
		rootParams, //根参数指针
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
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "PNB", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "UVTF", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	ComPtr<ID3DBlob> vbc = CompileShader(SHADER_CORE, nullptr, "VS", "vs_5_1");
	ComPtr<ID3DBlob> pbc = CompileShader(SHADER_CORE, nullptr, "PS", "ps_5_1");

	auto rs = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rs.CullMode = D3D12_CULL_MODE_NONE;
	//rs.FillMode = D3D12_FILL_MODE_WIREFRAME;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { inputLayoutDesc.data(), (UINT)inputLayoutDesc.size() };
	psoDesc.pRootSignature = coreSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(vbc->GetBufferPointer()), vbc->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(pbc->GetBufferPointer()), pbc->GetBufferSize() };
	psoDesc.RasterizerState = rs;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,全部采样，没有遮罩
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//归一化的无符号整型
	//psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;	//归一化的无符号整型
	//psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = MSAA ? 4 : 1;	//不使用4XMSAA
	psoDesc.SampleDesc.Quality = MSAA ? m4xMsaaQuality - 1 : 0;	////不使用4XMSAA

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

	bmapsHeap.Create(d3dDevice.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2048, true);
	///dbg
	pbc = CompileShader(L"shader\\dbgWire.hlsl", nullptr, "PS", "ps_5_1");
	vbc = CompileShader(L"shader\\dbgWire.hlsl", nullptr, "VS", "vs_5_1");
	rs.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.PS = { reinterpret_cast<BYTE*>(pbc->GetBufferPointer()), pbc->GetBufferSize() };
	psoDesc.VS = { reinterpret_cast<BYTE*>(vbc->GetBufferPointer()), vbc->GetBufferSize() };
	psoDesc.RasterizerState = rs;
	SUCCESS(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&dbgWirePSO)));

}
#pragma region api
//RLL
wchar_t* RLL::GetLocale()
{
	return locale;
}
Math3D::Vector2 RLL::GetScale()
{
	return dpiScaleFactor;
}
RLL::IFrame* RLL::CreateFrame(IFrame* parent, Math3D::Vector2 size, Math3D::Vector2 pos)
{
	return new Frame(reinterpret_cast<Frame*>(parent), size, pos);
}
int gFlags;
void RLL::Initiate(int flags)
{
	gFlags = flags;
	HRESULT hr;
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

	GetUserDefaultLocaleName(locale, sizeof(locale));

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

	hr = DCompositionCreateDevice3(nullptr, IID_PPV_ARGS(&dCompDevice));
	SUCCESS(hr);
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	SUCCESS(d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	LoadShader();
	//SUCCESS(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
}
int RLL::GetFlags()
{
	return gFlags;
}

//DEV
ID3D12Device5* GetD3DDevice()
{
	return d3dDevice.Get();
}
IDCompositionDevice* GetDCompDevice()
{
	return dCompDevice.Get();
}
IDXGIFactory5* GetDXGIFactory()
{
	return dxgiFactory.Get();
}
#pragma endregion

LRESULT wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT result = 0;
	if (msg == WM_NCCREATE) {
		auto userdata = reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams;
		// store window instance pointer in window user data
		::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
	}
	if (auto window_ptr = reinterpret_cast<Frame*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA)))
	{
		auto& frame = *window_ptr;
		result = frame.WndProc(hwnd, msg, wp, lp);
	}
	return result;
}
LRESULT Frame::NextProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	return DefWindowProc(hwnd, msg, wp, lp);
}
Frame::Frame(Frame* parent, Vector2 size, Vector2 pos)
{
	HRESULT hr;
	HWND hWnd_parent = NULL;
	if (parent)
		hWnd_parent = parent->hWnd;
	SIZE scaledsz{ (LONG)(size.x * dpiScaleFactor.x) ,(LONG)(size.y * dpiScaleFactor.x) };
	POINT posi{ (LONG)pos.x,(LONG)pos.y };
	hWnd = create_window(hWnd_parent, wndProc, this, scaledsz, posi, extStyles | WS_EX_NOREDIRECTIONBITMAP);

	//CreateCommandThings
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	SUCCESS(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	SUCCESS(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));//&cmdAllocator等价于cmdAllocator.GetAddressOf
	SUCCESS(d3dDevice->CreateCommandList(0, //掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT, //命令列表类型
		cmdAllocator.Get(),	//命令分配器接口指针
		nullptr,	//流水线状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&cmdList)));	//返回创建的命令列表


	//CreateSwapChain
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	RECT r;
	GetClientRect(hWnd, &r);
	SetViewRect(r);

	scDesc.Width = r.right - r.left;
	scDesc.Height = r.bottom - r.top;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = 2;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.Stereo = false;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
	hr = (dxgiFactory->CreateSwapChainForComposition(cmdQueue.Get(), &scDesc, nullptr, &swapChain));

	CreateRenderTargets();

	SUCCESS(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gFence)));

	SUCCESS(dCompDevice->CreateTargetForHwnd(hWnd, true, &dcompTarget));
	SUCCESS(dCompDevice->CreateVisual(&dcompVisual));
	SUCCESS(dcompVisual->SetContent(swapChain.Get()));
	SUCCESS(dcompTarget->SetRoot(dcompVisual.Get()));
	SUCCESS(dCompDevice->Commit());

	paintDevice.d3dDevice = d3dDevice.Get();
	paintDevice.coreSignature = coreSignature.Get();
	paintDevice.corePSO = corePSO.Get();
	paintDevice.cmdList = cmdList.Get();


	//TestField.
	auto ffact = RLL::CreateFontFactory(&paintDevice);
	auto fc_tms = ffact->LoadFromFile("c:/windows/fonts/times.ttf");
	auto fc_rob = ffact->LoadFromFile("D:/Download/Roboto/Roboto-Regular.ttf");
	auto fc_dsm = ffact->LoadFromFile("C:/Users/Tibyl/AppData/Local/Microsoft/Windows/Fonts/DroidSansMono.ttf");
	auto fc_msyh = ffact->LoadFromFile("c:/windows/fonts/msyh.ttc");
	auto fc_emj = ffact->LoadFromFile("c:/windows/fonts/seguiemj.ttf");
	auto fc_khm = ffact->LoadFromFile("F:/libs/harfbuzz-5.3.1/test/subset/data/fonts/Khmer.ttf");
	auto fc_sun = ffact->LoadFromFile("c:/windows/fonts/simsun.ttc");
	gb = paintDevice.CreateGeometryBuilder();
	auto go = fc_msyh->GetGlyph(U'默');
	gb->Reset();
	auto go2 = (D3D12Geometry*)fc_msyh->GetPlainGlyph(U'字');

	gb->Reset();
	auto go1 = (D3D12Geometry*)fc_msyh->GetPlainGlyph(U'M');

	auto tmat = Matrix4x4::Scaling(1.234) * Matrix4x4::Translation({ 200,300 }) * Matrix4x4::Rotation(1, 0, 0);
	RLL::ColorGradient cg;
	cg.colors[0] = { 1,1,0,0 };
	cg.colors[1] = { 0.3,1,0.3,0 };
	cg.positions[1] = 1;
	auto br_yg = paintDevice.CreateRadialBrush({ 0,0 }, 1.0, &cg);

	cg.colors[0] = { 1,0.87,0,0 };
	cg.colors[1] = { 1,0.27,0.3,0 };
	auto br_yg_tex = paintDevice.CreateRadialBrush({ 0.5,0.5 }, 1, &cg);
	auto br_rec = paintDevice.CreateRadialBrush({ 0,0 }, 1, &cg);


	sb = paintDevice.CreateSVGBuilder();
	auto svg_sb = (D3D12SVG*)hb_test(fc_rob, sb, L"Latin series AW AV VA WA fi fii fj fl ft ff. 中字默。abc");
	sb->Reset();
	auto svg_sbt = (D3D12SVG*)hb_test(fc_tms, sb, L"Latin series AW AV VA WA fi fii fj fl ft ff. 中字默。abc");
	sb->Reset();
	auto svg_sbk = (D3D12SVG*)hb_test(fc_khm, sb, L"ញុំបានមើ khmer");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_sbc = (D3D12SVG*)hb_test(fc_msyh, sb, L"中字默一川七八毫");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_sbcs = (D3D12SVG*)hb_test(fc_sun, sb, L"中字默一川七八毫");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_dsm = (D3D12SVG*)hb_test(fc_dsm, sb, L"Latin series TrueType DroidSansMono.");
	sb->Reset();
	auto svg_heb = (D3D12SVG*)hb_test(fc_tms, sb, L"Hebrew: אָלֶף־בֵּית עִבְרִי, Arabic: اللغة العربية");
	sb->Reset();
	auto svg_emj = (D3D12SVG*)hb_test(fc_emj, sb, L"🧑🧑🏻🧑🏼🧑🏽🧑🏾🧑🏿");

	sb->Reset();
	sb->Push(go);
	sb->Push(go1, br_yg, &Matrix4x4::Translation(Vector3(1, 0, 0) * 1.2));
	sb->Push(go2, br_yg_tex, &Matrix4x4::Translation(Vector3(0, 1, 0) * 1.2));
	auto glf_clr = fc_emj->GetGlyph(U'🥳');
	sb->Push(glf_clr, &Matrix4x4::Translation(Vector3(2, 0, 0) * 1.2));
	glf_clr = fc_emj->GetGlyph(U'😍');
	sb->Push(glf_clr, &Matrix4x4::Translation(Vector3(3, 0, 0) * 1.2));
	sb->Push(svg_sb, &Matrix4x4::Translation(Vector3(0, 2, 0) * 1.2));
	sb->Push(svg_sbt, &Matrix4x4::Translation(Vector3(0, 3, 0) * 1.2));
	sb->Push(svg_sbk, &Matrix4x4::Translation(Vector3(0, 4, 0) * 1.2));
	sb->Push(svg_sbc, &Matrix4x4::Translation(Vector3(0, 6, 0) * 1.2));
	sb->Push(svg_sbcs, &Matrix4x4::Translation(Vector3(0, 7, 0) * 1.2));
	sb->Push(svg_dsm, &Matrix4x4::Translation(Vector3(0, 8, 0) * 1.2));
	sb->Push(svg_heb, &Matrix4x4::Translation(Vector3(0, 9, 0) * 1.2));
	sb->Push(svg_emj, &Matrix4x4::Translation(Vector3(0, 10, 0) * 1.2));
	auto sv = (D3D12SVG*)sb->Commit();
	cm_t_glfs = sv->mesh;
	//cm_t_glfs = svg_sb->mesh;
	gb->Reset();
	gb->Ellipse({ 0,0 }, { 1,1.3 });
	gb->Ellipse({ 0.8,0 }, { 1 * 0.9,1.3 * .9 }, true);
	auto go_circle = gb->Fill();

	sb->Reset();
	sb->Push(go_circle, br_yg, &(Matrix4x4::Scaling(60) * Matrix4x4::Translation({ -100,-70 })));
	gb->Reset();
	gb->Ellipse({ 0,0 }, { 1,1 });
	gb->Ellipse({ -0.7,0 }, { 0.9,0.9 }, true);
	go_circle = gb->Fill();
	gb->Reset();
	gb->Rectangle({ -1,-1 }, { 1,1 });
	auto go_geo = gb->Fill();

	sb->Push(go_circle, br_yg, &(Matrix4x4::Scaling(60) * Matrix4x4::Translation({ -100,70 })));
	sb->Push(go_geo, br_rec, &(Matrix4x4::Scaling(40) * Matrix4x4::Translation({ -300,100 }) * Matrix4x4::Rotation(Quaternion::RollPitchYaw(0, 0, 1))));

	gb->Reset();
	gb->Triangle({ -1,0 }, { 1,0 }, { 1.5,1.26 });
	go_geo = gb->Fill();

	sb->Push(go_geo, br_yg_tex, &(Matrix4x4::Scaling(50) * Matrix4x4::Translation({ 50,-100 })));

	gb->Reset();
	gb->RoundRectangle({ -1,0 }, { 1,1 }, { 0.1,0.1 });
	gb->Ellipse({ 0,0.5 }, { 0.3,0.3 }, true);
	gb->RoundRectangle({ -0.5,0.1 }, { 0.5,0.5 }, { 0.05,0.1 }, true);
	go_geo = gb->Fill();
	sb->Push(go_geo, nullptr, &(Matrix4x4::Scaling(90) * Matrix4x4::Translation({ 50,-250 })));

	gb->Reset();
	gb->Begin({ 0,0 });
	gb->ArcTo({ 0,0 }, { 1.3,1 }, 0, Deg2Rad(138));
	gb->End(true);
	go_geo = gb->Fill();
	sb->Push(go_geo, br_yg, &(Matrix4x4::Scaling(50) * Matrix4x4::Translation({ -200,200 })));

	gb->Reset();
	gb->Begin({ 0,0 });
	//gb->SetBackgroundTransform(&Matrix4x4::Scaling({ 1.f/200, 1.f/200, 1 }));
	gb->QuadraticTo({ 250,-100 }, { 200,0 });
	gb->LineTo({ 300,50 });
	//gb->MoveTo({ 200,0 });
	gb->End(true);

	gb->Begin({ 0,50 });
	gb->CubicTo({ -250,150 }, { 50,150 }, { -200,50 });
	gb->End(false);

	//below convhull bug
	//gb->Begin({ 50,0 });
	//gb->CubicTo({ -250,150 }, { 50,150 }, { -200,50 });
	//gb->End(false); //bug test end

	go_geo = gb->Stroke(5, nullptr, &Matrix4x4::Scaling({ 1/200.f, 1/200.f, 1 }));
	//go_geo = gb->Stroke(5, nullptr);
	//go_geo = gb->Fill(&Matrix4x4::Scaling({ 1.f / 200, 1.f / 200, 1 }));
	sb->Push(go_geo, br_yg_tex, &(Matrix4x4::Translation({ -180,-150 })));

	cm_t_cir = ((D3D12SVG*)sb->Commit())->mesh;

	paintDevice.CommitChange();

	rbo_glyph = paintDevice.CreateUploadBuffer(sizeof(CBObject));
	rbo_circ = paintDevice.CreateUploadBuffer(sizeof(CBObject));
	rbf_root = paintDevice.CreateUploadBuffer(sizeof(CBFrame));
	cbo_t_glyph.lerpTime = 0;
	cbo_t_glyph.sampleType = 0;
	cbo_t_glyph.objToWorld = Matrix4x4::Scaling({ 13 * 96 / 72 }); // px = pt * dpi / 72
	cbo_t_circ.lerpTime = 0;
	cbo_t_circ.sampleType = 0;
	cbo_t_circ.objToWorld = Matrix4x4::Scaling(1); // px = pt * dpi / 72

	cbf_root.dColor = { 0,0,0,0 };
	cbf_root.gWorldViewProj = Matrix4x4::LookTo({ 0,0,-500 }, { 0,0,1 }) *
		Matrix4x4::Scaling({ 1,-1,1 }) *
		Matrix4x4::Orthographic(800, 600, 1, 1000);
	cbf_root.vwh = { 800,600 };

	rbo_glyph.Sync(cbo_t_glyph);
	rbo_circ.Sync(cbo_t_circ);
	rbf_root.Sync(cbf_root);
	cmdList->Close();	//重置命令列表前必须将其关闭
	ID3D12CommandList* commandLists[] = { cmdList.Get() };//声明并定义命令列表数组
	cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);//将命令从命令列表传至命令队列

	FlushCommandQueue();

}
LRESULT Frame::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{

	switch (msg)
	{
	case WM_CREATE:
		//std::cout << "WMC\n";
		hWnd = hwnd;
		break;
	case WM_NCCALCSIZE:
	{
		auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
		adjust_maximized_client_rect(hwnd, params.rgrc[0]);
		return 0;
	}
	case WM_NCHITTEST:
		// When we have no border or title bar, we need to perform our
		// own hit testing to allow resizing and moving.
		return hit_test(hwnd, POINT{
			GET_X_LPARAM(lp),
			GET_Y_LPARAM(lp)
			}, false, true);//!!!!!! TO MODIFY
	case WM_NCACTIVATE:
		if (!composition_enabled()) {
			// Prevents window frame reappearing on window activation
			// in "basic" theme, where no aero shadow is present.
			return 1;
		}
		break;
	case WM_PAINT:
	{
		//OutputDebugString(L"Paint\n");
		SUCCESS(cmdAllocator->Reset());//重复使用记录命令的相关内存
		SUCCESS(cmdList->Reset(cmdAllocator.Get(), nullptr));//复用命令列表及其内存
		if (MSAA)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(maBuffer[currentBuffer].Get(),//转换资源为后台缓冲区资源
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));//从呈现到渲染目标转换
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[currentBuffer].Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));//从渲染目标到呈现
		}
		else {
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[currentBuffer].Get(),
				D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));//从渲染目标到呈现
		}
		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &scissorRect);
		FLOAT color[] = { 0.000000000f, 0.545098066f, 0.000000000f, 1.000000000f };
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBuffer, rtvDescriptorSize);
		cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::White, 0, &scissorRect);//清除RT背景色为暗红，并且不设置裁剪矩形
		cmdList->OMSetRenderTargets(1,//待绑定的RTV数量
			&rtvHandle,	//指向RTV数组的指针
			true,	//RTV对象在堆内存中是连续存放的
			nullptr);	//指向DSV的指针
		cmdList->SetGraphicsRootSignature(coreSignature.Get());
		cmdList->SetPipelineState(corePSO.Get());
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->SetGraphicsRootConstantBufferView(0, rbo_glyph.gpuBuffer->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(1, rbf_root.gpuBuffer->GetGPUVirtualAddress());
		ID3D12DescriptorHeap* heaps[] = { bmapsHeap.Get() };
		cmdList->SetDescriptorHeaps(1, heaps);
		cmdList->SetGraphicsRootDescriptorTable(2, bmapsHeap.hGPU(0));
		cmdList->SetGraphicsRootShaderResourceView(3, paintDevice.gpuPath->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootShaderResourceView(4, paintDevice.gpuCurve->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootShaderResourceView(5, paintDevice.gpuBrush->GetGPUVirtualAddress());

		cmdList->IASetIndexBuffer(&cm_t_glfs->ibv);
		cmdList->IASetVertexBuffers(0, 4, cm_t_glfs->vbv);

		cmdList->DrawIndexedInstanced(cm_t_glfs->idxCount, 1, 0, 0, 0);

		cmdList->SetGraphicsRootConstantBufferView(0, rbo_circ.gpuBuffer->GetGPUVirtualAddress());
		cmdList->IASetIndexBuffer(&cm_t_cir->ibv);
		cmdList->IASetVertexBuffers(0, 4, cm_t_cir->vbv);

		//cmdList->SetPipelineState(dbgWirePSO.Get());
		//cmdList->DrawIndexedInstanced(cm_t_cir->idxCount, 1, 0, 0, 0);

		//cmdList->SetPipelineState(corePSO.Get());
		cmdList->DrawIndexedInstanced(cm_t_cir->idxCount, 1, 0, 0, 0);
		//cmdList->SetPipelineState(dbgWirePSO.Get());
		//cmdList->DrawIndexedInstanced(cm_t_cir->idxCount, 1, 0, 0, 0);

		//cmdList->DrawIndexedInstanced(cm_t_cir->idxCount, 1, 0, 0, 0);

		if (MSAA)
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(maBuffer[currentBuffer].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));//从渲染目标到呈现
			cmdList->ResolveSubresource(scBuffer[currentBuffer].Get(), 0, maBuffer[currentBuffer].Get(), 0, DXGI_FORMAT_R8G8B8A8_UNORM);
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[currentBuffer].Get(),
				D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));//从渲染目标到呈现
		}
		else
		{
			cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scBuffer[currentBuffer].Get(),
				D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));//从渲染目标到呈现
		}

		//完成命令的记录关闭命令列表
		SUCCESS(cmdList->Close());
		ID3D12CommandList* commandLists[] = { cmdList.Get() };//声明并定义命令列表数组
		cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);//将命令从命令列表传至命令队列
		SUCCESS(swapChain->Present(0, 0));
		currentBuffer = (currentBuffer + 1) % 2;
		FlushCommandQueue();
		paintDevice.PostFlush();
		SUCCESS(dCompDevice->Commit());
		break;
	}
	case WM_SIZE:
		if (swapChain.Get())
		{
			AquireWindowRect();
			ResizeView();
		}
		break;
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	{

		//break;
	}
	case WM_LBUTTONUP:
	case WM_LBUTTONDOWN:
	{
		//WndProc(hwnd, WM_MOUSEMOVE, wp, lp);
	}
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		//case WM_SETCURSOR:
	{
		//WndProc(hwnd, WM_MOUSEMOVE, wp, lp);
		Vector2 mouse{ GET_X_LPARAM(lp) / dpiScaleFactor.x,GET_Y_LPARAM(lp) / dpiScaleFactor.y };
		POINT cm;
		cm.x = mouse.x * dpiScaleFactor.x;
		cm.y = mouse.y * dpiScaleFactor.y;
		if (msg == WM_LBUTTONDOWN)
		{
			cam_md = { (float)cm.x,(float)cm.y };
		}
		else if (msg == WM_LBUTTONUP)
		{
			cam_md.x = -1;
		}
		else if (msg == WM_MOUSEMOVE && cam_md.x != -1)
		{
			Vector2 mp(cm.x, cm.y);
			auto d = mp - cam_md;
			cam_t = cam_t + Vector2(d.x, -d.y) * 1.01;
			cam_md = mp;

			cbf_root.gWorldViewProj =
				Matrix4x4::TRS({ cam_t.x,cam_t.y,0 }, Quaternion::identity(), { cam_s,-cam_s,cam_s }) *
				Matrix4x4::LookTo({ 0,0,-500 }, { 0,0,1 }) * Matrix4x4::Orthographic(viewport.Width, viewport.Height, 1, 1000)
				;
			rbf_root.Sync(cbf_root);
			PostMessage(hwnd, WM_PAINT, 0, 0);

		}
		//std::cout << cam_md.x << "\t" << cam_s << "\t" << cam_t.x << ", " << cam_t.y << std::endl;
		LRESULT ret = 0;
		return ret;
	}
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	{
		RECT wr;
		GetWindowRect(hWnd, &wr);
		POINT mouse{ (GET_X_LPARAM(lp) - wr.left) ,(GET_Y_LPARAM(lp) - wr.top) };
		//WndProc(hwnd, msg + 0x160, wp, mouse.x | (mouse.y << 16));
		//if (msg == WM_NCLBUTTONDOWN) frame.flags.nclbd = true;
		break;
	}
	case WM_SYSCOMMAND:
	{
		if ((wp & SC_MAXIMIZE) || (wp & SC_RESTORE))
		{
			PostMessageW(hwnd, WM_EXITSIZEMOVE, 0, 0);
			if ((wp & SC_MAXIMIZE) == SC_MAXIMIZE)
			{
				//frame.flags.maximized = true;
				//frame.View::Position();
			}
			if ((wp & SC_RESTORE) == SC_RESTORE)
			{
				//frame.flags.maximized = false;
				//frame.View::Position();
			}
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		auto dta = GET_WHEEL_DELTA_WPARAM(wp);
		cam_s *= (1 + (float)dta / 120 * 0.1f);
		cbf_root.gWorldViewProj =
			Matrix4x4::TRS({ cam_t.x,cam_t.y,0 }, Quaternion::identity(), { cam_s,-cam_s,cam_s }) *
			Matrix4x4::LookTo({ 0,0,-500 }, { 0,0,1 }) * Matrix4x4::Orthographic(800, 600, 1, 1000)
			;
		rbf_root.Sync(cbf_root);

		PostMessage(hwnd, WM_PAINT, 0, 0);
	}
	case WM_CHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_IME_COMPOSITION:
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	{
		break;
	}
	case WM_DESTROY:
	{
		break;
	}
	case WM_KILLFOCUS:
	{

		break;
	}
	case WM_MOUSELEAVE:
	{
		break;
	}
	case WM_SETCURSOR:
	{

		break;
	}
	case WM_MOVE:
	case WM_EXITSIZEMOVE:
		int x = GET_X_LPARAM(lp);
		int y = GET_Y_LPARAM(lp);

		break;

	}
	return NextProc(hwnd, msg, wp, lp);
}
void Frame::ResizeView()
{
	for (int i = 0; i < 2; i++)
	{
		scBuffer[i].Reset();
	}
	SUCCESS(swapChain->ResizeBuffers(2, viewport.Width, viewport.Height, DXGI_FORMAT_UNKNOWN, 0));
	CreateRenderTargets();
}
void Frame::Show()
{
	set_borderless_shadow(hWnd, true);
	ShowWindow(hWnd, SW_SHOW);
}
void Frame::Run()
{
	MSG msg;
	HWND phWnd = 0;
	//if (modeled && parent)
	//{
	//	phWnd = ((Frame*)parent)->GetNative();
	//	EnableWindow(phWnd, false);
	//}
	while (::GetMessageW(&msg, 0, 0, 0) == TRUE)
	{
		//MainThreadDispatch();
		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}
	//if (modeled && phWnd)
	//	EnableWindow(phWnd, true), SetForegroundWindow(phWnd);
}
void Frame::SetViewRect(RECT& rc)
{
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = rc.right - rc.left;
	scissorRect.bottom = rc.bottom - rc.top;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = rc.right - rc.left;
	viewport.Height = rc.bottom - rc.top;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
}
void Frame::FlushCommandQueue()
{
	cFence++;	//CPU传完命令并关闭后，将当前围栏值+1
	cmdQueue->Signal(gFence.Get(), cFence);	//当GPU处理完CPU传入的命令后，将fence接口中的围栏值+1，即fence->GetCompletedValue()+1
	if (gFence->GetCompletedValue() < cFence)	//如果小于，说明GPU没有处理完所有命令
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");	//创建事件
		gFence->SetEventOnCompletion(cFence, eventHandle);//当围栏达到mCurrentFence值（即执行到Signal（）指令修改了围栏值）时触发的eventHandle事件
		WaitForSingleObject(eventHandle, INFINITE);//等待GPU命中围栏，激发事件（阻塞当前线程直到事件触发，注意此Enent需先设置再等待，
							   //如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
		CloseHandle(eventHandle);
	}
}
Math3D::Vector4 Frame::AquireWindowRect()
{
	RECT wr;
	GetWindowRect(hWnd, &wr);
	SetViewRect(wr);
	return { (float)wr.left,(float)wr.top,(float)wr.right,(float)wr.bottom };
}
void Frame::CreateRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;
	SUCCESS(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));
	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
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
		if (MSAA)
		{
			SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), //创建上传堆类型的堆
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, viewport.Width, viewport.Height,
					1, 1, 4, m4xMsaaQuality - 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),//变体的构造函数，传入byteSize，其他均为默认值，简化书写
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
				&clearValue,	//不是深度模板资源，不用指定优化值
				IID_PPV_ARGS(&maBuffer[i])));

			//创建RTV
			d3dDevice->CreateRenderTargetView(maBuffer[i].Get(),
				nullptr,	//在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
				rtvHeapHandle);	//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
			//偏移到描述符堆中的下一个缓冲区
		}
		else
		{
			d3dDevice->CreateRenderTargetView(scBuffer[i].Get(),
				nullptr,	//在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
				rtvHeapHandle);	//描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）

		}
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
	currentBuffer = 0;
}


