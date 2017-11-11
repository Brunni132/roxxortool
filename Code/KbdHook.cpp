#include "Precompiled.h"
#include "KbdHook.h"
#include "AudioMixer.h"
#include "Config.h"
#include "Main.h"
#include "Monitor.h"
#include "WindowsExplorer.h"
#include "Utilities.h"
#include "StatusWindow.h"
#include "DisableAnimationsForWinTab.h"

static bool lCtrlPressed = false, rCtrlPressed = false, lWinPressed = false, rWinPressed = false, lShiftPressed = false, rShiftPressed = false, lAltPressed = false;
static bool ctrlPressed() { return lCtrlPressed || rCtrlPressed; }
static bool winPressed() { return lWinPressed || rWinPressed; }
static bool shiftPressed() { return lShiftPressed || rShiftPressed; }
static bool altPressed() { return lAltPressed; }

// Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html or https://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct
void kbddown(int vkCode, BYTE scanCode, int flags) {
	keybd_event(vkCode, scanCode, flags, 0);
}

void kbdup(int vkCode, BYTE scanCode, int flags) {
	keybd_event(vkCode, scanCode + 0x80, flags | KEYEVENTF_KEYUP, 0);
}

void kbdpress(int vkCode, BYTE scanCode, int flags) {
	kbddown(vkCode, scanCode, flags);
	kbdup(vkCode, scanCode, flags);
}

static const struct { int original; int modified; } keyboardEatTable[] = {
	VK_LEFT, VK_HOME,
	VK_RIGHT, VK_END,
	VK_UP, VK_PRIOR,
	VK_DOWN, VK_NEXT,
	VK_BACK, VK_DELETE
};

enum Location { START, CURRENT, END };

static void switchToHiragana() {
	bool needsShift = !shiftPressed(), needsControl = !ctrlPressed();
	if (needsShift) kbddown(VK_RSHIFT, 0);
	kbdpress(VK_CAPITAL, 0);
	if (needsShift) kbdup(VK_RSHIFT, 0);
	if (needsControl) kbddown(VK_RCONTROL, 0);
	kbdpress(VK_CAPITAL, 0);
	if (needsControl) kbdup(VK_RCONTROL, 0);
}

static void moveToTask(int taskNo, Location from) {
	bool needsWin = !winPressed(), needsShift = !shiftPressed();
	if (from == START) kbdpress(VK_HOME, 0);
	else if (from == END) kbdpress(VK_END, 0);
	else if (from == CURRENT) taskNo += sgn(taskNo);
	if (taskNo == 0 || taskNo == 1) return;

	RunAfterDelay([=] {
		if (needsWin) kbddown(VK_RWIN, 0);
		if (taskNo <= 0) {
			if (needsShift) kbddown(VK_RSHIFT, 0);
			for (int i = 0; i < -1 - taskNo; i += 1) kbdpress('T', 0);
			if (needsShift) kbdup(VK_RSHIFT, 0);
		}
		else {
			for (int i = 0; i < taskNo - 1; i += 1) kbdpress('T', 0);
		}
		if (needsWin) kbdup(VK_RWIN, 0);
	});
}

static HHOOK g_hHook;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	// Not something for us
	if (nCode != HC_ACTION) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	// TODO Florian -- replace all this with reading the scan code (in kbd)
	KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT*) lParam;
	int nKey = kbd->vkCode;
	int processed = 0;
	bool ctrlKeyWasPressed = lCtrlPressed || rCtrlPressed, rShiftWasPressed = rShiftPressed;

	// Special keys
	if (wParam == WM_KEYDOWN || wParam == WM_KEYUP) {
		switch (nKey) {
		case VK_LCONTROL:
			lCtrlPressed = wParam == WM_KEYDOWN;
			break;
		case VK_RCONTROL:
			rCtrlPressed = wParam == WM_KEYDOWN;
			break;
		case VK_LSHIFT:
			lShiftPressed = wParam == WM_KEYDOWN;
			break;
		case VK_RSHIFT:
			rShiftPressed = wParam == WM_KEYDOWN;
			break;
		case VK_LWIN:
			lWinPressed = wParam == WM_KEYDOWN;
			break;
		case VK_RWIN:
			rWinPressed = wParam == WM_KEYDOWN;
			break;
		case VK_LMENU:
			lAltPressed = wParam == WM_KEYDOWN;
			break;
		}
	}

	// Ignore injected input
	bool injected = (kbd->flags & (LLKHF_INJECTED | LLKHF_LOWER_IL_INJECTED));
	//if (wParam == WM_KEYDOWN) {
	//	printf("Down: %d %d\n", nKey, kbd->scanCode);
	//}
	//if (wParam == WM_KEYUP) {
	//	printf("Up: %d %d\n", nKey, kbd->scanCode);
	//}

	if (wParam == WM_KEYDOWN) {
		// Insert -> start screen saver & lock
		if (config.startScreenSaverWithInsert) {
			if (nKey == VK_INSERT) {
				LockWorkStation();
				SendMessage(GetForegroundWindow(), WM_SYSCOMMAND, SC_SCREENSAVE, 0);
				return 1;
			}
		}

		// Ctrl+H -> toggle hide/show hidden folders
		if (config.toggleHideFolders) {
			if (nKey == 'H' && ctrlKeyWasPressed) {
				if (WindowsExplorer::isActive()) {
					WindowsExplorer::toggleShowHideFolders();
					// Refresh the explorer
					kbdpress(VK_F5, 0, 0);
				}
			}
		}

		// Logarithmic volume management
		if (config.smoothVolumeControl) {
			if (nKey == 0xae) {
				// Molette -
				RunAfterDelay([] { AudioMixer::decrementVolume(config.volumeIncrementQuantity); });
				processed = 0xae;
			} else if (nKey == 0xaf) {
				// Molette +
				RunAfterDelay([] { AudioMixer::incrementVolume(config.volumeIncrementQuantity); });
				processed = 0xaf;
			}

			// Rel�che la touche, il ne faut pas que Windows la prenne
			if (processed != 0) {
				kbdup(processed, 0);
				return 1;
			}
		}

		// External monitor brightness change
		if (lWinPressed && lCtrlPressed) {
			if (config.ddcCiBrightnessControl) {
				if (nKey == VK_LEFT) {
					int qty = lShiftPressed ? 1 : config.brightnessIncrementQuantity;
					RunAfterDelay([qty] {
						Monitor::decreaseBrightnessBy(qty);
					});
					return 1;
				}
				else if (nKey == VK_RIGHT) {
					int qty = lShiftPressed ? 1 : config.brightnessIncrementQuantity;
					RunAfterDelay([qty] {
						Monitor::increaseBrightnessBy(qty);
					});
					return 1;
				}
			}
			// Reexecute ourselves on Ctrl+Win+R
			if (config.reloadConfigWithCtrlWinR && nKey == 'R') {
				RunAfterDelay([] {
					Main::editConfigAndRelaunch();
				});
				return 1;
			}
		}

		if (config.winFOpensYourFiles && lWinPressed && nKey == 'F') {
			if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0);
			WindowsExplorer::showHomeFolderWindow();
			return 1;
		}
		if (config.winHHidesWindow && lWinPressed && nKey == 'H') {
			if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0); // To avoid bringing the menu
			ShowWindow(GetForegroundWindow(), SW_MINIMIZE);
			return 1;
		}

		if (config.closeWindowWithWinQ && lWinPressed && nKey == 'Q') {
			bool needAlt = !lAltPressed;
			if (needAlt) kbddown(VK_LMENU, 0);
			kbdup(VK_LWIN, 0); // Temporarily release WIN since Win+Alt+F4 does nothing
			kbdpress(VK_F4, 0); // +F4
			kbddown(VK_LWIN, 0); // Re-enable WIN
			if (needAlt) kbdup(VK_LMENU, 0);
			return 1;
		}

		if (config.winTSelectsLastTask) {
			 static bool inFunction = false;
			// // Check that we are still in the task bar and restart function (Win+T from start) if not
			 if (inFunction) {
			 	char className[128];
			 	GetClassNameA(GetForegroundWindow(), className, 128);
				if (strcmp(className, "Shell_TrayWnd")) {
					inFunction = false;
				}
			}
			if (inFunction) {
				if (nKey >= '5' && nKey <= '7') {
					moveToTask(11 + (nKey - '5') * 10, START);
					return -1;
				}
				else if (nKey == VK_PRIOR) {
					moveToTask(-5, CURRENT);
					return -1;
				}
				else if (nKey == VK_NEXT) {
					moveToTask(+5, CURRENT);
					return -1;
				}
			}
			if (lWinPressed && nKey == 'T' && !injected) {
				inFunction = true;
				// First press on Win+T
				RunAfterDelay([] {
					kbdpress('T', 0);
					kbdpress(VK_END, 0);
				}, 10);
			}
		}
	}

	if (wParam == WM_KEYUP) {
		if (config.selectHiraganaByDefault) {
			static bool shouldSwitchToHiragana = false;
			if (!winPressed() && shouldSwitchToHiragana) {
				shouldSwitchToHiragana = false;
				RunAfterDelay([] {
					switchToHiragana();
					RunAfterDelay([] {
						bool needsControl = !ctrlPressed();
						if (needsControl) kbddown(VK_RCONTROL, 0);
						kbdpress(VK_CAPITAL, 0);
						kbdpress(VK_CAPITAL, 0);
						if (needsControl) kbdup(VK_RCONTROL, 0);
					}, 500);
				}, 50);
			}
			if (nKey == VK_SPACE && winPressed()) {
				// Execute when both have been released
				shouldSwitchToHiragana = true;
			}
		}
	}

	// TEMP
	//if (true) {
	//	struct Key {
	//		DWORD vkCode, scanCode;
	//	};
	//	printf("%x %x\n", kbd->vkCode, kbd->scanCode);
	//	if (kbd->vkCode == 0xff && kbd->scanCode == 0x79) { // 変換
	//		if (wParam == WM_KEYDOWN)
	//			keybd_event(0xa4, 0x38, kbd->flags, kbd->dwExtraInfo);
	//		else
	//			keybd_event(0xa4, 0x38, kbd->flags | KEYEVENTF_KEYUP, kbd->dwExtraInfo);
	//		return 1;
	//	}
	//	if (kbd->vkCode == 0xeb && kbd->scanCode == 0x7b) { // 無変換
	//		if (wParam == WM_KEYDOWN)
	//			keybd_event(0xa4, 0x38, kbd->flags, kbd->dwExtraInfo);
	//		else
	//			keybd_event(0xa4, 0x38, kbd->flags | KEYEVENTF_KEYUP, kbd->dwExtraInfo);
	//		return 1;
	//	}
	//}

	//if (true) {
	//	static bool capsIsDown;
	//	if (nKey == VK_CAPITAL) {
	//		capsIsDown = wParam == WM_KEYDOWN;
	//		return 1;
	//	}
	//	if (nKey == 'J' && wParam == WM_KEYDOWN) {
	//		TCHAR name[1024];
	//		HKL keyboards[10];
	//		int count = GetKeyboardLayoutList(10, keyboards);
	//		for (int i = 0; i < count; i++)
	//			printf("Keybd: %x\n", keyboards[i]);
	//		// printf("Enabled: %x", ActivateKeyboardLayout(keyboards[1], KLF_ACTIVATE | KLF_SETFORPROCESS));
	//		SystemParametersInfo(SPI_SETDEFAULTINPUTLANG, 0, keyboards[0], SPIF_SENDCHANGE);
	//		//GetKeyboardLayoutName(name);
	//		//HKL hkl1 = LoadKeyboardLayout("00000409", KLF_REPLACELANG | KLF_ACTIVATE | KLF_SUBSTITUTE_OK | KLF_REORDER);
	//		//ActivateKeyboardLayout(hkl1, 0);
	//		//printf("Current keyboard: %s\n", name);
	//		return 1;
	//	}

	//}

	if (config.iAmAMac) {
		// Eat accidental left/right presses after home/end on Mac
		static int eatNextCorrespondingKey;
		static DWORD releaseTime;

		if (wParam == WM_KEYDOWN && nKey == eatNextCorrespondingKey) {
			// Key to be eaten
			DWORD nowTime = GetTickCount();
			eatNextCorrespondingKey = 0;
			if (nowTime - releaseTime <= 10) {
				return 1;
			}
		}
		else if (wParam == WM_KEYUP) {
			for (int i = 0; i < numberof(keyboardEatTable); i++) {
				if (nKey == keyboardEatTable[i].original) {
					eatNextCorrespondingKey = keyboardEatTable[i].modified;
					releaseTime = GetTickCount();
					break;
				}
				else if (nKey == keyboardEatTable[i].modified) {
					eatNextCorrespondingKey = keyboardEatTable[i].original;
					releaseTime = GetTickCount();
					break;
				}
			}
		}
	}

	// LWin+[0-9] -> Ctrl+Win+[0-9]
	if (config.noNumPad && !injected) {
		if (lWinPressed && nKey >= '0' && nKey <= '9' && wParam == WM_KEYDOWN) {
			if (!shiftPressed() && !ctrlPressed() && config.multiDesktopLikeApplicationSwitcher) {
				// Press Ctrl (http://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct)
				kbddown(VK_RCONTROL, 0);
				kbddown(nKey, 0);
				kbdup(VK_RCONTROL, 0);
				return 1;
			}
		}
	}

	if (config.rightShiftContextMenu) {
		static bool pressedAnotherKeySince = false;
		// Remappe Alt droit => context menu
		if (nKey == VK_RSHIFT) {
			if (wParam == WM_KEYDOWN && !injected) {
				pressedAnotherKeySince = false;
			}
			else if (wParam == WM_KEYUP && !pressedAnotherKeySince) {
				// Au keyup, on presse un context menu (93)
				RunAfterDelay([] {
					kbdpress(VK_APPS, 0);
				});
			}
		}
		else if (wParam == WM_KEYDOWN)
			pressedAnotherKeySince = true;
	}

	if (config.altGraveToStickyAltTab) {
		//` = OEM_3 (0xC0)
		if (nKey == VK_OEM_3) {
			// SYSKEYDOWN means Alt is pressed
			if (wParam == WM_SYSKEYDOWN) {
				bool needsCtrl = !ctrlPressed();
				if (needsCtrl) kbddown(VK_RCONTROL, 0);
				kbdpress(VK_TAB, 0);
				if (needsCtrl) kbdup(VK_RCONTROL, 0);
			}
			// Do not propagate the ` char
			if (wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP) return 1;
		}
	}

	// TODO parameter
	if (config.disableWinTabAnimation) {
		if (nKey == VK_TAB && lWinPressed && wParam == WM_KEYDOWN) {
			disableAnimationsForDurationOfWinTab();
		}
	}

	if (nKey >= VK_NUMPAD0 && nKey <= VK_NUMPAD9 && config.multiDesktopLikeApplicationSwitcher) {
		if (wParam == WM_KEYDOWN && ctrlPressed()) {
			int taskId = nKey - VK_NUMPAD0 + '0';
			bool needsWin = !winPressed();
			// Ctrl+Win+[taskId]
			if (needsWin) kbddown(VK_RWIN, 0);
			kbdpress(taskId, 0);
			if (needsWin) kbdup(VK_RWIN, 0);
			return 1;
		}
	}

	if (config.useSoftMediaKeys) {
		if (lCtrlPressed && lWinPressed && wParam == WM_KEYDOWN) {
			switch (nKey) {
			case VK_HOME:
				kbdpress(VK_MEDIA_STOP, 0);
				return 1;
			case VK_END:
				kbdpress(VK_MEDIA_PLAY_PAUSE, 0);
				return 1;
			case VK_PRIOR:
				kbdpress(VK_MEDIA_PREV_TRACK, 0);
				return 1;
			case VK_NEXT:
				kbdpress(VK_MEDIA_NEXT_TRACK, 0);
				return 1;
			case VK_UP:
				AudioMixer::incrementVolume(config.volumeIncrementQuantity);
				return 1;
			case VK_DOWN:
				AudioMixer::decrementVolume(config.volumeIncrementQuantity);
				return 1;
			}
		}
	}

	// wParam will contain the virtual key code.  
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void KbdHook::start() {
	kbdup(VK_LCONTROL, 0);
	kbdup(VK_RCONTROL, 0);
	kbdup(VK_LWIN, 0);
	kbdup(VK_RWIN, 0);
	//kbdup(VK_APPS, 0);
	kbdup(VK_LMENU, 0);
	kbdup(VK_RMENU, 0);
	kbdup(VK_LSHIFT, 0);
	kbdup(VK_RSHIFT, 0);
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (config.ddcCiBrightnessControl)
		Monitor::init(config.autoApplyGammaCurveDelay);
}

void KbdHook::terminate() {
	UnhookWindowsHookEx(g_hHook);
}
