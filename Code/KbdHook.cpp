// Something to know: Ctrl+Win+Space switches between the last two languages
#include "Precompiled.h"
#include "KbdHook.h"
#include "AudioMixer.h"
#include "Config.h"
#include "KbdLayoutTranslators.h"
#include "Main.h"
#include "Monitor.h"
#include "WindowsExplorer.h"
#include "Utilities.h"
#include "StatusWindow.h"
#include "DisableAnimationsForWinTab.h"
#include "PowrProf.h"

static bool lCtrlPressed = false, rCtrlPressed = false, lWinPressed = false, rWinPressed = false, lShiftPressed = false, rShiftPressed = false, lAltPressed = false;
static bool ctrlPressed() { return lCtrlPressed || rCtrlPressed; }
static bool winPressed() { return lWinPressed || rWinPressed; }
static bool shiftPressed() { return lShiftPressed || rShiftPressed; }
static bool altPressed() { return lAltPressed; }
// Only those two, not the others
static bool ctrlWinPressed() { return lCtrlPressed && lWinPressed && !rWinPressed && !rCtrlPressed && !shiftPressed() && !altPressed(); }
static bool ctrlWinAndMaybeShiftPressed() { return lCtrlPressed && lWinPressed && !rWinPressed && !rCtrlPressed && !altPressed(); }
static bool winOnlyPressed() { return lWinPressed && !rWinPressed && !ctrlPressed() && !shiftPressed() && !altPressed(); }
static void cancelAllKeys();

// Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html or https://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct
void kbddown(int vkCode, BYTE scanCode, int flags) {
	keybd_event(vkCode, scanCode, flags, 0);
}

void kbdup(int vkCode, BYTE scanCode, int flags) {
	keybd_event(vkCode, scanCode, flags | KEYEVENTF_KEYUP, 0);
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

void sendNextPageCommand() {
	bool needsAlt = !altPressed();
	if (needsAlt) kbddown(VK_LMENU, 0);
	kbdpress(VK_RIGHT, 0);
	if (needsAlt) kbdup(VK_LMENU, 0);
}

static void switchToHiragana() {
	bool needsShift = !shiftPressed(), needsControl = !ctrlPressed();
	if (needsShift) kbddown(VK_RCONTROL, 0);
	kbdpress(VK_CAPITAL, 0);
	kbdpress(VK_CAPITAL, 0);
	if (needsShift) kbdup(VK_RCONTROL, 0);
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

static void sendAltShift() {
	// Replace by Alt+Shift
	bool needLWin = lWinPressed, needRWin = rWinPressed;
	kbddown(VK_LMENU, 0);
	if (needLWin) kbdup(VK_LWIN, 0);
	if (needRWin) kbdup(VK_RWIN, 0);
	kbdpress(VK_LSHIFT, 0);
	if (needLWin) kbddown(VK_LWIN, 0);
	if (needRWin) kbddown(VK_RWIN, 0);
	kbdup(VK_LMENU, 0);
}

static UINT getCurrentLayout() {
	HWND fore = GetForegroundWindow();
	UINT tpid = GetWindowThreadProcessId(fore, NULL);
	HKL hKL = GetKeyboardLayout(tpid);
	return LOWORD(hKL);
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
	// Ignore injected input
	bool injected = (kbd->flags & (LLKHF_INJECTED | LLKHF_LOWER_IL_INJECTED));

#ifdef _DEBUG
	if (wParam == WM_KEYDOWN)
		printf("Down%s: %x %x\n", injected ? " (inj.)" : "", nKey, kbd->scanCode);
	if (wParam == WM_KEYUP)
		printf("Up%s: %x %x\n", injected ? " (inj.)" : "", nKey, kbd->scanCode);
	if (wParam == WM_SYSKEYDOWN)
		printf("Sysdown%s: %x %x\n", injected ? " (inj.)" : "", nKey, kbd->scanCode);
	if (wParam == WM_SYSKEYUP)
		printf("Sysup%s: %x %x\n", injected ? " (inj.)" : "", nKey, kbd->scanCode);
#endif

	// Keyboard translation services, must be run before everyone else
	if (config.japaneseMacKeyboard && !injected) {
		if (nKey == 0xEB) {
			// 英 -> Lwin
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0x5B, 0x5B);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0x5B, 0x5B);
			return 1;
		}
		if (nKey == 0x14) {
			// Caps -> ctrl
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA2, 0x1D);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA2, 0x1D);
			return 1;
		}
		if (nKey == 0x5B) {
			// Lwin -> Loption
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA4, 0x38);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA4, 0x38);
			return 1;
		}
		if (nKey == 0xA2 && kbd->scanCode == 0x1D) {
			// Lctrl -> Caps
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0x14, 0x3A);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0x14, 0x3A);
			return 1;
		}
		if (nKey == 0xFF) {
			// かな -> Ralt
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
				kbddown(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				kbdup(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			return 1;
		}
		if (nKey == 0xA4) {
			// Loption -> Ctrl
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA2, 0x1D);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA2, 0x1D);
			return 1;
		}
		// If a key has been remapped, we'll never go further (return 1)
	}

	if (config.japaneseWindowsKeyboard && !injected) {
		if (nKey == 0xEB) {
			// 無変換 -> Lalt
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA4, 0x38);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA4, 0x38);
			return 1;
		}
		if (nKey == 0xFF && kbd->scanCode == 0x79) {
			// 変換 -> Ralt
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			return 1;
		}
		if (nKey == 0xFF && kbd->scanCode == 0x70) {
			// ｶﾀｶﾅ -> Rwin
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0x5C, 0x5C);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0x5C, 0x5C);
			return 1;
		}
		// If a key has been remapped, we'll never go further (return 1)
	}

	if (config.japaneseMacBookPro && !injected) {
		static bool virtualFnIsDown = false;
		static int fnRemappings[][2] = {
			{ VK_LEFT, VK_HOME },
			{ VK_RIGHT, VK_END },
			{ VK_UP, VK_PRIOR },
			{ VK_DOWN, VK_NEXT },
			{ VK_BACK, VK_DELETE },
			{ VK_F7, VK_MEDIA_PREV_TRACK },
			{ VK_F8, VK_MEDIA_PLAY_PAUSE },
			{ VK_F9, VK_MEDIA_NEXT_TRACK },
			{ VK_F10, VK_VOLUME_MUTE },
			{ VK_F11, VK_VOLUME_DOWN },
			{ VK_F12, VK_VOLUME_UP },
		};
		static bool virtualKeysActive[numberof(fnRemappings)] = { 0 };
		if (nKey == 0x14) {
			// Fn
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) virtualFnIsDown = true;
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				virtualFnIsDown = false;
				// Release all keys that were pressed during the time Fn was down
				for (int i = 0; i < numberof(virtualKeysActive); i++) {
					if (virtualKeysActive[i]) kbdup(fnRemappings[i][1], 0);
					virtualKeysActive[i] = false;
				}
			}
			return 1;
		}
		if (virtualFnIsDown) {
			int foundIndex = -1;
			for (int i = 0; i < numberof(fnRemappings); i++) {
				if (nKey == fnRemappings[i][0]) {
					foundIndex = i;
					break;
				}
			}
			if (foundIndex >= 0) {
				int destKey = fnRemappings[foundIndex][1];
				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
					virtualKeysActive[foundIndex] = true;
					kbddown(destKey, 0);
					return 1;
				}
				if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
					virtualKeysActive[foundIndex] = false;
					kbdup(destKey, 0);
					return 1;
				}
			}
		}
		if (nKey == 0xEB) {
			// 英 -> Lcommand
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0x5B, 0x5B);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0x5B, 0x5B);
			return 1;
		}
		if (nKey == 0x5B) {
			// Lcommand -> Lalt
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA4, 0x38);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA4, 0x38);
			return 1;
		}
		if (nKey == 0xFF) {
			// かな -> Ralt
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
				kbddown(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				kbdup(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			return 1;
		}
		if (nKey == 0xA4) {
			// Loption -> Ctrl
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0xA2, 0x1D);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0xA2, 0x1D);
			return 1;
		}
		if (nKey == 0xA2 && kbd->scanCode == 0x1D) {
			// Lctrl -> Caps
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) kbddown(0x14, 0x3A);
			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) kbdup(0x14, 0x3A);
			return 1;
		}
		// If a key has been remapped, we'll never go further (return 1)
	}

	if (config.internationalUsKeyboardForFrench) {
		bool isDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
		bool isUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;

		layoutTranslatorsRegister();
		if (isDown) {
			if (layoutTranslatorsEnUs.processKeyDown(nKey, shiftPressed())) {
				return 1;
			}
		}
		if (isUp) {
			if (layoutTranslatorsEnUs.processKeyUp(nKey)) {
				return 1;
			}
		}
	}

	// Special keys
	if (wParam == WM_KEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP) {
		bool isDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
		switch (nKey) {
		case VK_LCONTROL:
			lCtrlPressed = isDown;
			break;
		case VK_RCONTROL:
			rCtrlPressed = isDown;
			break;
		case VK_LSHIFT:
			lShiftPressed = isDown;
			break;
		case VK_RSHIFT:
			rShiftPressed = isDown;
			break;
		case VK_LWIN:
			lWinPressed = isDown;
			break;
		case VK_RWIN:
			rWinPressed = isDown;
			break;
		case VK_LMENU:
			lAltPressed = isDown;
			break;
		//case VK_RMENU: // used by international keyboards
		//	rAltPressed = isDown;
		//	break;
		}
	}

	//if (config.startScreenSaverWithInsert && !injected) {
	//	DidPerformAnAction();
	//}

	if (wParam == WM_KEYDOWN) {
		// Win+L triggers a key down but not up, and no up for Win so get aware of that
		if (nKey == 'L' && winPressed()) {
			lWinPressed = rWinPressed = false;
		}

		// Insert -> start screen saver & lock
		if (config.startScreenSaverWithInsert) {
			if (nKey == VK_INSERT) {
				//LockMachineOnNextAction();
				SendMessage(GetForegroundWindow(), WM_SYSCOMMAND, SC_SCREENSAVE, 0);
				return 1;
			}
		}

		// Ctrl+H -> toggle hide/show hidden folders
		if (config.toggleHideFolders) {
			if (nKey == 'H' && ctrlPressed()) {
				if (WindowsExplorer::isActive()) {
					WindowsExplorer::toggleShowHideFolders();
					// Refresh the explorer
					kbdpress(VK_F5, 0, 0);
				}
			}
		}

		// Logarithmic volume management
		if (config.smoothVolumeControl) {
			int processed = 0;
			if (nKey == 0xae) {
				// Molette -
				RunAfterDelay([] { AudioMixer::decrementVolume(config.volumeIncrementQuantity); });
				processed = 0xae;
			} else if (nKey == 0xaf) {
				// Molette +
				RunAfterDelay([] { AudioMixer::incrementVolume(config.volumeIncrementQuantity); });
				processed = 0xaf;
			}

			// Relâche la touche, il ne faut pas que Windows la prenne
			if (processed != 0) {
				kbdup(processed, 0);
				return 1;
			}
		}

		// External monitor brightness change
		if (ctrlWinAndMaybeShiftPressed()) {
			if (config.ddcCiBrightnessControl) {
				if (nKey == VK_F9) {
					int qty = lShiftPressed ? 1 : config.brightnessIncrementQuantity;
					RunAfterDelay([qty] {
						Monitor::decreaseBrightnessBy(qty);
					});
					return 1;
				}
				else if (nKey == VK_F10) {
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

			if (config.useSoftMediaKeys) {
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
				case VK_F12:
					AudioMixer::incrementVolume(config.volumeIncrementQuantity);
					return 1;
				case VK_F11:
					AudioMixer::decrementVolume(config.volumeIncrementQuantity);
					return 1;
				}
			}
		}

		// Win only
		if (winOnlyPressed()) {
			if (config.doNotUseWinSpace && nKey == VK_SPACE) {
				// Replace by Alt+Shift
				sendAltShift();

				if (config.selectHiraganaByDefault) {
					RunAfterDelay([] {
						if (getCurrentLayout() == 0x0411) {
							switchToHiragana();
						}
					}, 200);
				}
				return 1;
			}

			// Win+DOT also working in Japanese layout (not so useful for now)
			if (config.doNotUseWinSpace && nKey == 0xBE && getCurrentLayout() == 0x0411) {
				sendAltShift();
				RunAfterDelay([] {
					bool needsWin = !winPressed();
					if (needsWin) kbddown(VK_RWIN, 0);
					kbdpress(0xBE, 0x34);
					if (needsWin) kbdup(VK_RWIN, 0);
				}, 100);
				return 1;
			}

			if (config.winFOpensYourFiles && nKey == 'F') {
				if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0);
				WindowsExplorer::showHomeFolderWindow();
				return 1;
			}
			if (config.winHHidesWindow && nKey == 'H') {
				if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0); // To avoid bringing the menu
				char className[128], title[128];
				HWND hWnd = GetForegroundWindow();
				GetClassNameA(hWnd, className, 128);
				GetWindowTextA(hWnd, title, 128);
				if (strcmp(className, "Windows.UI.Core.CoreWindow") /*|| strcmp(title, "Search")*/) {
					ShowWindow(GetForegroundWindow(), SW_MINIMIZE);
				}
				return 1;
			}

			if (config.closeWindowWithWinQ && nKey == 'Q') {
				if (!injected) {
					// Normal machine
					bool needAlt = !lAltPressed;
					if (needAlt) kbddown(VK_LMENU, 0);
					kbdup(VK_LWIN, 0); // Temporarily release WIN since Win+Alt+F4 does nothing
					kbdpress(VK_F4, 0); // +F4
					kbddown(VK_LWIN, 0); // Re-enable WIN
					if (needAlt) kbdup(VK_LMENU, 0);
				}
				else {
					// TeamViewer support
					if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0); // To avoid bringing the menu
					SendMessage(GetForegroundWindow(), WM_CLOSE, 0, 0);
				}
				return 1;
			}

			if (config.winSSuspendsSystem && !injected && nKey == 'S') {
				SetSuspendState(false, false, false);
				return 1;
			}
		}

		if (config.winTSelectsLastTask) {
			 static bool inFunction = false;
			// // Check that we are still in the task bar and restart function (Win+T from start) if not
			 if (inFunction) {
			 	char className[128];
			 	GetClassNameA(GetForegroundWindow(), className, 128);
				if (strcmp(className, "Shell_TrayWnd")) {
					printf("Not anymore\n");
					inFunction = false;
				}
			}
			if (inFunction) {
				if (!winPressed()) {
					if (nKey >= '5' && nKey <= '7') {
						moveToTask(11 + (nKey - '5') * 10, START);
						return 1;
					}
					else if (nKey == VK_PRIOR) {
						moveToTask(-config.winTTaskMoveBy, CURRENT);
						return 1;
					}
					else if (nKey == VK_NEXT) {
						moveToTask(+config.winTTaskMoveBy, CURRENT);
						return 1;
					}
				}
			}
			else if (winOnlyPressed() && nKey == 'T') {
				inFunction = true;
				kbdpress('B', 0);
				// First press on Win+T
				RunAfterDelay([] {
					kbdpress('T', 0);
					kbdpress(VK_END, 0);
				}, 10);
				return 1;
			}
		}
	}

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

#if PRE_WINDOWS_1803_UPDATE
	// LWin+[0-9] -> Ctrl+Win+[0-9]
	if (config.noNumPad /*&& !injected*/ && !rCtrlPressed) {
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
#endif

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
		if (nKey == VK_OEM_3 && !ctrlPressed()) {
			// SYSKEYDOWN means Alt is pressed
			if (wParam == WM_SYSKEYDOWN) {
				kbddown(VK_RCONTROL, 0);
				kbdpress(VK_TAB, 0);
				kbdup(VK_RCONTROL, 0);
			}
			// Do not propagate the ` char
			if (wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP) return 1;
		}
	}

	// TODO parameter
	if (config.disableWinTabAnimation) {
		if (nKey == VK_TAB && winPressed() && wParam == WM_KEYDOWN) {
			disableAnimationsForDurationOfWinTab();
		}
	}

#if PRE_WINDOWS_1803_UPDATE
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
#else
	// Horrible hack because the new taskbar is coded with feet
	if (config.multiDesktopLikeApplicationSwitcher) {
		static int lCtrlPressCount = 0, rCtrlPressCount = 0;

		if (wParam == WM_KEYUP) {
			if (nKey == VK_LCONTROL && lCtrlPressCount > 0) {
				if (--lCtrlPressCount == 0) {
					kbdup(VK_RWIN, 0);
					return 1;
				}
			}
			if (nKey == VK_RCONTROL && rCtrlPressCount > 0) {
				if (--rCtrlPressCount == 0) {
					kbdup(VK_RWIN, 0);
					return 1;
				}
			}
		}

		if (wParam == WM_KEYDOWN) {
			if (nKey >= VK_NUMPAD0 && nKey <= VK_NUMPAD9) {
				int taskId = nKey - VK_NUMPAD0 + '0';
				// Key repeats
				if (lCtrlPressCount > 0 || rCtrlPressCount > 0) {
					kbdpress(taskId, 0);
					return 1;
				}

				if (ctrlPressed() && !winPressed()) {
					bool needsWin = !winPressed();
					lCtrlPressCount = lCtrlPressed ? 2 : 0;
					rCtrlPressCount = rCtrlPressed ? 2 : 0;

					if (lCtrlPressCount) kbdup(VK_LCONTROL, 0);
					if (rCtrlPressCount) kbdup(VK_RCONTROL, 0);
					// Ctrl+Win+[taskId]
					kbddown(VK_RWIN, 0);
					kbdpress(taskId, 0);
					return 1;
				}
			}

			// In case another key was pressed while the win key was artificially switched
			if (!(nKey >= VK_NUMPAD0 && nKey <= VK_NUMPAD9 || nKey >= '0' && nKey <= '9' || nKey == VK_LCONTROL || nKey == VK_RCONTROL || nKey == VK_RWIN)) {
				if (lCtrlPressCount > 0) {
					lCtrlPressCount = 0;
					kbddown(VK_LCONTROL, 0);
					kbdup(VK_RWIN, 0);
					kbddown(nKey, (BYTE)kbd->scanCode, kbd->flags);
					return 1;
				}
				if (rCtrlPressCount > 0) {
					rCtrlPressCount = 0;
					kbddown(VK_RCONTROL, 0);
					kbdup(VK_RWIN, 0);
					kbddown(nKey, (BYTE)kbd->scanCode, kbd->flags);
					return 1;
				}
			}
		}
	}
#endif

	// wParam will contain the virtual key code.  
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void cancelAllKeys() {
	kbdup(VK_LCONTROL, 0);
	kbdup(VK_RCONTROL, 0);
	kbdup(VK_LWIN, 0);
	kbdup(VK_RWIN, 0);
	//kbdup(VK_APPS, 0);
	kbdup(VK_LMENU, 0);
	kbdup(VK_RMENU, 0);
	kbdup(VK_LSHIFT, 0);
	kbdup(VK_RSHIFT, 0);
}

void KbdHook::start() {
	cancelAllKeys();
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (config.ddcCiBrightnessControl)
		Monitor::init(config.autoApplyGammaCurveDelay);
}

void KbdHook::terminate() {
	if (config.ddcCiBrightnessControl)
		Monitor::terminate();
	UnhookWindowsHookEx(g_hHook);
	cancelAllKeys();
}
