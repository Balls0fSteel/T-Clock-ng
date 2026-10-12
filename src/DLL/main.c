/*-----------------------------------------------------
// main.c : API, hook procedure -> KAZUBON 1997-2001
//-------------------------------------------------------*/
// Modified by Stoic Joker: Tuesday, March 2 2010 - 10:42:42
#include "tcdll.h"

HANDLE g_exit_lock = NULL;
void InitClock(HWND hwnd);
HWND CreateOverlayClock(HWND taskbar); // tclock.c

//========================================================================================
//-----------------------------+++--> Entry Point of This (SystemTray Clock ShellHook) DLL:
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)   //---+++-->
{
	(void)lpvReserved;
	api.hInstance = hinstDLL;
	switch(fdwReason) {
	case DLL_PROCESS_ATTACH:
//		DebugLog(0, "DLL_PROCESS_ATTACH"); // API not loaded yet, thus bypasses "NoLog" setting (portable mode)
		#if !defined(_MSC_VER) || defined(_DLL) // if not static MSVC build
		DisableThreadLibraryCalls(hinstDLL);
		#endif
		break;
	case DLL_PROCESS_DETACH:
		DebugLog(0, "DLL_PROCESS_DETACH");
		DebugLogFree();
		if(g_exit_lock) {
			// "notify" Clock.exe about our exit
			ReleaseSemaphore(g_exit_lock, 1, NULL);
			CloseHandle(g_exit_lock);
		}
		break;
	default: break;
	}
	return TRUE;
}
//========================================================================================
//---------------------------------------------+++--> SystemTray Clock ShellHook Procedure:
LRESULT CALLBACK Hook_CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)   //-----+++-->
{/// @todo : rewrite hooking code (use wrapper exe to hook it)
	CWPSTRUCT* pcwps = (CWPSTRUCT*)lParam;
	if(nCode >= 0 && !gs_taskbar) { // if this message is sent to the clock
		char classname[80];
		if(gs_overlay_bar) { // no classic clock, create our own on the taskbar's thread
			static char s_creating = 0;
			if(pcwps->hwnd == gs_overlay_bar && !s_creating) {
				HWND clock;
				s_creating = 1;
				clock = CreateOverlayClock(gs_overlay_bar);
				if(clock) {
					InitClock(clock);
					PostMessage(gs_hwndTClockMain,MAINM_CLOCKINIT,0,(LPARAM)clock);
				} else {
					DebugLog(0, "overlay clock creation failed (%lu)", GetLastError());
					gs_overlay_bar = NULL; // don't retry on every message
				}
				s_creating = 0;
			}
		} else if(GetClassNameA(pcwps->hwnd, classname, 80) && !strcmp(classname, "TrayClockWClass")) {
			InitClock(pcwps->hwnd); // initialize (tclock.c)
			PostMessage(gs_hwndTClockMain,MAINM_CLOCKINIT,0,(LPARAM)pcwps->hwnd);
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
