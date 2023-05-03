#include "win32internal.h"
#include "win32paint_d12.h"
#include "DirectXColors.h"
#include "fi_ft.h"
#include "rlltest.h"
#include "TextInterfaces.h"

using namespace Math3D;
using namespace RLL;
#define ARRLEN(x) (sizeof(x)/sizeof(*x))
#define SHADER_CORE L"shader\\core.hlsl"
#define MSAA 0

Vector2 dpiScaleFactor = { 1,1 };
wchar_t locale[LOCALE_NAME_MAX_LENGTH];
RLL::IPaintDevice* paintDevice;

//TestField
RLL::ISVG* cm_t_glfs;
RLL::ISVG* cm_t_cir;
RLL::ISVG* cm_t_tly;
RLL::ISVGBuilder* sb;
RLL::IGeometryBuilder* gb;
UINT m4xMsaaQuality;
Vector2 cam_t = { 0,0 };
Vector cam_s = 1;
Vector2 cam_md = { -1,0 };
Matrix4x4 vtf;

#pragma region util
bool PointInRect(Math3D::Vector2 pt, Vector4 r)
{
	if (pt.x < r.l) return false;
	if (pt.y < r.t) return false;
	if (pt.x > r.r) return false;
	if (pt.y > r.b) return false;
	return true;
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

}
int RLL::GetFlags()
{
	return gFlags;
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

	AquireWindowRect();

	//TestField.
	paintDevice = RLL::CreatePaintDevice();
	paintCtx = (D3D12FramePaintContext*)paintDevice->CreateContextForFrame(this);

	auto ffact = RLL::CreateFontFactory(paintDevice);
	auto fc_tms = ffact->LoadFromFile("c:/windows/fonts/times.ttf");
	auto fc_rob = ffact->LoadFromFile("D:/Download/Roboto/Roboto-Regular.ttf");
	auto fc_dsm = ffact->LoadFromFile("C:/Users/Tibyl/AppData/Local/Microsoft/Windows/Fonts/DroidSansMono.ttf");
	auto fc_msyh = ffact->LoadFromFile("c:/windows/fonts/msyh.ttc");
	auto fc_emj = ffact->LoadFromFile("c:/windows/fonts/seguiemj.ttf");
	auto fc_khm = ffact->LoadFromFile("F:/libs/harfbuzz-5.3.1/test/subset/data/fonts/Khmer.ttf");
	auto fc_sun = ffact->LoadFromFile("c:/windows/fonts/simsun.ttc");
	auto fc_fsun = ffact->LoadFromFile("c:/windows/fonts/simfang.TTF");
	auto fc_arial = ffact->LoadFromFile("c:/windows/fonts/arial.ttf");
	auto fs_df = new IFontStack();
	fs_df->Push(fc_emj);
	fs_df->Push(fc_arial);
	fs_df->Push(fc_msyh);
	gb = paintDevice->CreateGeometryBuilder();
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
	auto br_yg = paintDevice->CreateRadialBrush({ 0,0 }, 1.0, &cg);

	cg.colors[0] = { 1,0.87,0,0 };
	cg.colors[1] = { 1,0.27,0.3,0 };
	auto br_dps = paintDevice->CreateRadialBrush({ 0,0 }, 1, &cg);
	br_dps->Release();
	auto br_yg_tex = paintDevice->CreateRadialBrush({ 0.5,0.5 }, 1, &cg);
	auto br_rec = paintDevice->CreateRadialBrush({ 0,0 }, 1, &cg);

	auto tl = new TextLayout(L"\\אָלֶף־בֵּית 🧑🏿🥳עִבְרִי/ ltr🥵rtl \\اللغة العربية/TextLayout文本布局。Français Abc defgh a123c 1.234ff.\
 nbsp left. done? emj😎🧑🏿🧑🏿🥳 ", { 300,50 }, fs_df);
	//tl->Break();//TextLayout文本布局。Abc defgh a123c 1.234ff. nbsp left. done? emj😎🧑🏿🧑🏿
	//tl->Metrics();
	cm_t_tly = tl->Commit(paintDevice);
	wchar_t* khm = L"אָלֶף־בֵּית";

	sb = paintDevice->CreateSVGBuilder();
	auto svg_sb = (D3D12SVG*)hb_test(fc_rob, sb, L"Roboto Regular. AWAVfifiifjflft. 中字默。abcDT");
	sb->Reset();
	auto svg_sbt = (D3D12SVG*)hb_test(fc_tms, sb, L"Times New Roman. AWAVfifiifjflft. 中字默。abcDT nbsp 1 km/h");
	sb->Reset();
	auto svg_sbk = (D3D12SVG*)hb_test(fc_khm, sb, L"ញុំបានមើ khmer");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_sbc = (D3D12SVG*)hb_test(fc_msyh, sb, L"雅黑。这次职业生涯规划生涯人物访谈，中字默一（十）川(abc)七abc八123abc毫");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_sbcs = (D3D12SVG*)hb_test(fc_sun, sb, L"宋体。这次职业生涯规划生涯人物访谈，中字默一（十）川(abc)七abc八123abc毫");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_sbcfs = (D3D12SVG*)hb_test(fc_fsun, sb, L"仿宋。这次职业生涯规划生涯人物访谈，中字默一（十）川(abc)七abc八123abc毫");//ញុំបានមើ khmer
	sb->Reset();
	auto svg_dsm = (D3D12SVG*)hb_test(fc_dsm, sb, L"Latin series TrueType DroidSansMono.");
	sb->Reset();
	auto svg_heb = (D3D12SVG*)hb_test(fc_arial, sb, L"Hebrew: אָלֶף־בֵּית עִבְרִי, Arabic: اللغة العربية, All 12pt");
	sb->Reset();
	auto svg_emj = (D3D12SVG*)hb_test(fc_emj, sb, L"🧑🧑🏻🧑🏼🧑🏽🧑🏾🧑🏿🥵😰");

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
	sb->Push(svg_sbcfs, &Matrix4x4::Translation(Vector3(0, 8, 0) * 1.2));
	sb->Push(svg_dsm, &Matrix4x4::Translation(Vector3(0, 9, 0) * 1.2));
	sb->Push(svg_heb, &Matrix4x4::Translation(Vector3(0, 10, 0) * 1.2));
	sb->Push(svg_emj, &Matrix4x4::Translation(Vector3(0, 11, 0) * 1.2));
	cm_t_glfs = sb->Commit();
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
	sb->Push(go_geo, nullptr, &(Matrix4x4::Scaling(90) * Matrix4x4::Translation({ -100,250 })));

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

	go_geo = gb->Stroke(5, nullptr, &Matrix4x4::Scaling({ 1 / 200.f, 1 / 200.f, 1 }));
	//go_geo = gb->Stroke(5, nullptr);
	//go_geo = gb->Fill(&Matrix4x4::Scaling({ 1.f / 200, 1.f / 200, 1 }));
	sb->Push(go_geo, br_yg_tex, &(Matrix4x4::Translation({ -180,-150 })));
	//sb->Push(svg_tly);
	cm_t_cir = sb->Commit();
	paintCtx->Flush();
	//rbo_glyph = paintDevice->CreateUploadBuffer(sizeof(CBObject));
	//rbo_circ = paintDevice->CreateUploadBuffer(sizeof(CBObject));
	//rbf_root = paintDevice->CreateUploadBuffer(sizeof(CBFrame));

	//cbo_t_glyph.lerpTime = 0;
	//cbo_t_glyph.sampleType = 0;
	//cbo_t_glyph.objToWorld = Matrix4x4::Scaling({ 13 * 96 / 72 }); // px = pt * dpi / 72
	//cbo_t_circ.lerpTime = 0;
	//cbo_t_circ.sampleType = 0;
	//cbo_t_circ.objToWorld = Matrix4x4::Scaling(1); // px = pt * dpi / 72

	//cbf_root.dColor = { 0,0,0,0 };
	//cbf_root.gWorldViewProj = Matrix4x4::LookTo({ 0,0,-500 }, { 0,0,1 }) *
	//	Matrix4x4::Scaling({ 1,-1,1 }) *
	//	Matrix4x4::Orthographic(800, 600, 1, 1000);
	//cbf_root.vwh = { 800,600 };

	//rbo_glyph.Sync(cbo_t_glyph);
	//rbo_circ.Sync(cbo_t_circ);
	//rbf_root.Sync(cbf_root);
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
		paintCtx->BeginDraw();
		paintCtx->Clear({ 1,1,1, 1.f });
		paintCtx->SetTransform(Matrix4x4::Scaling({ 12 * 96 / 72 }) * vtf);
		paintCtx->DrawSVG(cm_t_glfs);
		paintCtx->SetTransform(vtf);
		paintCtx->DrawSVG(cm_t_cir);
		paintCtx->SetTransform(Matrix4x4::Translation({ 0,-200,0 }) * vtf);
		paintCtx->DrawSVG(cm_t_tly);
		paintCtx->EndDraw();
		//SUCCESS(dCompDevice->Commit());
		break;
	}
	case WM_SIZE:
		if (paintDevice)
		{
			AquireWindowRect();
			paintCtx->ResizeView();
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
			cam_t = cam_t + Vector2(d.x, d.y) * 1.01;
			cam_md = mp;

			vtf = Matrix4x4::TRS({ cam_t.x,cam_t.y,0 }, Quaternion::identity(), { cam_s,cam_s,cam_s });
			//rbf_root.Sync(cbf_root);
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
		vtf = Matrix4x4::TRS({ cam_t.x,cam_t.y,0 }, Quaternion::identity(), { cam_s,cam_s,cam_s });

		//rbf_root.Sync(cbf_root);

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
	viewRect.left = rc.left;
	viewRect.top = rc.top;
	viewRect.right = rc.right;
	viewRect.bottom = rc.bottom;
}

Math3D::Vector4 Frame::AquireWindowRect()
{
	RECT wr;
	GetWindowRect(hWnd, &wr);
	SetViewRect(wr);
	return { (float)wr.left,(float)wr.top,(float)wr.right,(float)wr.bottom };
}

