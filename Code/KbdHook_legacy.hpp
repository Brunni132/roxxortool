int legacy_handleMacKeyboard(bool isDown, bool isUp, DWORD nKey) {
	// Eat accidental left/right presses after home/end on Mac
	static int eatNextCorrespondingKey;
	static DWORD releaseTime;
	static const struct { int original; int modified; } keyboardEatTable[] = {
		VK_LEFT, VK_HOME,
		VK_RIGHT, VK_END,
		VK_UP, VK_PRIOR,
		VK_DOWN, VK_NEXT,
		VK_BACK, VK_DELETE
	};

	if (isDown && nKey == eatNextCorrespondingKey) {
		// Key to be eaten
		DWORD nowTime = GetTickCount();
		eatNextCorrespondingKey = 0;
		if (nowTime - releaseTime <= 10) {
			return 1;
		}
	}
	else if (isUp) {
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
	return 0;
}

int legacy_handleJapaneseKeyboards(bool isDown, bool isUp, KBDLLHOOKSTRUCT* kbd) {
	if (config.japaneseMacKeyboard) {
		if (kbd->vkCode == 0xEB) {
			// 英 -> Lwin
			if (isDown) kbddown(0x5B, 0x5B);
			if (isUp) kbdup(0x5B, 0x5B);
			return 1;
		}
		if (kbd->vkCode == 0x14) {
			// Caps -> ctrl
			if (isDown) kbddown(0xA2, 0x1D);
			if (isUp) kbdup(0xA2, 0x1D);
			return 1;
		}
		if (kbd->vkCode == 0x5B) {
			// Lwin -> Loption
			if (isDown) kbddown(0xA4, 0x38);
			if (isUp) kbdup(0xA4, 0x38);
			return 1;
		}
		if (kbd->vkCode == 0xA2 && kbd->scanCode == 0x1D) {
			// Lctrl -> Caps
			if (isDown) kbddown(0x14, 0x3A);
			if (isUp) kbdup(0x14, 0x3A);
			return 1;
		}
		if (kbd->vkCode == 0xFF) {
			// かな -> Ralt
			if (isDown) {
				kbddown(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			if (isUp) {
				kbdup(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			return 1;
		}
		if (kbd->vkCode == 0xA4) {
			// Loption -> Ctrl
			if (isDown) kbddown(0xA2, 0x1D);
			if (isUp) kbdup(0xA2, 0x1D);
			return 1;
		}
		// If a key has been remapped, we'll never go further (return 1)
	}

	if (config.japaneseWindowsKeyboard) {
		if (kbd->vkCode == 0xEB) {
			// 無変換 -> Lalt
			if (isDown) kbddown(0xA4, 0x38);
			if (isUp) kbdup(0xA4, 0x38);
			return 1;
		}
		if (kbd->vkCode == 0xFF && kbd->scanCode == 0x79) {
			// 変換 -> Ralt
			if (isDown) kbddown(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			if (isUp) kbdup(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			return 1;
		}
		if (kbd->vkCode == 0xFF && kbd->scanCode == 0x70) {
			// ｶﾀｶﾅ -> Rwin
			if (isDown) kbddown(0x5C, 0x5C);
			if (isUp) kbdup(0x5C, 0x5C);
			return 1;
		}
		// If a key has been remapped, we'll never go further (return 1)
	}

	if (config.japaneseMacBookPro) {
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
		if (kbd->vkCode == 0x14) {
			// Fn
			if (isDown) virtualFnIsDown = true;
			if (isUp) {
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
				if (kbd->vkCode == fnRemappings[i][0]) {
					foundIndex = i;
					break;
				}
			}
			if (foundIndex >= 0) {
				int destKey = fnRemappings[foundIndex][1];
				if (isDown) {
					virtualKeysActive[foundIndex] = true;
					kbddown(destKey, 0);
					return 1;
				}
				if (isUp) {
					virtualKeysActive[foundIndex] = false;
					kbdup(destKey, 0);
					return 1;
				}
			}
		}
		if (kbd->vkCode == 0xEB) {
			// 英 -> Lcommand
			if (isDown) kbddown(0x5B, 0x5B);
			if (isUp) kbdup(0x5B, 0x5B);
			return 1;
		}
		if (kbd->vkCode == 0x5B) {
			// Lcommand -> Lalt
			if (isDown) kbddown(0xA4, 0x38);
			if (isUp) kbdup(0xA4, 0x38);
			return 1;
		}
		if (kbd->vkCode == 0xFF) {
			// かな -> Ralt
			if (isDown) {
				kbddown(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			if (isUp) {
				kbdup(0xA5, 0x38, KEYEVENTF_EXTENDEDKEY);
			}
			return 1;
		}
		if (kbd->vkCode == 0xA4) {
			// Loption -> Ctrl
			if (isDown) kbddown(0xA2, 0x1D);
			if (isUp) kbdup(0xA2, 0x1D);
			return 1;
		}
		if (kbd->vkCode == 0xA2 && kbd->scanCode == 0x1D) {
			// Lctrl -> Caps
			if (isDown) kbddown(0x14, 0x3A);
			if (isUp) kbdup(0x14, 0x3A);
			return 1;
		}
		// If a key has been remapped, we'll never go further (return 1)
	}

	return 0;
}

static void legacy_showStatusInfo() {
	TaskManager::RunLater([] {
		std::vector<std::string> infos;

		if (lCtrlPressed) infos.push_back("lCtrl");
		if (rCtrlPressed) infos.push_back("rCtrl");
		if (lWinPressed) infos.push_back("lWin");
		if (rWinPressed) infos.push_back("rWin");
		if (lShiftPressed) infos.push_back("lShift");
		if (rShiftPressed) infos.push_back("rShift");
		if (lAltPressed) infos.push_back("lAlt");

		for (int i = 0x00; i <= 0xff; i++) {
			if (GetAsyncKeyState(i) & 0x8000) {
				char buf[256];
				sprintf_s(buf, "key=%02x(%c)", i, i);
				infos.push_back(buf);
			}
		}

		char destBuffer[1024];
		listToString<1024>(destBuffer, infos);
		MessageBoxA(NULL, destBuffer, "RoxxorTool debug info", MB_ICONINFORMATION);
	}, 500);
}

void legacy_selectInputMethod() {
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
}
