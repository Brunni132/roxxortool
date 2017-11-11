#include "Precompiled.h"
#include "MouseHook.h"
#include "Config.h"
#include "Utilities.h"
#include <CommCtrl.h>

const DWORD CLICK_TIME_FOR_TASK_SWITCHER = 250;

static HHOOK hHook;

static void triggerTaskView() {
	RunAfterDelay([=] {
		kbddown(VK_LCONTROL, 0);
		kbddown(VK_LMENU, 0);
		kbdpress(VK_TAB, 0);
		kbdup(VK_LCONTROL, 0);
		kbdup(VK_LMENU, 0);
	});
}

static void cancelTaskView(POINT mousePosition) {
	kbdpress(VK_ESCAPE, 0);
	RunAfterDelay([=] {
		mouse_event(MOUSEEVENTF_XDOWN, mousePosition.x, mousePosition.y, XBUTTON2, 0);
		mouse_event(MOUSEEVENTF_XUP, mousePosition.x, mousePosition.y, XBUTTON2, 0);
	}, 10);
}

LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode >= 0 && (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		//printf("w=%x flags=%x md=%x time=%x\n", wParam, mllStruct->flags, mllStruct->mouseData, mllStruct->time);
		// Do not process injected
		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED)
			return CallNextHookEx(hHook, nCode, wParam, lParam);

		static bool isDown = false;
		static POINT clickPosition;
		bool isGoingDown = wParam == WM_XBUTTONDOWN, isGoingUp = wParam == WM_XBUTTONUP;
		int button = mllStruct->mouseData >> 16;
		if (button & XBUTTON2) {
			if (isGoingDown) {
				isDown = true;
				clickPosition = mllStruct->pt;
				triggerTaskView();
				RunAfterDelay([=] {
					if (isDown) {
						cancelTaskView(clickPosition);
					}
				}, CLICK_TIME_FOR_TASK_SWITCHER);
				return 1;
			}
			else if (isGoingUp) {
				isDown = false;
				return 1;
			}
		}
	}
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void MouseHook::start() {
	if (config.altTabWithMouseButtons)
		hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc_AltTab, GetModuleHandle(NULL), 0);
}

void MouseHook::terminate() {
	if (hHook) {
		UnhookWindowsHookEx(hHook);
		hHook = 0;
	}
}
