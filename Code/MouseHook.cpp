#include "Precompiled.h"
#include "MouseHook.h"
#include "Config.h"
#include "Utilities.h"
#include <CommCtrl.h>

static HHOOK hHook;
static vector<WPARAM> mouseEventsToIgnore;

static void addForIgnore(WPARAM eventType) {
	mouseEventsToIgnore.push_back(eventType);
}

static void cancelTaskView(POINT mousePosition) {
	kbdpress(VK_ESCAPE, 0);
	// Avoid running on the same thread because it can cause lock up the OS for a while in case of delays in the processing
	TaskManager::RunLater([=] {
		INPUT input;
		ZeroMemory(&input, sizeof(input));
		input.type = INPUT_MOUSE;
		input.mi.mouseData = XBUTTON2;
		input.mi.dwFlags = MOUSEEVENTF_XDOWN;
		addForIgnore(WM_XBUTTONDOWN);
		SendInput(1, &input, sizeof(input));

		input.mi.dwFlags = MOUSEEVENTF_XUP;
		addForIgnore(WM_XBUTTONUP);
		SendInput(1, &input, sizeof(input));
		//extern void sendNextPageCommand();
		//sendNextPageCommand();
	}, 20);
}

#ifdef _DEBUG
static char *buttonName(WPARAM wParam) {
	switch (wParam) {
	case WM_XBUTTONDOWN: return "WM_XBUTTONDOWN";
	case WM_XBUTTONUP: return "WM_XBUTTONUP";
	case WM_XBUTTONDBLCLK: return "WM_XBUTTONDBLCLK";
	case WM_MOUSEWHEEL: return "WM_MOUSEWHEEL";
	default: return "";
	}
}
#endif


LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
	// Unlike the keyboard hook, the mouse hook is called and processed properly even if MS TSC is the active window
	// So we let the host do the job and ignore anything on the guest
	if (TaskManager::isBeingRemoteDesktopd && !config.processAltTabWithMouseButtonsEvenFromRdp) {
#ifdef _DEBUG
		printf("Ignoring mouse event because in remote desktop session\n");
#endif
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

//#ifdef _DEBUG
//	{
//		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
//		bool injected = mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED;
//		printf("Mouse%s, w=%llx (%s), for ignore=%zd\n", injected ? " (inj.)" : "", wParam, buttonName(wParam), mouseEventsToIgnore.size());
//	}
//#endif

	// See if we should ignore the event
	for (auto it = mouseEventsToIgnore.begin(); it != mouseEventsToIgnore.end(); ++it) {
		if (*it == wParam) {
#ifdef _DEBUG
			MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
			printf("Ignored event %s! (%d)\n", buttonName(wParam), GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData));
#endif
			mouseEventsToIgnore.erase(it);
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}
	}

	if (config.altTabWithMouseButtons && nCode >= 0 && (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) && !TaskManager::isInTeamViewer) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		static bool isDown = false;
		static POINT clickPosition;
		bool isGoingDown = wParam == WM_XBUTTONDOWN, isGoingUp = wParam == WM_XBUTTONUP;
		int button = mllStruct->mouseData >> 16;

		if (button & XBUTTON2) {
			if (isGoingDown) {
				const DWORD CLICK_TIME_FOR_TASK_SWITCHER = 250;
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
							//kbdpress(VK_HOME, 0);
							TaskManager::RunLaterOnSameThread([=] {
								if (isDown) {
									cancelTaskView(clickPosition);
								}
							}, CLICK_TIME_FOR_TASK_SWITCHER - 50);
						}, 50);
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

	if (config.scrollAccelerationEnabled && nCode >= 0 && wParam == WM_MOUSEWHEEL) {
		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED) {
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		static DWORD lastWheelTime;
		static int lastMouseDelta = 0x7fffffff;
		static int dismissedIrregularScrollsFor = 0;
		static float consecutiveScrolls = 0;

		int mouseDelta = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
		//printf("PROCESSED %d\n", mouseDelta);
		if (config.scrollAccelerationBaseValue > 0) {
			//if (labs(mouseDelta) != labs(lastMouseDelta)) {
			//	dismissedIrregularScrollsFor = lastMouseDelta == 0x7fffffff ? 1 : 4;
			if (labs(mouseDelta) != labs(config.scrollAccelerationBaseValue)) {
				dismissedIrregularScrollsFor = 5;
			}
			if (dismissedIrregularScrollsFor > 0) {
				dismissedIrregularScrollsFor--;
				lastMouseDelta = mouseDelta;
				lastWheelTime = mllStruct->time;
				//printf("Dismissed trackpad\n");
				return CallNextHookEx(NULL, nCode, wParam, lParam);
			}
		}
		if (mouseDelta * lastMouseDelta < 0) {
			consecutiveScrolls = 0;
			//printf("RESETED (%d %d)\n", mouseDelta, lastMouseDelta);
		}
		else {
			//if (consecutiveScrolls <= 1) printf("\n\nWas reseted\n");
			//printf("Affected: %f (time diff=%d [%ld - %ld])\n", 1 - float(mllStruct->time - lastWheelTime) / config.scrollAccelerationIntertia, mllStruct->time - lastWheelTime, mllStruct->time, lastWheelTime);
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
				input.mi.dwExtraInfo = 0x8;
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
