#include "Precompiled.h"
#include "KbdHook.h"
#include "AudioMixer.h"
#include "Config.h"
#include "Monitor.h"
#include "WindowsExplorer.h"
#include "Utilities.h"
#include "StatusWindow.h"
#include "DisableAnimationsForWinTab.h"

static void TEMP_log(const char *format, ...) {
	char vbuf[256];
	va_list arg_ptr;
	va_start(arg_ptr, format);
	vsnprintf(vbuf, sizeof(vbuf), format, arg_ptr);
	FILE *f = fopen("log.txt", "a");
	if (f) {
		fputs(vbuf, f);
		fputc('\n', f);
		fclose(f);
	}
}

// TODO move around
// Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html or https://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct
static void kbddown(int vkCode, BYTE scanCode, int flags = 0) {
	keybd_event(vkCode, scanCode, flags, 0);
}

static void kbdup(int vkCode, BYTE scanCode, int flags = 0) {
	keybd_event(vkCode, scanCode + 0x80, flags | KEYEVENTF_KEYUP, 0);
}

static void kbdpress(int vkCode, BYTE scanCode, int flags = 0) {
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

static HHOOK g_hPreviousHook;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	static bool lCtrlPressed = false, rCtrlPressed = false, lWinPressed = false, lShiftPressed = false, rShiftPressed = false;
	static int numkeysDown = 0;
	static bool winKeyWasForced = false;

	// Not something for us
	if (nCode != HC_ACTION) {
		return CallNextHookEx(g_hPreviousHook, nCode, wParam, lParam);
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
		case VK_LWIN:
			lWinPressed = wParam == WM_KEYDOWN;
			break;
		case VK_RSHIFT:
			rShiftPressed = wParam == WM_KEYDOWN;
			break;
		}
	}

	// Ignore injected input
	if (kbd->flags & (LLKHF_INJECTED | LLKHF_LOWER_IL_INJECTED)) {
		return CallNextHookEx(g_hPreviousHook, nCode, wParam, lParam);
	}

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
				AudioMixer::decrementVolume(config.volumeIncrementQuantity);
				processed = 0xae;
			} else if (nKey == 0xaf) {
				// Molette +
				AudioMixer::incrementVolume(config.volumeIncrementQuantity);
				processed = 0xaf;
			}

			// Rel�che la touche, il ne faut pas que Windows la prenne
			if (processed != 0) {
				kbdup(processed, 0);
				return 1;
			}
		}

		// Quit
		if (config.closeWindowWithWinQ && lWinPressed) {
			if (nKey == 'Q') {
				//HWND window = GetForegroundWindow();
				//SendMessage(window, WM_SYSCOMMAND, SC_CLOSE, 0);
				kbddown(VK_LCONTROL, 0); //CONTROL, to avoid bringing the menu
				kbdup(VK_LWIN, 0); // WIN
				kbdup(VK_LCONTROL, 0); // CONTROL, to avoid bringing the menu
				kbddown(VK_LMENU, 0); //ALT
				kbddown(VK_F4, 0); // +F4
				kbdup(VK_LMENU, 0);
				kbdup(VK_F4, 0);

				kbddown(VK_LWIN, 0); // Restore WIN
				kbddown(VK_LCONTROL, 0); //CONTROL, to avoid bringing the menu
				kbdup(VK_LCONTROL, 0);
				return 1;
			}
			//else if (nKey == 'W') {
			//	keybd_event(VK_CONTROL, 0, 0, 0);  //CONTROL, to avoid bringing the menu
			//	keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0); //WIN
			//	keybd_event(VK_F4, 0, 0, 0);  //F4
			//	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);  //CONTROL, to avoid bringing the menu
			//	keybd_event(VK_F4, 0, KEYEVENTF_KEYUP, 0); //F4
			//	keybd_event(VK_LWIN, 0, 0, 0); //WIN
			//	return 1;
			//}
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
				TCHAR reexecute[1024];
				PROCESS_INFORMATION pi;
				STARTUPINFO si;
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof si;
				GetModuleFileName(NULL, reexecute, 1024);
				CreateProcess(reexecute, NULL,
					NULL, NULL, FALSE, 0, NULL,
					NULL, &si, &pi);
				return 1;
			}
		}

		if (config.winFOpensYourFiles && lWinPressed && nKey == 'F') {
			kbdpress(VK_LCONTROL, 0);
			WindowsExplorer::showHomeFolderWindow();
			return 1;
		}
		if (config.winHHidesWindow && lWinPressed && nKey == 'H') {
			kbdpress(VK_LCONTROL, 0); // To avoid bringing the menu
			ShowWindow(GetForegroundWindow(), SW_MINIMIZE);
			return 1;
		}
	}

	//if (config.altGrContextMenu) {
	//	static bool bringMenuAtNextKeyUp = false;
	//	// Remappe Alt droit => context menu
	//	if (nKey == VK_RMENU) {
	//		if (wParam == WM_SYSKEYDOWN) {
	//			bringMenuAtNextKeyUp = true;
	//		} else if (wParam == WM_KEYUP && bringMenuAtNextKeyUp) {
	//			// Au keyup, on presse un context menu
	//			bringMenuAtNextKeyUp = false;
	//			kbdup(nKey, 0);
	//			kbdpress(VK_APPS, 0);
	//			return 1;
	//		}
	//	} else if (nKey != VK_LCONTROL) {
	//		bringMenuAtNextKeyUp = false;
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
	if (config.noNumPad) {
		static bool needToReleaseCtrl = false;
		if (lWinPressed && nKey >= '0' && nKey <= '9' && wParam == WM_KEYDOWN && !winKeyWasForced) {
			if (!needToReleaseCtrl && !lShiftPressed && !lCtrlPressed && config.multiDesktopLikeApplicationSwitcher) {
				needToReleaseCtrl = true;
				// Press Ctrl (http://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct)
				kbddown(VK_LCONTROL, 0x9d);
			}
		}
		if (needToReleaseCtrl && nKey == VK_LWIN && wParam == WM_KEYUP) {
			// Release Ctrl
			kbdup(VK_LCONTROL, 0x9d);
			needToReleaseCtrl = false;
		}
	}

	if (config.rightShiftContextMenu) {
		static bool pressedAnotherKeySince = false;
		// Remappe Alt droit => context menu
		if (nKey == VK_RSHIFT) {
			if (wParam == WM_KEYDOWN)
				pressedAnotherKeySince = false;
			// Au keyup, on presse un context menu (93)
			else if (wParam == WM_KEYUP  && rShiftWasPressed && !pressedAnotherKeySince) {
//				SimulateKeyUp(VK_RCONTROL);		// Fix for the apps which do not accept context menu with CTRL pressed
				kbdpress(VK_APPS, 0);
			}
		}
		else if (wParam == WM_KEYDOWN)
			pressedAnotherKeySince = true;
	}

	if (config.rightCtrlContextMenu) {
		// Remappe Ctrl droit => context menu
		if (nKey == VK_RCONTROL) {
			// Au keyup, on presse un context menu (93)
			if (wParam == WM_KEYUP  && ctrlKeyWasPressed) {
//				SimulateKeyUp(VK_RCONTROL);		// Fix for the apps which do not accept context menu with CTRL pressed
				kbdpress(VK_APPS, 0);
			}
		}
	}

	if (config.altGraveToStickyAltTab) {
		//` = OEM_3 (0xC0)
		if (nKey == VK_OEM_3) {
			// SYSKEYDOWN means Alt is pressed
			if (wParam == WM_SYSKEYDOWN) {
				kbddown(VK_CONTROL, 0);
				kbdpress(VK_TAB, 0);
				kbdup(VK_CONTROL, 0);
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

	if (config.winTSelectsLastTask) {
		static bool winTWasGenerated = false;
		winTWasGenerated = winTWasGenerated && lWinPressed;
		if (lWinPressed && nKey == 'T' && wParam == WM_KEYDOWN && !winTWasGenerated) {
			kbdpress('T', 0);
			kbdpress('T', 0);
			winTWasGenerated = true;
			RunAfterDelay([] {
				kbdpress(VK_END, 0);
			}, 50);
			return 1;
		}
	}

	// Win+[NUMPAD0-9] -> Win+[0-9]
	if (nKey >= VK_NUMPAD0 && nKey <= VK_NUMPAD9 && config.multiDesktopLikeApplicationSwitcher) {
		if ((lCtrlPressed || rCtrlPressed) && wParam == WM_KEYDOWN) {
			if (numkeysDown == 0 && !lWinPressed) {
				kbddown(VK_LWIN, 0);
				winKeyWasForced = true;
			}
			kbddown(nKey - VK_NUMPAD0 + '0', 0);
			numkeysDown++;
			return 1;
		} else if (wParam == WM_KEYUP) {
			if (numkeysDown > 0) {
				kbdup(nKey - VK_NUMPAD0 + '0', 0);
				if (--numkeysDown == 0 && winKeyWasForced) {
					kbdup(VK_LWIN, 0);
					winKeyWasForced = false;
				}
				return 1;
			}
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
	return CallNextHookEx(g_hPreviousHook, nCode, wParam, lParam);
}

void KbdHook::start() {
	kbdup(VK_LCONTROL, 0);
	kbdup(VK_RCONTROL, 0);
	kbdup(VK_LWIN, 0);
	kbdup(VK_RWIN, 0);
	//kbdup(VK_APPS, 0);
	kbdup(VK_LMENU, 0);
	kbdup(VK_RMENU, 0);
	g_hPreviousHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (config.ddcCiBrightnessControl)
		Monitor::init(config.autoApplyGammaCurveDelay);
}