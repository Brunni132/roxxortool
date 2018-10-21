#include "Precompiled.h"
#include "MouseHook.h"
#include "Config.h"
#include "Utilities.h"
#include <CommCtrl.h>

const DWORD CLICK_TIME_FOR_TASK_SWITCHER = 250;

static HHOOK hHook;

static void RunInWorker(std::function<void()> function);

static void cancelTaskView(POINT mousePosition) {
	kbdpress(VK_ESCAPE, 0);
	RunAfterDelay([=] {
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
	}, 100);
}

LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
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
				RunAfterDelay([=] {
					kbddown(VK_LCONTROL, 0);
					kbddown(VK_LMENU, 0);
					RunAfterDelay([=] {
						kbdpress(VK_TAB, 0);
						kbdup(VK_LCONTROL, 0);
						kbdup(VK_LMENU, 0);
						RunAfterDelay([=] {
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

	if (nCode >= 0 && wParam == WM_MOUSEWHEEL) {
		// For some reason it's always injected on the Precision Trackpad...
		//static unsigned consecutiveScrolls = 0;
		//MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		//// Do not process injected
		//if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED) {
		//	printf("Was injected %x\n", mllStruct->flags);
		//	return CallNextHookEx(NULL, nCode, wParam, lParam);
		//}

		//int zDelta = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
		//printf("zDelta: %d\n", zDelta);

		//INPUT input;
		//ZeroMemory(&input, sizeof(input));
		//input.type = INPUT_MOUSE;
		//input.mi.dx = mllStruct->pt.x;
		//input.mi.dy = mllStruct->pt.y;
		//input.mi.mouseData = mllStruct->mouseData;
		//input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		//input.mi.time = mllStruct->time;
		//input.mi.dwExtraInfo = mllStruct->dwExtraInfo;
		//SendInput(1, &input, sizeof(input));
		//return 1;

		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED) {
			printf("Was injected %x %d\n", mllStruct->flags, GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData));
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		static int lastWheelTime;
		static unsigned consecutiveScrolls = 0;
		consecutiveScrolls++;

		printf("Time: %d\n", mllStruct->time - lastWheelTime);
		lastWheelTime = mllStruct->time;

		//if (consecutiveScrolls < 2) {
			//int zDelta = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
			//int repeatCount = 1;

			//for (int i = 0; i < repeatCount; i++) {
			//	INPUT input;
			//	ZeroMemory(&input, sizeof(input));
			//	input.type = INPUT_MOUSE;
			//	input.mi.dx = mllStruct->pt.x;
			//	input.mi.dy = mllStruct->pt.y;
			//	input.mi.mouseData = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
			//	input.mi.dwFlags = MOUSEEVENTF_WHEEL;
			//	//input.mi.time = mllStruct->time;
			//	//input.mi.dwExtraInfo = mllStruct->dwExtraInfo;
			//	SendInput(1, &input, sizeof(input));
			//	printf("Sent input\n");
			//}
		//}

		int mouseDelta = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
		RunInWorker([=] {
			INPUT input;
			ZeroMemory(&input, sizeof(input));
			input.type = INPUT_MOUSE;
			input.mi.mouseData = mouseDelta * 2;
			input.mi.dwFlags = MOUSEEVENTF_WHEEL;
			SendInput(1, &input, sizeof(input));
			printf("INJECTED %d\n", GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData));
		});
		return 1;

		//return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

//	if (nCode >= 0 && wParam == WM_MOUSEWHEEL) {
//		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
//		static int interceptedCount = 0;
//
//		if (interceptedCount++ >= 5)
//			exit(0);
//
//		// Do not process injected
//		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED)
//			return CallNextHookEx(NULL, nCode, wParam, lParam);
//
////#ifdef _DEBUG
////		printf("w=%x flags=%x md=%x time=%x\n", wParam, mllStruct->flags, mllStruct->mouseData, mllStruct->time);
////#endif
//		INPUT input;
//		ZeroMemory(&input, sizeof(input));
//		input.type = INPUT_MOUSE;
//		input.mi.dx = mllStruct->pt.x;
//		input.mi.dy = mllStruct->pt.y;
//		input.mi.mouseData = mllStruct->mouseData;
//		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
//		//input.mi.time = 0;
//		//input.mi.dwExtraInfo = 0;
//		SendInput(1, &input, sizeof(input));
//		return 1;
//	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static std::list<std::function<void()>> queue;

void RunInWorker(std::function<void()> function) {
	queue.push_back(function);
}

//LRESULT CALLBACK LowLevelMouseProc_AltTab(int nCode, WPARAM wParam, LPARAM lParam) {
//	if (nCode >= 0 && wParam == WM_MOUSEWHEEL) {
//		MSLLHOOKSTRUCT *mllStruct = (MSLLHOOKSTRUCT*)lParam;
//		if (mllStruct->flags & LLMHF_INJECTED || mllStruct->flags & LLMHF_LOWER_IL_INJECTED)
//			return CallNextHookEx(NULL, nCode, wParam, lParam);
//
//		int mouseDelta = GET_WHEEL_DELTA_WPARAM(mllStruct->mouseData);
//		RunInWorker([=] {
//			INPUT input;
//			ZeroMemory(&input, sizeof(input));
//			input.type = INPUT_MOUSE;
//			input.mi.mouseData = mouseDelta;
//			input.mi.dwFlags = MOUSEEVENTF_WHEEL;
//			SendInput(1, &input, sizeof(input));
//		});
//		return 1;
//	}
//	return CallNextHookEx(NULL, nCode, wParam, lParam);
//}

DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
	while (true) {
		while (!queue.empty()) {
			auto& fun = queue.front();
			fun();
			queue.pop_front();
		}
		Sleep(10);
	}

	return 0;
}

void MouseHook::start() {
	if (config.altTabWithMouseButtons /*|| config.startScreenSaverWithInsert*/) {
		hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc_AltTab, GetModuleHandle(NULL), 0);

		HANDLE hThread = CreateThread(
			NULL,                   // default security attributes
			0,                      // use default stack size  
			MyThreadFunction,       // thread function name
			NULL,                   // argument to thread function 
			0,                      // use default creation flags 
			NULL);                  // returns the thread identifier 
	}
}

void MouseHook::terminate() {
	if (hHook) {
		UnhookWindowsHookEx(hHook);
		hHook = 0;
	}
}
