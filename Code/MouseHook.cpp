#include "Precompiled.h"
#include "MouseHook.h"
#include "Config.h"
#include "Utilities.h"
#include <CommCtrl.h>

#define SHIFTED 0x8000

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

LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
	static bool firstButtonIsDown = false, secondButtonIsDown = false, eatNext = false;
	if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		bool isDown = wParam == WM_XBUTTONDOWN;
		if (mllStruct->mouseData == XBUTTON1)
			firstButtonIsDown = isDown;
		else if (mllStruct->mouseData == XBUTTON2)
			secondButtonIsDown = isDown;

		// Second button released -> alt tab
		if (wParam == WM_XBUTTONUP && (mllStruct->mouseData >> 16 & 0xffff) == XBUTTON2) {
			kbddown(VK_LCONTROL, 0);
			kbddown(VK_LMENU, 0);
			kbdpress(VK_TAB, 0);
			kbdup(VK_LCONTROL, 0);
			kbdup(VK_LMENU, 0);
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
