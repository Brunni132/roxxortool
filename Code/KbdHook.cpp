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
//#include "DisableAnimationsForWinTab.h"
#include "PowrProf.h"

const char* WINDOWS_NOT_TO_HIDE[] = {
	"Progman", "Shell_TrayWnd", "Windows.UI.Core.CoreWindow"
};

bool lCtrlPressed = false, rCtrlPressed = false, lWinPressed = false, rWinPressed = false, lShiftPressed = false, rShiftPressed = false, lAltPressed = false, capsPressed = false;
inline bool ctrlPressed() { return lCtrlPressed || rCtrlPressed; }
inline bool winPressed() { return lWinPressed || rWinPressed; }
inline bool shiftPressed() { return lShiftPressed || rShiftPressed; }
inline bool altPressed() { return lAltPressed; }
inline bool anyModifierPressed() { return lShiftPressed || rShiftPressed || lWinPressed || rWinPressed || lCtrlPressed || rCtrlPressed; }
// Only those two, not the others
inline bool ctrlWinPressed() { return lCtrlPressed && lWinPressed && !rWinPressed && !rCtrlPressed && !shiftPressed() && !altPressed(); }
inline bool ctrlWinAndMaybeShiftPressed() { return lCtrlPressed && lWinPressed && !rWinPressed && !rCtrlPressed && !altPressed(); }
inline bool winOnlyPressed() { return lWinPressed && !rWinPressed && !ctrlPressed() && !shiftPressed() && !altPressed(); }
static void cancelAllKeys();

// Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html or https://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct
inline void kbddown(int vkCode, BYTE scanCode, int flags) {
	keybd_event(vkCode, scanCode, flags, 0);
}

inline void kbdup(int vkCode, BYTE scanCode, int flags) {
	keybd_event(vkCode, scanCode, flags | KEYEVENTF_KEYUP, 0);
}

inline void kbdpress(int vkCode, BYTE scanCode, int flags) {
	kbddown(vkCode, scanCode, flags);
	kbdup(vkCode, scanCode, flags);
}

template<size_t N>
UINT MySendInput(const INPUT(&inputs)[N]) {
	return SendInput(static_cast<UINT>(N), const_cast<INPUT*>(inputs), sizeof(INPUT));
}

enum Location { START, CURRENT, END };

bool shouldIgnoreWindow(HWND hWnd) {
	char className[128], title[128];
	GetClassNameA(hWnd, className, 128);
	GetWindowTextA(hWnd, title, 128);

	for (auto i = 0u; i < numberof(WINDOWS_NOT_TO_HIDE); i++) {
		if (!strcmp(className, WINDOWS_NOT_TO_HIDE[i])) {
			return true;
		}
	}

	return false;
}

void sendNextPageCommand() {
	bool needsAlt = !altPressed();
	if (needsAlt) kbddown(VK_LMENU, 0);
	kbdpress(VK_RIGHT, 0);
	if (needsAlt) kbdup(VK_LMENU, 0);
}

static void switchToHiragana() {
	//bool needsControl = !ctrlPressed();
	//if (needsControl) kbddown(VK_RCONTROL, 0);
	//kbdpress(VK_CAPITAL, 0);
	//kbdpress(VK_CAPITAL, 0);
	//if (needsControl) kbdup(VK_RCONTROL, 0);
	//kbdpress(0x16, 0, 0); // 0x16 = VK_IME_ON

	kbddown(0xA2, 0x1D); // https://sethclydesdale.github.io/genki-study-resources/help/writing/#microsoft-shortcuts:~:text=CTRL%2BCAPS%3A%20Change%20to%20Hiragana%20Input
	kbdpress(0xF2, 0x3A);
	kbdup(0xA2, 0x1D);
}

static void switchToHiraganaAsync() {
	TaskManager::RunNamedLater(TASKID_SWITCH_TO_HIRAGANA, [] {
		switchToHiragana();
	}, config.selectHiraganaDelay);
}

static void moveToTask(int taskNo, Location from) {
	bool needsWin = !winPressed(), needsShift = !shiftPressed();
	if (from == START) kbdpress(VK_HOME, 0);
	else if (from == END) kbdpress(VK_END, 0);
	else if (from == CURRENT) taskNo += sgn(taskNo);
	if (taskNo == 0 || taskNo == 1) return;

	TaskManager::RunNamedLater(TASKID_TASK_SWITCH, [=] {
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
	}, 70);
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

template<size_t Size>
static void listToString(char dest[Size], std::vector<std::string> strings) {
	strcpy_s(dest, Size, "[");
	for (int i = 0; i < strings.size(); i++) {
		if (i > 0) strcat_s(dest, Size, ", ");
		strcat_s(dest, Size, strings[i].c_str());
	}
	strcat_s(dest, Size, "]");
}

void preventStartMenu() { // use that when releasing the Win key
	if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0);
}

#include "KbdHook_legacy.hpp"

static HHOOK g_hHook;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	// Not something for us
	if (nCode != HC_ACTION || TaskManager::isInTeamViewer) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT*)lParam;
	DWORD nKey = kbd->vkCode;
	// Ignore injected input
	bool injected = (kbd->flags & (LLKHF_INJECTED | LLKHF_LOWER_IL_INJECTED));
	bool isDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
	bool isUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;

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

	// Keep state for special keys
	if (isDown || isUp) {
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
			case VK_CAPITAL:
				capsPressed = isDown;
			//case VK_RMENU: // used by layoutTranslators*
			//	rAltPressed = isDown;
			//	break;
		}
	}

	// Win+L triggers a key down but not up, and no up for Win so get aware of that
	if (isDown && nKey == 'L' && winPressed()) {
		lWinPressed = rWinPressed = false;
	}

	// Keyboard translation services, must be run before everything else
	if (!injected && config.japaneseMacBookPro || config.japaneseWindowsKeyboard || config.japaneseMacKeyboard) {
		legacy_handleJapaneseKeyboards(isDown, isUp, kbd);
	}

	if (config.internationalUsKeyboardForFrench) {
		layoutTranslatorsRegister();
		if (isDown) {
			if (layoutTranslatorsEnUs.processKeyDown(nKey, shiftPressed())) return 1;
		}
		if (isUp) {
			if (layoutTranslatorsEnUs.processKeyUp(nKey)) return 1;
		}
	}

	// External monitor brightness change
	if (config.brightnessControl && ctrlWinAndMaybeShiftPressed()) {
		if (nKey == VK_F9) {
			if (isDown) {
				int qty = shiftPressed() ? 1 : config.brightnessIncrementQuantity;
				TaskManager::RunLaterOnSameThread([qty] { Monitor::decreaseBrightnessBy(qty); });
			}
			return 1;
		}
		else if (nKey == VK_F10) {
			if (isDown) {
				int qty = shiftPressed() ? 1 : config.brightnessIncrementQuantity;
				TaskManager::RunLaterOnSameThread([qty] { Monitor::increaseBrightnessBy(qty); });
			}
			return 1;
		}
	}

	if (config.useSoftMediaKeys && ctrlWinAndMaybeShiftPressed()) {
		if (nKey == VK_F5) {
			if (isDown) kbdpress(VK_MEDIA_STOP, 0);
			return 1;
		}
		else if (nKey == VK_F6) {
			if (isDown) kbdpress(VK_MEDIA_PREV_TRACK, 0);
			return 1;
		}
		else if (nKey == VK_F7) {
			if (isDown) kbdpress(VK_MEDIA_NEXT_TRACK, 0);
			return 1;
		}
		else if (nKey == VK_F8) {
			if (isDown) kbdpress(VK_MEDIA_PLAY_PAUSE, 0);
			return 1;
		}
		else if (nKey == VK_F11) {
			auto qty = shiftPressed() ? 1 : config.volumeIncrementQuantity;
			if (isDown) AudioMixer::decrementVolume(qty);
			return 1;
		}
		else if (nKey == VK_F12) {
			auto qty = shiftPressed() ? 1 : config.volumeIncrementQuantity;
			if (isDown) AudioMixer::incrementVolume(qty);
			return 1;
		}
	}

	// Insert -> start screen saver & lock
	if (config.startScreenSaverWithInsert && isDown && nKey == VK_INSERT) {
		//LockMachineOnNextAction();
		SendMessage(GetForegroundWindow(), WM_SYSCOMMAND, SC_SCREENSAVE, 0);
		return 1;
	}

	// Ctrl+H -> toggle hide/show hidden folders
	if (config.toggleHideFolders && isDown && ctrlPressed() && nKey == 'H') {
		if (WindowsExplorer::isActive()) {
			WindowsExplorer::toggleShowHideFolders();
			// Refresh the explorer
			kbdpress(VK_F5, 0, 0);
		}
	}

	// Logarithmic volume management
	if (config.smoothVolumeControl && isDown) {
		int processed = 0;
		if (nKey == 0xae) {
			// Molette -
			TaskManager::RunLaterOnSameThread([] { AudioMixer::decrementVolume(config.volumeIncrementQuantity); });
			processed = 0xae;
		}
		else if (nKey == 0xaf) {
			// Molette +
			TaskManager::RunLaterOnSameThread([] { AudioMixer::incrementVolume(config.volumeIncrementQuantity); });
			processed = 0xaf;
		}

		// Relâche la touche, il ne faut pas que Windows la prenne
		if (processed != 0) {
			kbdup(processed, 0);
			return 1;
		}
	}

	// Reexecute ourselves on Ctrl+Win+R
	if (config.reloadConfigWithCtrlWinR && isDown && ctrlWinPressed() && nKey == 'R') {
		Main::editConfigAndRelaunch();
		return 1;
	}

	//if (config.reloadConfigWithCtrlWinR && isDown && ctrlWinPressed() && nKey == 'D') {
	//	legacy_showStatusInfo();
	//	return 1;
	//}

	if (config.useSoftMediaKeys && isDown && ctrlWinPressed()) {
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
		}
	}

	if (config.winTSelectsLastTask) {
		if (winOnlyPressed() && isDown && nKey == 'T' && !injected) {
			//auto pressedKey = lWinPressed ? VK_LWIN : VK_RWIN;
			// Let the normal Win+T operate, and later, move the cursor
			TaskManager::RunLater([=] {
				kbdpress('T', 0);
				//kbdup(pressedKey, 0);
				kbdpress(VK_END, 0);
				//kbddown(pressedKey, 0);
			}, 10);
		}

		static bool injectReturnAfterWinB = false;
		if (winOnlyPressed() && wParam == WM_KEYDOWN && nKey == 'B') {
			injectReturnAfterWinB = true;
		}
		else if (!winPressed() && injectReturnAfterWinB) {
			injectReturnAfterWinB = false;
			TaskManager::RunLater([=] {
				kbdpress(VK_RETURN, 0);
			}, 10);
		}
	}

	if (config.winEOpensYourFiles && isDown && winOnlyPressed() && nKey == 'E') {
		preventStartMenu();
		WindowsExplorer::showHomeFolderWindow();
	}

	if (config.winFOpensYourFiles && isDown && winOnlyPressed() && nKey == 'F') {
		preventStartMenu();
		WindowsExplorer::showHomeFolderWindow();
	}

	if (config.winEOpensThisPC && isDown && winOnlyPressed() && nKey == 'E') {
		preventStartMenu();
		WindowsExplorer::showThisPcFolderWindow();
		return 1;
	}

	if (config.winHHidesWindow && isDown && winOnlyPressed() && nKey == 'H') {
		preventStartMenu();

		HWND hWnd = GetForegroundWindow();
		if (!shouldIgnoreWindow(hWnd)) {
			ShowWindow(hWnd, SW_MINIMIZE);
		}
		return 1;
	}

	if (config.closeWindowWithWinQ && isDown && winOnlyPressed() && nKey == 'Q') {
		preventStartMenu();

		HWND hWnd = GetForegroundWindow();
		if (!shouldIgnoreWindow(hWnd)) {
			SendMessage(GetForegroundWindow(), WM_SYSCOMMAND, SC_CLOSE, 0);
		}

		//if (!injected) {
		//	// Normal machine
		//	bool needAlt = !lAltPressed;
		//	if (needAlt) kbddown(VK_LMENU, 0);
		//	kbdup(VK_LWIN, 0); // Temporarily release WIN since Win+Alt+F4 does nothing
		//	kbdpress(VK_F4, 0); // +F4
		//	kbddown(VK_LWIN, 0); // Re-enable WIN
		//	if (needAlt) kbdup(VK_LMENU, 0);
		//}
		//else {
		//	// TeamViewer support
		//	if (!ctrlPressed()) kbdpress(VK_RCONTROL, 0); // To avoid bringing the menu
		//	SendMessage(GetForegroundWindow(), WM_CLOSE, 0, 0);
		//}
		return 1;
	}

	if (config.winSSuspendsSystem && isDown && winOnlyPressed() && nKey == 'S' && !injected) {
		SetSuspendState(false, false, false);
		return 1;
	}

	if (config.selectHiraganaByDefault) {
		static uint8_t needSwitchToHiragana = 0;

		if (isDown) {
			if (!config.doNotUseWinSpace && nKey == VK_SPACE && winPressed()) {
				needSwitchToHiragana = 1;
			}

			if (lShiftPressed && lAltPressed) {
				needSwitchToHiragana = 2;
			}
		}

		if (isUp) {
			if ((needSwitchToHiragana == 1 && !lWinPressed && !rWinPressed) ||
				(needSwitchToHiragana == 2 && !lAltPressed && !lShiftPressed)) {
				needSwitchToHiragana = 0;

				switchToHiraganaAsync();
			}
		}
	}

	if (config.doNotUseWinSpace && isDown && winOnlyPressed() && nKey == VK_SPACE) {
		// Replace by Alt+Shift
		sendAltShift();
		return 1;
	}

	if (config.iAmAMac) {
		if (legacy_handleMacKeyboard(isDown, isUp, nKey)) return 1;
	}

	if (config.capsPageControls) {
		static bool capsIsDown = false, capsHasHadEffect = false, preventNativeKeys = true;

		// Prevent native keys from working
		if (preventNativeKeys && !injected && (nKey == VK_PRIOR || nKey == VK_NEXT || nKey == VK_HOME || nKey == VK_END)) return 1;

		if (nKey == VK_INSERT) {
			if (wParam == WM_KEYDOWN) {
				StatusWindow::showPgControlsBlocked(preventNativeKeys = !preventNativeKeys);
			}
			return 1;
		}

		if (nKey == VK_CAPITAL && !injected) {
			if (capsIsDown && wParam == WM_KEYUP && !capsHasHadEffect && !config.disableCapsLock) {
				TaskManager::RunLaterOnSameThread([] {
					kbdpress(VK_CAPITAL, 0);
				});
			}
			capsIsDown = wParam == WM_KEYDOWN;
			capsHasHadEffect = false;
			return 1;
		}

		if (capsIsDown) {
			auto method = wParam == WM_KEYDOWN ? kbddown : kbdup;
			auto param = KEYEVENTF_EXTENDEDKEY | (wParam == WM_KEYUP ? KEYEVENTF_KEYUP : 0);
			if (nKey == VK_UP) {
				method(VK_PRIOR, 0x49, param);
				capsHasHadEffect = true;
				return 1;
			}
			else if (nKey == VK_DOWN) {
				method(VK_NEXT, 0x51, param);
				capsHasHadEffect = true;
				return 1;
			}
			else if (nKey == VK_LEFT) {
				method(VK_HOME, 0x47, param);
				capsHasHadEffect = true;
				return 1;
			}
			else if (nKey == VK_RIGHT) {
				method(VK_END, 0x4f, param);
				capsHasHadEffect = true;
				return 1;
			}
		}
	}
	else if (config.disableCapsLock) {
		if (nKey == VK_CAPITAL) return 1;
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

	// Extra responsive version (shows win menu on key down)
	if (config.disableWinKey && !shiftPressed() && !altPressed() && !ctrlPressed()) {
		static bool eatNextWinKey = true;
		bool isWinKey = nKey == VK_LWIN || nKey == VK_RWIN;

		if (winPressed() && !isWinKey) {
			if (nKey == config.disableWinKey) {
				if (wParam == WM_KEYDOWN) {
					HWND hwnd = FindWindow("Shell_TrayWnd", NULL);
					SendMessage(hwnd, WM_SYSCOMMAND, SC_TASKLIST, 0);
				}
				return 1;
			}
			else {
				eatNextWinKey = false;
			}
		}
		else if (isWinKey && wParam == WM_KEYUP) {
			if (eatNextWinKey) {
				// Quickly tap Alt
				MySendInput({
					{.type = INPUT_KEYBOARD, .ki = {.wVk = VK_LCONTROL, .dwFlags = 0 } },
					{.type = INPUT_KEYBOARD, .ki = {.wVk = VK_LCONTROL, .dwFlags = KEYEVENTF_KEYUP } }
				});
			}
			eatNextWinKey = true;
			return 0;
		}
	}

	if (config.rightShiftContextMenu) {
		static bool pressedAnotherKeySince = false;
		// Remappe Alt droit => context menu
		if (nKey == VK_RSHIFT) {
			if (wParam == WM_KEYDOWN) {
				pressedAnotherKeySince = false;
			}
			else if (wParam == WM_KEYUP && !pressedAnotherKeySince) {
				// Au keyup, on presse un context menu (93)
				TaskManager::Run([] {
					bool needsLeftShift = config.rightShiftContextMenuOpensExtendedMenu && !lShiftPressed;
					if (needsLeftShift) kbddown(VK_LSHIFT, 0);
					kbdpress(VK_APPS, 0);
					if (needsLeftShift) kbdup(VK_LSHIFT, 0);
				});
			}
		}
		else if (wParam == WM_KEYDOWN && nKey != VK_LSHIFT) // Note: you can tap rshift while holding lshift, which shows the classic or extended context menu
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

	//if (config.disableWinTabAnimation) {
	//	if (nKey == VK_TAB && winPressed() && wParam == WM_KEYDOWN) {
	//		// Must be run in the main thread because it calls SetTimer!
	//		disableAnimationsForDurationOfWinTab();
	//	}
	//}

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

	// wParam will contain the virtual key code.  
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void cancelAllKeys() {
	kbdup(VK_LCONTROL, 0); lCtrlPressed = false;
	kbdup(VK_RCONTROL, 0); rCtrlPressed = false;
	kbdup(VK_LWIN, 0); lWinPressed = false;
	kbdup(VK_RWIN, 0); rWinPressed = false;
	//kbdup(VK_APPS, 0);
	kbdup(VK_LMENU, 0); lAltPressed = false;
	kbdup(VK_RMENU, 0); // rAltPressed = false;
	kbdup(VK_LSHIFT, 0); lShiftPressed = false;
	kbdup(VK_RSHIFT, 0); rShiftPressed = false;
	kbdup(VK_CAPITAL, 0); capsPressed = false;
}

void KbdHook::start() {
	cancelAllKeys(); // can be useful in case the hook messed up something
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	if (config.brightnessControl)
		Monitor::init(config.autoApplyGammaCurveDelay);
}

void KbdHook::terminate() {
	if (config.brightnessControl)
		Monitor::terminate();
	UnhookWindowsHookEx(g_hHook);
	cancelAllKeys();
}
