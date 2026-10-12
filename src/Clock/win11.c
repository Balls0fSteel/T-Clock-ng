/*-------------------------------------------
  win11.c - Windows 11 (XAML taskbar) support
  The Windows 11 taskbar has no classic clock window (TrayClockWClass) we could
  hook. Instead our DLL creates an "overlay" clock on top of the original one.
  This file runs a helper thread inside T-Clock that keeps the overlay informed:
  - where the original clock is (UI Automation, or window rect on classic taskbars)
  - which color the taskbar has (to cover the original clock)
  and opens Windows 11's calendar on request.
---------------------------------------------*/
#include "tclock.h"
#define COBJMACROS
#include <ole2.h>
#include <uiautomation.h>

// GUIDs defined locally as not every SDK/MinGW version ships them in a library
static const GUID kCLSID_CUIAutomation = {0xff48dba4,0x60ef,0x4201,{0xaa,0x87,0x54,0x10,0x3e,0xef,0x59,0x4e}};
static const GUID kIID_IUIAutomation = {0x30cbe57d,0xd9d0,0x452a,{0xab,0x13,0x7a,0xc5,0xac,0x48,0x25,0xee}};
static const GUID kIID_IUIAutomationInvokePattern = {0xfb377fbe,0x8ea6,0x46d5,{0x9c,0x73,0x64,0x99,0x64,0x2d,0x30,0x59}};
#define kUIA_InvokePatternId 10000

#define kUpdateInterval 2000 ///< ms between position/color updates
#define kSearchInterval 1000 ///< ms between searches while the clock wasn't found

static HANDLE m_thread = NULL;
static HANDLE m_wake = NULL; ///< auto-reset event to wake the helper thread
static volatile LONG m_stop = 0;
static volatile LONG m_calendar = 0; ///< pending calendar requests
static HWND m_clock = NULL; ///< our overlay clock

/// thread state
typedef struct Win11State {
	IUIAutomation* uia;
	IUIAutomationElement* element; ///< original clock (XAML)
	HWND taskbar;
	HWND classic; ///< original clock (classic taskbar, forced overlay)
	RECT rc;
	int rc_valid;
	COLORREF bg;
	int bg_valid;
	unsigned searches;
} Win11State;

/** \brief copies only the decimal digits of \p in to \p out */
static void DigitsOnly(const wchar_t* in, wchar_t* out, size_t out_size) {
	size_t len = 0;
	for(; *in && len+1 < out_size; ++in) {
		if(*in >= '0' && *in <= '9')
			out[len++] = *in;
	}
	out[len] = '\0';
}

/**
 * \brief scores how likely \p name is the name of the taskbar clock
 * \return 0 = no match, 1 = contains current time, 2 = time and year */
static int ClockNameScore(const wchar_t* name, const wchar_t* time_digits, const wchar_t* time_digits_prev, const wchar_t* year_digits) {
	wchar_t digits[128];
	if(!name || !*name)
		return 0;
	DigitsOnly(name, digits, _countof(digits));
	if(!wcsstr(digits, time_digits) && !wcsstr(digits, time_digits_prev))
		return 0;
	return (wcsstr(digits, year_digits) ? 2 : 1);
}

/** \brief formats the current (and previous) minute's time as digits, eg. "429" for 4:29 PM */
static void GetTimeDigits(wchar_t time_digits[16], wchar_t time_digits_prev[16], wchar_t year_digits[8]) {
	wchar_t buf[64];
	SYSTEMTIME st;
	GetLocalTime(&st);
	swprintf(year_digits, 8, FMT("%u"), st.wYear);
	GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buf, _countof(buf));
	DigitsOnly(buf, time_digits, 16);
	if(st.wMinute) {
		--st.wMinute;
	} else {
		st.wMinute = 59;
		st.wHour = (st.wHour ? st.wHour-1 : 23);
	}
	GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, buf, _countof(buf));
	DigitsOnly(buf, time_digits_prev, 16);
}

/** \brief checks if \p rc is a sane clock position within the taskbar's rect */
static int IsValidClockRect(const RECT* rc, const RECT* bar) {
	RECT tmp;
	if(rc->right - rc->left < 8 || rc->bottom - rc->top < 8)
		return 0;
	return IntersectRect(&tmp, rc, bar);
}

/** \brief finds the original clock element of the XAML taskbar via UI Automation */
static IUIAutomationElement* FindClockElement(Win11State* self) {
	IUIAutomationElement* root = NULL;
	IUIAutomationCondition* cond = NULL;
	IUIAutomationElementArray* all = NULL;
	IUIAutomationElement* best = NULL;
	int best_score = 0;
	LONG best_area = 0;
	RECT bar;
	wchar_t time_digits[16], time_digits_prev[16], year_digits[8];
	int count, i;

	if(!self->uia || FAILED(IUIAutomation_ElementFromHandle(self->uia, self->taskbar, &root)) || !root)
		return NULL;
	GetWindowRect(self->taskbar, &bar);
	GetTimeDigits(time_digits, time_digits_prev, year_digits);
	if(SUCCEEDED(IUIAutomation_CreateTrueCondition(self->uia, &cond))
	&& SUCCEEDED(IUIAutomationElement_FindAll(root, TreeScope_Descendants, cond, &all)) && all
	&& SUCCEEDED(IUIAutomationElementArray_get_Length(all, &count))) {
		for(i=0; i<count; ++i) {
			IUIAutomationElement* element = NULL;
			BSTR name = NULL;
			RECT rc;
			UIA_HWND native = 0;
			int score;
			if(FAILED(IUIAutomationElementArray_GetElement(all, i, &element)) || !element)
				continue;
			if(SUCCEEDED(IUIAutomationElement_get_CurrentNativeWindowHandle(element, &native)) && (HWND)native == m_clock) {
				IUIAutomationElement_Release(element); // that's us
				continue;
			}
			IUIAutomationElement_get_CurrentName(element, &name);
			score = ClockNameScore(name, time_digits, time_digits_prev, year_digits);
			SysFreeString(name);
			if(score && SUCCEEDED(IUIAutomationElement_get_CurrentBoundingRectangle(element, &rc)) && IsValidClockRect(&rc, &bar)) {
				LONG area = (rc.right-rc.left) * (rc.bottom-rc.top);
				// prefer full time+date matches, then the smallest element (the button, not a container)
				if(score > best_score || (score == best_score && area < best_area)) {
					if(best)
						IUIAutomationElement_Release(best);
					best = element;
					best_score = score;
					best_area = area;
					continue;
				}
			}
			IUIAutomationElement_Release(element);
		}
	}
	if(all)
		IUIAutomationElementArray_Release(all);
	if(cond)
		IUIAutomationCondition_Release(cond);
	IUIAutomationElement_Release(root);
	return best;
}

/** \brief finds a classic, visible clock (taskbars restored by 3rd party tools) */
static HWND FindClassicClock(HWND taskbar) {
	HWND tray = FindWindowEx(taskbar, NULL, L"TrayNotifyWnd", NULL);
	HWND clock = (tray ? FindWindowEx(tray, NULL, L"TrayClockWClass", NULL) : NULL);
	return (clock && IsWindowVisible(clock) ? clock : NULL);
}

/** \brief updates \c self->rc with the original clock's position
 * \return boolean, whether the clock was found */
static int LocateClock(Win11State* self) {
	RECT rc;
	RECT bar;
	GetWindowRect(self->taskbar, &bar);
	if(!self->classic || !IsWindow(self->classic))
		self->classic = FindClassicClock(self->taskbar);
	if(self->classic) {
		GetWindowRect(self->classic, &self->rc);
		return 1;
	}
	if(self->element) { // verify cached element
		wchar_t time_digits[16], time_digits_prev[16], year_digits[8];
		BSTR name = NULL;
		int valid = 0;
		GetTimeDigits(time_digits, time_digits_prev, year_digits);
		if(SUCCEEDED(IUIAutomationElement_get_CurrentName(self->element, &name))) {
			valid = ClockNameScore(name, time_digits, time_digits_prev, year_digits);
			SysFreeString(name);
		}
		if(valid && SUCCEEDED(IUIAutomationElement_get_CurrentBoundingRectangle(self->element, &rc)) && IsValidClockRect(&rc, &bar)) {
			self->rc = rc;
			return 1;
		}
		IUIAutomationElement_Release(self->element);
		self->element = NULL;
	}
	++self->searches;
	self->element = FindClockElement(self);
	if(self->element && SUCCEEDED(IUIAutomationElement_get_CurrentBoundingRectangle(self->element, &rc))) {
		DebugLog(0, "win11: clock found (%ld,%ld)-(%ld,%ld) after %u searches", rc.left, rc.top, rc.right, rc.bottom, self->searches);
		self->rc = rc;
		return 1;
	}
	return 0;
}

/** \brief samples the taskbar's color right next to (above or below) the original clock
 * \return boolean */
static int SampleBackground(Win11State* self, COLORREF* out) {
	RECT bar, overlay;
	POINT pts[3];
	HDC hdc;
	BYTE r[3], g[3], b[3];
	int y, i, n = 0;
	int margin_top, margin_bottom;
	GetWindowRect(self->taskbar, &bar);
	margin_top = self->rc.top - bar.top;
	margin_bottom = bar.bottom - self->rc.bottom;
	if(margin_top >= 2 || margin_bottom >= 2) { // above/below the clock (Windows 11)
		if(margin_top >= margin_bottom)
			y = bar.top + margin_top/2 + (margin_top > 2 ? 1 : 0); // skip the taskbar's 1px border
		else
			y = self->rc.bottom + margin_bottom/2;
		for(i=0; i<3; ++i) {
			pts[i].x = self->rc.left + (self->rc.right - self->rc.left) * (i+1) / 4;
			pts[i].y = y;
		}
	} else { // clock uses the full height, sample next to it, close to the top edge (icons are centered)
		GetWindowRect(m_clock, &overlay);
		y = bar.top + 3;
		pts[0].x = overlay.left - 2; pts[0].y = y;
		pts[1].x = overlay.left - 5; pts[1].y = y;
		pts[2].x = (overlay.right > self->rc.right ? overlay.right : self->rc.right) + 2; pts[2].y = y;
	}
	hdc = GetDC(NULL);
	if(!hdc)
		return 0;
	for(i=0; i<3; ++i) {
		POINT pt = pts[i];
		HWND hwnd;
		COLORREF color;
		hwnd = WindowFromPoint(pt);
		if(!hwnd || GetAncestor(hwnd, GA_ROOT) != self->taskbar || hwnd == m_clock)
			continue; // covered by something else (eg. fullscreen window)
		color = GetPixel(hdc, pt.x, pt.y);
		if(color == CLR_INVALID)
			continue;
		r[n] = GetRValue(color); g[n] = GetGValue(color); b[n] = GetBValue(color);
		++n;
	}
	ReleaseDC(NULL, hdc);
	if(!n)
		return 0;
	if(n == 3) { // per channel median
		#define MEDIAN3(a) ((a[0]>a[1]) ? ((a[1]>a[2]) ? a[1] : ((a[0]>a[2]) ? a[2] : a[0])) : ((a[0]>a[2]) ? a[0] : ((a[1]>a[2]) ? a[2] : a[1])))
		*out = RGB(MEDIAN3(r), MEDIAN3(g), MEDIAN3(b));
		#undef MEDIAN3
	} else {
		*out = RGB(r[0], g[0], b[0]);
	}
	return 1;
}

/** \brief opens/closes Windows 11's calendar by "clicking" the original clock */
static void InvokeCalendar(Win11State* self) {
	if(self->classic && IsWindow(self->classic)) { // same trick our DLL uses on Windows 10
		PostMessage(self->classic, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(4,4));
		PostMessage(self->classic, WM_LBUTTONUP, 0, MAKELPARAM(4,4));
		return;
	}
	if(self->element) {
		IUIAutomationElement* element = self->element;
		IUIAutomationTreeWalker* walker = NULL;
		int level;
		IUIAutomationElement_AddRef(element);
		IUIAutomation_get_ControlViewWalker(self->uia, &walker);
		for(level=0; element && level<4; ++level) { // the name might belong to a child of the button
			IUIAutomationInvokePattern* invoke = NULL;
			IUIAutomationElement* parent = NULL;
			if(SUCCEEDED(IUIAutomationElement_GetCurrentPatternAs(element, kUIA_InvokePatternId, &kIID_IUIAutomationInvokePattern, (void**)&invoke)) && invoke) {
				HRESULT hr = IUIAutomationInvokePattern_Invoke(invoke);
				IUIAutomationInvokePattern_Release(invoke);
				IUIAutomationElement_Release(element);
				if(walker)
					IUIAutomationTreeWalker_Release(walker);
				if(SUCCEEDED(hr))
					return;
				element = NULL;
				break;
			}
			if(walker)
				IUIAutomationTreeWalker_GetParentElement(walker, element, &parent);
			IUIAutomationElement_Release(element);
			element = parent;
		}
		if(element)
			IUIAutomationElement_Release(element);
		if(walker)
			IUIAutomationTreeWalker_Release(walker);
	}
	{ // fallback: Win+N (notification center & calendar)
		INPUT input[4];
		memset(input, 0, sizeof(input));
		input[0].type = input[1].type = input[2].type = input[3].type = INPUT_KEYBOARD;
		input[0].ki.wVk = input[3].ki.wVk = VK_LWIN;
		input[1].ki.wVk = input[2].ki.wVk = 'N';
		input[2].ki.dwFlags = input[3].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(4, input, sizeof(INPUT));
	}
}

static DWORD WINAPI Win11Thread(void* param) {
	typedef void* (WINAPI* SetThreadDpiAwarenessContext_t)(void* context);
	SetThreadDpiAwarenessContext_t pSetThreadDpiAwarenessContext;
	Win11State self;
	HRESULT hr_com;
	unsigned fails = 0;
	(void)param;
	memset(&self, 0, sizeof(self));
	// use physical pixels like the taskbar does (we're only "system aware")
	pSetThreadDpiAwarenessContext = (SetThreadDpiAwarenessContext_t)GetProcAddress(GetModuleHandleA("user32"), "SetThreadDpiAwarenessContext");
	if(pSetThreadDpiAwarenessContext)
		pSetThreadDpiAwarenessContext((void*)(intptr_t)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
	hr_com = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if(FAILED(CoCreateInstance(&kCLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER, &kIID_IUIAutomation, (void**)&self.uia)))
		self.uia = NULL;
	DebugLog(0, "win11: helper started (UIA: %s)", (self.uia ? "yes" : "no"));

	while(!m_stop) {
		RECT old_rc = self.rc;
		int found;
		if(!IsWindow(m_clock))
			break;
		self.taskbar = GetParent(m_clock); // GetParent() returns the owner for popups
		if(!self.taskbar)
			break;
		if(InterlockedExchange(&m_calendar, 0))
			InvokeCalendar(&self);
		found = LocateClock(&self);
		if(found != self.rc_valid || (found && !EqualRect(&old_rc, &self.rc))) {
			self.rc_valid = found;
			if(found)
				PostMessage(m_clock, CLOCKM_OVERLAY_RECT, MAKEWPARAM(self.rc.left, self.rc.top), MAKELPARAM(self.rc.right-self.rc.left, self.rc.bottom-self.rc.top));
			else
				PostMessage(m_clock, CLOCKM_OVERLAY_RECT, 0, 0);
		}
		if(found) {
			COLORREF bg;
			if(SampleBackground(&self, &bg) && (!self.bg_valid || bg != self.bg)) {
				self.bg = bg;
				self.bg_valid = 1;
				PostMessage(m_clock, CLOCKM_OVERLAY_BG, (WPARAM)bg, 1);
			}
		}
		if(found) {
			fails = 0;
		} else if(fails < 9) {
			++fails;
		}
		// back off if the clock can't be found (eg. hidden), searching is somewhat expensive
		WaitForSingleObject(m_wake, (found ? kUpdateInterval : kSearchInterval * (1 + fails)));
	}

	if(self.element)
		IUIAutomationElement_Release(self.element);
	if(self.uia)
		IUIAutomation_Release(self.uia);
	if(SUCCEEDED(hr_com))
		CoUninitialize();
	DebugLog(0, "win11: helper stopped");
	return 0;
}

int Win11Overlay_IsOverlay(HWND clock) {
	wchar_t classname[64];
	return clock && GetClassName(clock, classname, _countof(classname)) && !wcscmp(classname, TCLOCK_OVERLAY_CLASS);
}

void Win11Overlay_Stop() {
	if(!m_thread)
		return;
	InterlockedExchange(&m_stop, 1);
	SetEvent(m_wake);
	if(WaitForSingleObject(m_thread, 5000) == WAIT_TIMEOUT)
		DebugLog(0, "win11: helper didn't stop in time");
	CloseHandle(m_thread);
	m_thread = NULL;
	m_clock = NULL;
}

void Win11Overlay_Start(HWND clock) {
	Win11Overlay_Stop();
	if(!Win11Overlay_IsOverlay(clock))
		return;
	if(!m_wake)
		m_wake = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_clock = clock;
	m_stop = 0;
	m_calendar = 0;
	m_thread = CreateThread(NULL, 0, Win11Thread, NULL, 0, NULL);
}

int Win11Overlay_ToggleCalendar() {
	if(!m_thread)
		return 0;
	InterlockedIncrement(&m_calendar);
	SetEvent(m_wake);
	return 1;
}
