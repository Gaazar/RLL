#include "win32internal.h"

enum class Style : DWORD {
	windowed = WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
	aero_borderless = WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
	basic_borderless = WS_POPUP | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
	noanim_borderless = WS_SYSMENU
};

bool maximized(HWND hwnd)
{
	WINDOWPLACEMENT placement;
	if (!::GetWindowPlacement(hwnd, &placement)) {
		return false;
	}

	return placement.showCmd == SW_MAXIMIZE;
}

void adjust_maximized_client_rect(HWND window, RECT& rect)
{
	if (!maximized(window)) {
		return;
	}

	auto monitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONULL);
	if (!monitor) {
		return;
	}

	MONITORINFO monitor_info{};
	monitor_info.cbSize = sizeof(monitor_info);
	if (!::GetMonitorInfoW(monitor, &monitor_info)) {
		return;
	}

	// when maximized, make the client area fill just the monitor (without task bar) rect,
	// not the whole window rect which extends beyond the monitor.
	rect = monitor_info.rcWork;
}

LRESULT hit_test(HWND hWnd, POINT cursor, bool dragable, bool sizable)
{
	// identify borders and corners to allow resizing the window.
	// Note: On Windows 10, windows behave differently and
	// allow resizing outside the visible window frame.
	// This implementation does not replicate that behavior.
	const POINT border{
		::GetSystemMetrics(SM_CXFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER),
		::GetSystemMetrics(SM_CYFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER)
	};
	RECT window;
	if (!::GetWindowRect(hWnd, &window)) {
		return HTNOWHERE;
	}

	const auto drag = dragable ? HTCAPTION : HTCLIENT;

	enum region_mask {
		client = 0b0000,
		left = 0b0001,
		right = 0b0010,
		top = 0b0100,
		bottom = 0b1000,
	};

	const auto result =
		left * (cursor.x < (window.left + border.x)) |
		right * (cursor.x >= (window.right - border.x)) |
		top * (cursor.y < (window.top + border.y)) |
		bottom * (cursor.y >= (window.bottom - border.y));

	switch (result) {
	case left: return sizable ? HTLEFT : drag;
	case right: return sizable ? HTRIGHT : drag;
	case top: return sizable ? HTTOP : drag;
	case bottom: return sizable ? HTBOTTOM : drag;
	case top | left: return sizable ? HTTOPLEFT : drag;
	case top | right: return sizable ? HTTOPRIGHT : drag;
	case bottom | left: return sizable ? HTBOTTOMLEFT : drag;
	case bottom | right: return sizable ? HTBOTTOMRIGHT : drag;
	case client: return drag;
	default: return HTNOWHERE;
	}
}

bool composition_enabled() {
	BOOL composition_enabled = FALSE;
	bool success = ::DwmIsCompositionEnabled(&composition_enabled) == S_OK;
	return composition_enabled && success;
}
const wchar_t* window_class(WNDPROC wndproc)
{
	static const wchar_t* window_class_name = [&] {
		WNDCLASSEXW wcx{};
		wcx.cbSize = sizeof(wcx);
		wcx.style = CS_HREDRAW | CS_VREDRAW;
		wcx.hInstance = nullptr;
		wcx.lpfnWndProc = wndproc;
		wcx.lpszClassName = L"Frame";
		wcx.hbrBackground = CreateSolidBrush(0x00FF00FF);
		wcx.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
		const ATOM result = ::RegisterClassExW(&wcx);
		assert(result);

		return wcx.lpszClassName;
	}();
	return window_class_name;
}

HWND create_window(HWND parent, WNDPROC wndproc, void* userdata, SIZE size, POINT posi, DWORD extStyles = 0) {
	auto handle = CreateWindowExW(
		extStyles, window_class(wndproc), L"Frame",
		static_cast<DWORD>(Style::aero_borderless), posi.x, posi.y,
		size.cx, size.cy, parent, nullptr, nullptr, userdata
	);
	assert(handle);//failed to create window

	return handle;
}

void set_shadow(HWND handle, bool enabled) {
	if (composition_enabled()) {
		static const MARGINS shadow_state[2]{ { 0,0,0,0 },{ 0,0,1,0 } };
		::DwmExtendFrameIntoClientArea(handle, &shadow_state[enabled]);
	}
}
Style select_borderless_style()
{
	return composition_enabled() ? Style::aero_borderless : Style::basic_borderless;
}

void set_borderless(HWND hWnd, bool enabled) {
	Style new_style = (enabled) ? select_borderless_style() : Style::windowed;
	Style old_style = static_cast<Style>(::GetWindowLongPtrW(hWnd, GWL_STYLE));

	if (new_style != old_style) {

		::SetWindowLongPtrW(hWnd, GWL_STYLE, static_cast<LONG>(new_style));

		// when switching between borderless and windowed, restore appropriate shadow state
		set_shadow(hWnd, (new_style != Style::windowed));

		// redraw frame
		::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
		::ShowWindow(hWnd, SW_SHOW);
	}
}

void set_borderless_shadow(HWND hWnd, bool enabled) {
	set_shadow(hWnd, enabled);
}
