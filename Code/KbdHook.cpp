#include "Precompiled.h"
#include "KbdHook.h"
#include "AudioMixer.h"
#include "Config.h"
#include "Monitor.h"
#include "WindowsExplorer.h"
#include "Utilities.h"
#include "StatusWindow.h"

static void SimulateKeyAction(int code, int state);
static void SimulateKeyDown(int code);
static void SimulateKeyUp(int code);
static void SimulateKeyPress(int code);

static const struct { int original; int modified; } keyboardEatTable[] = {
	VK_LEFT, VK_HOME,
	VK_RIGHT, VK_END,
	VK_UP, VK_PRIOR,
	VK_DOWN, VK_NEXT,
	VK_BACK, VK_DELETE
};


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	static bool lCtrlPressed = false, rCtrlPressed = false, lWinPressed = false, lShiftPressed = false;
	static int numkeysDown = 0;
	static bool winKeyWasForced = false;

	// Not something for us
	if (nCode != HC_ACTION) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT*) lParam;
	int nKey = kbd->vkCode;
	int processed = 0;
	bool ctrlKeyWasPressed = lCtrlPressed || rCtrlPressed;

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
		}
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
					SimulateKeyPress(VK_F5);
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

			// Relâche la touche, il ne faut pas que Windows la prenne
			if (processed != 0) {
				SimulateKeyAction(processed, 0);
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
			WindowsExplorer::showHomeFolderWindow();
			SimulateKeyPress(VK_LCONTROL);
			return 1;
		}
		if (config.winHHidesWindow && lWinPressed && nKey == 'H') {
			ShowWindow(GetForegroundWindow(), SW_FORCEMINIMIZE);
			return 1;
		}
	}

	if (config.iAmAMac) {
		static bool bringMenuAtNextKeyUp = false;
		// Remappe Alt droit => context menu
		if (nKey == VK_RMENU) {
			if (wParam == WM_SYSKEYDOWN) {
				bringMenuAtNextKeyUp = true;
			} else if (wParam == WM_KEYUP && bringMenuAtNextKeyUp) {
				// Au keyup, on presse un context menu
				bringMenuAtNextKeyUp = false;
				SimulateKeyUp(nKey);
				SimulateKeyPress(93);
				return 1;
			}
		} else if (nKey != VK_LCONTROL) {
			bringMenuAtNextKeyUp = false;
		}

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
		} else if (wParam == WM_KEYUP) {
			for (int i = 0; i < numberof(keyboardEatTable); i++) {
				if (nKey == keyboardEatTable[i].original) {
					eatNextCorrespondingKey = keyboardEatTable[i].modified;
					releaseTime = GetTickCount();
					break;
				} else if (nKey == keyboardEatTable[i].modified) {
					eatNextCorrespondingKey = keyboardEatTable[i].original;
					releaseTime = GetTickCount();
					break;
				}
			}
		}

		// LWin+[0-9] -> Ctrl+Win+[0-9]
		static bool needToReleaseCtrl = false;
		if (lWinPressed && nKey >= '0' && nKey <= '9' && wParam == WM_KEYDOWN && !winKeyWasForced) {
			if (!needToReleaseCtrl && !lShiftPressed && !lCtrlPressed && config.multiDesktopLikeApplicationSwitcher) {
				needToReleaseCtrl = true;
				// Press Ctrl (http://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct)
				keybd_event(VK_CONTROL, 0x9d, 0, 0);
			}
		}
		if (needToReleaseCtrl && nKey == VK_LWIN && wParam == WM_KEYUP) {
			// Release Ctrl
			keybd_event(VK_CONTROL, 0x9d, KEYEVENTF_KEYUP, 0);
			needToReleaseCtrl = false;
		}
	} else {
		if (config.rightCtrlContextMenu) {
			// Remappe Ctrl droit => context menu
			if (nKey == VK_RCONTROL) {
				// Au keyup, on presse un context menu (93)
				if (wParam == WM_KEYUP  && ctrlKeyWasPressed) {
					//				SimulateKeyUp(VK_RCONTROL);		// Fix for the apps which do not accept context menu with CTRL pressed
					SimulateKeyPress(93);
				}
			}
		}
	}

	// Win+[NUMPAD0-9] -> Win+[0-9]
	if (nKey >= VK_NUMPAD0 && nKey <= VK_NUMPAD9 && config.multiDesktopLikeApplicationSwitcher) {
		if ((lCtrlPressed || rCtrlPressed) && wParam == WM_KEYDOWN) {
			if (numkeysDown == 0 && !lWinPressed) {
				SimulateKeyDown(VK_LWIN);
				winKeyWasForced = true;
			}
			SimulateKeyDown(nKey - VK_NUMPAD0 + '0');
			numkeysDown++;
			return 1;
		} else if (wParam == WM_KEYUP) {
			if (numkeysDown > 0) {
				SimulateKeyUp(nKey - VK_NUMPAD0 + '0');
				if (--numkeysDown == 0 && winKeyWasForced) {
					SimulateKeyUp(VK_LWIN);
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
				SimulateKeyDown(VK_MEDIA_STOP);
				return 1;
			case VK_END:
				SimulateKeyDown(VK_MEDIA_PLAY_PAUSE);
				return 1;
			case VK_PRIOR:
				SimulateKeyDown(VK_MEDIA_PREV_TRACK);
				return 1;
			case VK_NEXT:
				SimulateKeyDown(VK_MEDIA_NEXT_TRACK);
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

void SimulateKeyDown(int code) {
	keybd_event(code,
		0x45,
		KEYEVENTF_EXTENDEDKEY | 0,
		0);
}

void SimulateKeyUp(int code) {
	keybd_event(code,
		0x45,
		KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
		0);
}

void SimulateKeyPress(int code) {
	SimulateKeyDown(code);
	SimulateKeyUp(code);
}

void SimulateKeyAction(int code, int state) {
	if (state)
		// Simulate a key press
		keybd_event(code, 0x45, KEYEVENTF_EXTENDEDKEY, 0);
	else
		// Simulate a key release
		keybd_event(code, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

void KbdHook::start() {
	SimulateKeyUp(VK_LCONTROL);
	SimulateKeyUp(VK_RCONTROL);
	SimulateKeyUp(VK_LWIN);
	SimulateKeyUp(VK_RWIN);
	SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (config.ddcCiBrightnessControl)
		Monitor::init(config.autoApplyGammaCurveDelay);
}
