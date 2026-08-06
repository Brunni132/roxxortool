#include "Precompiled.h"
#include "KbdLayoutTranslators.h"
#include "Utilities.h"

static const WCHAR CHAR_UNBREAKABLE_SPACE = 0xA0;
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
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('E', L'é', L'É', nullptr, 0));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('A', L'à', L'À', nullptr, 0));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('C', L'ç', L'Ç', nullptr, 0));
	// ; -> …
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State(VK_OEM_1, L'…', L'…', nullptr, 0));
	// AltGr+[ and AltGr+] -> « » (AltGr+space = unbreakable space)
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State(0xDB, L'«', L'«', nullptr, 0));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State(0xDD, L'»', L'»', nullptr, 0));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State(0x20, CHAR_UNBREAKABLE_SPACE, CHAR_UNBREAKABLE_SPACE, nullptr, 0));

	//const WCHAR test[] = { L'ä', L'ë', L'ï', L'ö', L'ü', L'Ä' };
	const LayoutTranslator::State::StateOutcome outcomesForAcute[] = {
		{ 'A', L'á', L'Á' },
		{ 'E', L'é', L'É' },
		{ 'I', L'í', L'Í' },
		{ 'O', L'ó', L'Ó' },
		{ 'U', L'ú', L'Ú' },
		{ ' ', L'´', L'´' },
	};
	const LayoutTranslator::State::StateOutcome outcomesForU[] = {
		{ 'A', L'à', L'À' },
		{ 'E', L'è', L'È' },
		{ 'I', L'ì', L'Ì' },
		{ 'O', L'ò', L'Ò' },
		{ 'U', L'ù', L'Ù' },
		{ ' ', L'`', L'`' },
	};
	const LayoutTranslator::State::StateOutcome outcomesForI[] = {
		{ 'A', L'â', L'Â' },
		{ 'E', L'ê', L'Ê' },
		{ 'I', L'î', L'Î' },
		{ 'O', L'ô', L'Ô' },
		{ 'U', L'û', L'Û' },
		{ ' ', L'^', L'^' },
	};
	const LayoutTranslator::State::StateOutcome outcomesForO[] = {
		{ 'A', L'ä', L'Ä' },
		{ 'E', L'ë', L'Ë' },
		{ 'I', L'ï', L'Ï' },
		{ 'O', L'ö', L'Ö' },
		{ 'U', L'ü', L'Ü' },
		{ ' ', L'¨', L'¨' },
	};
	const LayoutTranslator::State::StateOutcome outcomesForN[] = {
		{ 'N', L'ñ', L'Ñ' },
		{ ' ', L'~', L'~' },
	};
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('U', L'`', L'`', outcomesForU, numberof(outcomesForU)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('I', L'^', L'^', outcomesForI, numberof(outcomesForI)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('O', L'¨', L'¨', outcomesForO, numberof(outcomesForO)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('P', L'´', L'´', outcomesForAcute, numberof(outcomesForAcute)));
	layoutTranslatorsEnUs.states.push_back(LayoutTranslator::State('N', L'~', L'~', outcomesForN, numberof(outcomesForN)));
}

LayoutTranslator::State::State(WCHAR entryChar, WCHAR defaultOutcome, WCHAR defaultOutcomeWithShift, const StateOutcome* outcomes, size_t numOutcomes)
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
	if (kbdVcode >= 0x30 && kbdVcode <= 0x5A || kbdVcode >= 0x60 && kbdVcode <= 0x6F || kbdVcode >= 0xBA && kbdVcode <= 0xFE || kbdVcode == ' ') {
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
			const State& state = states[stateIndex];
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
#ifdef _DEBUG
				printf("Vcode=%x state=%d no outcome -> %s ? %x\n", kbdVcode, stateIndex, shiftPressed ? "true" : "false", shiftPressed ? state.defaultOutcomeWithShift : state.defaultOutcome);
#endif
				outputChar(shiftPressed ? state.defaultOutcomeWithShift : state.defaultOutcome);
				// Note that in this case we also want to output the original typed character after the outcome (e.g. `e)
				return false;
			}
			else {
				// OK, just output the new outcome and leave the state
				const State::StateOutcome& outcome = state.outcomes[foundOutcomeId];
				stateIndex = -1;
#ifdef _DEBUG
				printf("Vcode=%x state=%d has outcome ID=%d -> %s ? %x\n", kbdVcode, stateIndex, foundOutcomeId, shiftPressed ? "true" : "false", shiftPressed ? outcome.outputCharWithShift : outcome.outputCharNormal);
#endif
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
		const State& state = states[stateIndex];
		// Not enter the state, just output the char
		outputChar(shiftPressed ? state.defaultOutcomeWithShift : state.defaultOutcome);
	}
	else {
		// Enter the state, wait for further instructions (processKeyDown)
		this->stateIndex = stateIndex;
	}
}

void LayoutTranslator::outputChar(WCHAR character) {
	// Correct syntax in French
	if (character == L'»') outputChar(CHAR_UNBREAKABLE_SPACE);

#ifdef _DEBUG
	printf("Outputing %x\n", character);
#endif
	sendUnicodeKey(0, character, KEYEVENTF_UNICODE);
	sendUnicodeKey(0, character, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP);

	if (character == L'«') outputChar(CHAR_UNBREAKABLE_SPACE);
}
