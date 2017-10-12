#include "Precompiled.h"
#include "MouseHook.h"
#include "Config.h"
#include "Utilities.h"
#include <CommCtrl.h>

#define SHIFTED 0x8000
const DWORD CLICK_TIME_FOR_TASK_SWITCHER = 200;

static HHOOK hHook;

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	WPARAM originalMessage;
	static POINT p;
	static HWND hWnd, hWndTmp;
	static MSLLHOOKSTRUCT *msl;
	static char className[256];

	if (nCode != HC_ACTION || (wParam != WM_MOUSEWHEEL && wParam != WM_MOUSEHWHEEL))
		goto skip;

	msl = (MSLLHOOKSTRUCT*)lParam;
	originalMessage = wParam;

	if (!GetCursorPos(&p))
		goto skip;
	hWnd = WindowFromPoint(p);
	if (!hWnd)
		goto skip;

	WINDOWINFO wi;
	wi.cbSize = sizeof(wi);
	if (!GetWindowInfo(hWnd, &wi))
		goto skip;
	if (!GetClassName(hWnd, className, 256))
		goto skip;

	if (!strcmp(className, TOOLTIPS_CLASS)) {
		ShowWindow(hWnd, SW_HIDE);
		hWnd = WindowFromPoint(p);
		if (!hWnd)
			goto skip;
	}

	if (!strcmp(className, "Button") && (wi.dwStyle & BS_GROUPBOX)) {
		hWndTmp = hWnd;
		EnableWindow(hWndTmp, FALSE);
		hWnd = WindowFromPoint(p);
		EnableWindow(hWndTmp, TRUE);
		if (!hWnd)
			goto skip;
	}

	// Windows 8 | VMWare
	if (!strncmp(className, "Windows.UI.Core", 15) || !strcmp(className, "MKSEmbedded"))
		goto skip;

	// Make horizontal scroll wider
	if (config.horizontalScrollFactor != 1.0f && wParam == WM_MOUSEHWHEEL) {
		wParam = (SHORT)(HIWORD(msl->mouseData) * config.horizontalScrollFactor) << 16;
	}
	else {
		wParam = msl->mouseData & 0xFFFF0000;
	}

	wParam = msl->mouseData & 0xFFFF0000;

	if ((GetKeyState(VK_LBUTTON) | GetKeyState(VK_RBUTTON) | GetKeyState(VK_MBUTTON)) & SHIFTED)
		goto skip;

	if ((GetKeyState(VK_CONTROL) | GetKeyState(VK_LCONTROL) | GetKeyState(VK_RCONTROL)) & SHIFTED) {
		wParam |= MK_CONTROL;
	}
	if ((GetKeyState(VK_SHIFT) | GetKeyState(VK_LSHIFT) | GetKeyState(VK_RSHIFT)) & SHIFTED) {
		wParam |= MK_SHIFT;
	}

	lParam = p.x | (p.y << 16);

	if (!PostMessage(hWnd, (UINT)originalMessage, wParam, lParam))
		goto skip;

	return 1;

skip:
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

static void triggerTaskView() {
	RunAfterDelay([=] {
		kbddown(VK_LCONTROL, 0);
		kbddown(VK_LMENU, 0);
		kbdpress(VK_TAB, 0);
		kbdup(VK_LCONTROL, 0);
		kbdup(VK_LMENU, 0);
	});
}

//static void cancelTaskView(int x, int y) {
//	kbdpress(VK_ESCAPE, 0);
//	RunAfterDelay([=] {
//		mouse_event(MOUSEEVENTF_XDOWN, x, y, XBUTTON2, 0);
//		mouse_event(MOUSEEVENTF_XUP, x, y, XBUTTON2, 0);
//	}, 10);
//}

LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0 && (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED) {}
		else {
			static DWORD downTime;
			bool isDown = wParam == WM_XBUTTONDOWN;
			int button = mllStruct->mouseData >> 16;
			if (button & XBUTTON2) {
				if (isDown)
					downTime = GetTickCount();
				else {
					if (GetTickCount() - downTime < CLICK_TIME_FOR_TASK_SWITCHER)
						triggerTaskView();
					else
						RunAfterDelay([=] {
							mouse_event(MOUSEEVENTF_XDOWN, mllStruct->pt.x, mllStruct->pt.y, XBUTTON2, 0);
							mouse_event(MOUSEEVENTF_XUP, mllStruct->pt.x, mllStruct->pt.y, XBUTTON2, 0);
						}, 0);
				}
				return 1;
			}
		}
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void MouseHook::start() {
	if (config.altTabWithMouseButtons)
		hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc_AltTab, GetModuleHandle(NULL), 0);
	else
		hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
}
