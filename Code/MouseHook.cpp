#include "Precompiled.h"
#include "MouseHook.h"
#include "Config.h"
#include "Utilities.h"
#include <CommCtrl.h>

const DWORD CLICK_TIME_FOR_TASK_SWITCHER = 250;

static HHOOK hHook;

static void cancelTaskView(POINT mousePosition) {
	kbdpress(VK_ESCAPE, 0);
	TaskManager::RunLaterOnSameThread([=] {
		INPUT input;
		ZeroMemory(&input, sizeof(input));
		input.type = INPUT_MOUSE;
		input.mi.mouseData = XBUTTON2;
		input.mi.dwFlags = MOUSEEVENTF_XDOWN;
		SendInput(1, &input, sizeof(input));

		input.mi.dwFlags = MOUSEEVENTF_XUP;
		SendInput(1, &input, sizeof(input));
		//extern void sendNextPageCommand();
		//sendNextPageCommand();
	}, 20);
}

LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
	// Unlike the keyboard hook, the mouse hook is called and processed properly even if MS TSC is the active window
	// So we let the host do the job and ignore anything on the guest
	if (TaskManager::isInTeamViewer || TaskManager::isBeingRemoteDesktopd) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}
	//if (config.startScreenSaverWithInsert) {
	//	DidPerformAnAction();
	//}

	if (config.altTabWithMouseButtons && nCode >= 0 && (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP)) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;

		// Do not process injected
		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED) {
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		static bool isDown = false;
		static POINT clickPosition;
		bool isGoingDown = wParam == WM_XBUTTONDOWN, isGoingUp = wParam == WM_XBUTTONUP;
		int button = mllStruct->mouseData >> 16;
		if (button & XBUTTON2) {
			if (isGoingDown) {
				isDown = true;
				clickPosition = mllStruct->pt;
				TaskManager::RunLaterOnSameThread([=] {
					kbddown(VK_LCONTROL, 0);
					kbddown(VK_LMENU, 0);
					TaskManager::RunLaterOnSameThread([=] {
						kbdpress(VK_TAB, 0);
						kbdup(VK_LCONTROL, 0);
						kbdup(VK_LMENU, 0);
						TaskManager::RunLaterOnSameThread([=] {
							if (isDown) {
								cancelTaskView(clickPosition);
							}
						}, CLICK_TIME_FOR_TASK_SWITCHER);
					});
				});
				return 1;
			}
			else if (isGoingUp) {
				isDown = false;
				return 1;
			}
		}
	}

	if (config.scrollAccelerationFactor > 0 && nCode >= 0 && wParam == WM_MOUSEWHEEL) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED) {
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		static DWORD lastWheelTime;
		static int lastMouseDelta = 0;
		static int dismissedIrregularScrollsFor = 0;
		static float consecutiveScrolls = 1;

		int mouseDelta = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
		if (config.scrollAccelerationDismissTrackpad) {
			if (labs(mouseDelta) != labs(lastMouseDelta)) {
				dismissedIrregularScrollsFor = 4;
			}
			if (dismissedIrregularScrollsFor > 0) {
				dismissedIrregularScrollsFor--;
				lastMouseDelta = mouseDelta;
				lastWheelTime = mllStruct->time;
				return CallNextHookEx(NULL, nCode, wParam, lParam);
			}
		}
		if (mouseDelta * lastMouseDelta < 0) {
			consecutiveScrolls = 1;
			//printf("RESETED (%d %d)\n", mouseDelta, lastMouseDelta);
		}
		else {
			//printf("Affected: %f (time diff=%d)\n", 1 - float(mllStruct->time - lastWheelTime) / MIN_SCROLL_TIME, mllStruct->time - lastWheelTime);
			consecutiveScrolls += (1 - float(mllStruct->time - lastWheelTime) / config.scrollAccelerationIntertia) * config.scrollAccelerationFactor;
			consecutiveScrolls = max(0, min(config.scrollAccelerationMaxScrollFactor, consecutiveScrolls));
			//printf("Time: %lu - %lu = %lu (total %d)\n", mllStruct->time, lastWheelTime, mllStruct->time - lastWheelTime, consecutiveScrolls);
			//printf("%f\n", consecutiveScrolls);
		}
		lastMouseDelta = mouseDelta;
		lastWheelTime = mllStruct->time;

		if (!config.scrollAccelerationSendMultipleMessages && consecutiveScrolls > 1) {
			mouseDelta = int(mouseDelta * consecutiveScrolls);
			TaskManager::Run([=] {
				INPUT input;
				ZeroMemory(&input, sizeof(input));
				input.type = INPUT_MOUSE;
				input.mi.mouseData = mouseDelta;
				input.mi.dwFlags = MOUSEEVENTF_WHEEL;
				SendInput(1, &input, sizeof(input));
				//printf("INJECTED %d\n", GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData));
			});
			return 1;
		}
		else if (config.scrollAccelerationSendMultipleMessages && consecutiveScrolls >= 1.5f) {
			int messageCount = int(roundf(consecutiveScrolls - 1.0f));
			TaskManager::Run([=] {
				INPUT input;
				ZeroMemory(&input, sizeof(input));
				input.type = INPUT_MOUSE;
				input.mi.mouseData = mouseDelta;
				input.mi.dwFlags = MOUSEEVENTF_WHEEL;
				//printf("Sending %d messages\n", messageCount);
				for (int i = 0; i < messageCount; i++) SendInput(1, &input, sizeof(input));
			});
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void MouseHook::start() {
	if (config.altTabWithMouseButtons /*|| config.startScreenSaverWithInsert*/) {
		hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc_AltTab, GetModuleHandle(NULL), 0);
	}
}

void MouseHook::terminate() {
	if (hHook) {
		UnhookWindowsHookEx(hHook);
		hHook = 0;
	}
}
