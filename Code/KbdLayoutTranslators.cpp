#include "Precompiled.h"
#include "KbdLayoutTranslators.h"
#include "Utilities.h"

static bool layoutTranslatorsInited = false;
LayoutTranslator layoutTranslatorsEnUs;

static void sendUnicodeKey(WORD vk, WORD scan, DWORD flags) {
	INPUT down;
	ZeroMemory(&down, sizeof(down));
	down.type = 1; //INPUT_KEYBOARD
	down.ki.wVk = vk;
	down.ki.wScan = scan;
	down.ki.time = 0;
	down.ki.dwFlags = flags;
	down.ki.dwExtraInfo = 0;
	SendInput(1, &down, sizeof(INPUT));
}

// Only to be called once, before you use layoutTranslator*
void layoutTranslatorsRegister() {
	if (layoutTranslatorsInited) return;
	layoutTranslatorsInited = true;

	// Simple E and A accented, and Ç
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('E', 0xE9, 0xC9, nullptr, 0));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('A', 0xE0, 0xC0, nullptr, 0));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('C', 0xE7, 0xC7, nullptr, 0));
	// ; -> …
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State(VK_OEM_1, 0x2026, 0x2026, nullptr, 0));

	//const WCHAR test[] = { L'ä', L'ë', L'ï', L'ö', L'ü', L'Ä' };
	const LayoutTranslator::State::StateOutcome outcomesForAcute[] = {
		{ 'A', 0xE1, 0xC1 },
		{ 'E', 0xE9, 0xC9 },
		{ 'I', 0xED, 0xCD },
		{ 'O', 0xF3, 0xD3 },
		{ 'U', 0xFA, 0xDA },
		{ ' ', 0xB4, 0xB4 },
	};
	const LayoutTranslator::State::StateOutcome outcomesForU[] = {
		{ 'A', 0xE0, 0xC0 },
		{ 'E', 0xE8, 0xC8 },
		{ 'I', 0xEC, 0xCC },
		{ 'O', 0xF2, 0xD2 },
		{ 'U', 0xF9, 0xD9 },
		{ ' ', 0x60, 0x60 },
	};
	const LayoutTranslator::State::StateOutcome outcomesForI[] = {
		{ 'A', 0xE2, 0xC2 },
		{ 'E', 0xEA, 0xCA },
		{ 'I', 0xEE, 0xCE },
		{ 'O', 0xF4, 0xD4 },
		{ 'U', 0xFB, 0xDB },
		{ ' ', 0x5E, 0x5E },
	};
	const LayoutTranslator::State::StateOutcome outcomesForO[] = {
		{ 'A', 0xE4, 0xC4 },
		{ 'E', 0xEB, 0xCB },
		{ 'I', 0xEF, 0xCF },
		{ 'O', 0xF6, 0xD6 },
		{ 'U', 0xFC, 0xDC },
		{ ' ', 0xA8, 0xA8 },
	};
	const LayoutTranslator::State::StateOutcome outcomesForN[] = {
		{ L'N', 0xF1, 0xD1 },
		{ ' ', 0x7E, 0x7E },
	};
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('U', 0x60, 0x60, outcomesForU, numberof(outcomesForU)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('I', 0x5E, 0x5E, outcomesForI, numberof(outcomesForI)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('O', 0xA8, 0xA8, outcomesForO, numberof(outcomesForO)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('P', 0xB4, 0xB4, outcomesForAcute, numberof(outcomesForAcute)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('N', 0x7E, 0x7E, outcomesForN, numberof(outcomesForN)));
}

LayoutTranslator::State::State(WCHAR entryChar, WCHAR defaultOutcome, WCHAR defaultOutcomeWithShift, const StateOutcome *outcomes, size_t numOutcomes)
	: entryChar(entryChar), defaultOutcome(defaultOutcome), defaultOutcomeWithShift(defaultOutcomeWithShift)
{
	for (size_t i = 0; i < numOutcomes; i += 1) {
		this->outcomes.push_back(outcomes[i]);
	}
}

LayoutTranslator::LayoutTranslator() : stateIndex(-1), isRAltDown(false), isMenuDown(false), shouldSendMenuKey(false), didForcePressLCtrl(false) {}

void LayoutTranslator::cancelAnyState() {
	stateIndex = -1;
}

// Returns TRUE if the key must be eaten
bool LayoutTranslator::processKeyUp(int kbdVcode) {
	if (kbdVcode == VK_APPS && !TaskManager::isInTeamViewer) {
		bool pressMenuKey = isMenuDown && shouldSendMenuKey;
		isMenuDown = shouldSendMenuKey = false;
		if (pressMenuKey) {
			kbdpress(VK_APPS, 0);
		}
		return true;
	}
	if (kbdVcode == VK_RMENU) {
		isRAltDown = false;
		if (didForcePressLCtrl) {
			didForcePressLCtrl = false;
			kbdup(VK_LCONTROL, 0);
		}
		return false;
	}
	for (unsigned i = 0; i < keysToEatOnPressup.size(); i++) {
		if (keysToEatOnPressup[i] == kbdVcode) {
			keysToEatOnPressup.erase(keysToEatOnPressup.begin() + i);
			return true;
		}
	}
	return false;
}

// Returns TRUE if the key must be eaten
bool LayoutTranslator::processKeyDown(int kbdVcode, bool shiftPressed) {
	if (kbdVcode == VK_APPS && !TaskManager::isInTeamViewer) {
		isMenuDown = true;
		shouldSendMenuKey = true;
		return true;
	}
	shouldSendMenuKey = false;

	if (kbdVcode == VK_RMENU) {
		isRAltDown = true;
		if (!lCtrlPressed) {
			didForcePressLCtrl = true;
			kbddown(VK_LCONTROL, 0);
		}
		return false;
	}
	// Ignore non-char keystrokes
	if (kbdVcode >= 0x30 && kbdVcode <= 0x5A || kbdVcode >= 0x60 && kbdVcode <= 0x6F || kbdVcode >= 0xBA && kbdVcode <= 0xC0 || kbdVcode == ' ') {
		// No state currently
		if (stateIndex == -1) {
			// Need alt for any special key
			if (isRAltDown || isMenuDown) {
				for (int i = 0; i < states.size(); i++) {
					if (kbdVcode == states[i].entryChar) {
						enterState(i, shiftPressed);
						// This key is eaten
						keysToEatOnPressup.push_back(kbdVcode);
						return true;
					}
				}
			}
		}
		else {
			// Process current state
			// Assumes that there are outcomes (else we don't enter the state in the first place, cf. enterState)
			const State &state = states[stateIndex];
			int foundOutcomeId = -1; // -1 = not found
			for (int i = 0; i < state.outcomes.size(); i++) {
				if (kbdVcode == state.outcomes[i].pressedChar) {
					foundOutcomeId = i;
					break;
				}
			}

			// Not found any outcome -> output the default outcome and leave the state
			if (foundOutcomeId == -1) {
				stateIndex = -1;
				outputChar(shiftPressed ? state.defaultOutcomeWithShift : state.defaultOutcome);
				// Note that in this case we also want to output the original typed character after the outcome (e.g. `e)
				return false;
			}
			else {
				// OK, just output the new outcome and leave the state
				const State::StateOutcome &outcome = state.outcomes[foundOutcomeId];
				stateIndex = -1;
				outputChar(shiftPressed ? outcome.outputCharWithShift : outcome.outputCharNormal);
				keysToEatOnPressup.push_back(kbdVcode);
				return true;
			}
		}
	}
	// Eat out AltGr
	return kbdVcode == VK_RMENU;
}

// ---- Private ----

// May not effectively enter the state, in case the state has a direct outcome
void LayoutTranslator::eatRAlt() {
	//if (isRAltDown) {
	//	if (KbdHook::cancelRAlt()) {

	//	}
	//}
}

void LayoutTranslator::enterState(int stateIndex, bool shiftPressed) {
	if (states[stateIndex].outcomes.empty()) {
		const State &state = states[stateIndex];
		// Not enter the state, just output the char
		outputChar(shiftPressed ? state.defaultOutcomeWithShift : state.defaultOutcome);
	}
	else {
		// Enter the state, wait for further instructions (processKeyDown)
		this->stateIndex = stateIndex;
	}
}

void LayoutTranslator::outputChar(WCHAR character) {
	//printf("Outputing %x\n", character);
	sendUnicodeKey(0, character, KEYEVENTF_UNICODE);
	sendUnicodeKey(0, character, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP);
}
