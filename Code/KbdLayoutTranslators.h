#pragma once

struct LayoutTranslator {
	struct State {
		struct StateOutcome {
			WCHAR pressedChar;
			WCHAR outputCharNormal;
			WCHAR outputCharWithShift;
		};
		WCHAR entryChar;
		// Note that the outcomes may be empty, in which case the defaultOutcome is chosen directly (the state is not really entered, it exits with the default outcome)
		vector<StateOutcome> outcomes;
		WCHAR defaultOutcome;
		WCHAR defaultOutcomeWithShift;

		State(WCHAR entryChar, WCHAR defaultOutcome, WCHAR defaultOutcomeWithShift, const StateOutcome *outcomes, size_t numOutcomes);
	};

	vector<State> states;

	LayoutTranslator();
	void cancelAnyState();
	bool processKeyDown(int kbdVcode, bool shiftPressed);
	bool processKeyUp(int kbdVcode);

private:
	int stateIndex; // -1 = nones
	bool isRAltDown;
	vector<int> keysToEatOnPressup;

	// May not effectively enter the state, in case the state has a direct outcome
	void enterState(int stateIndex, bool shiftPressed);
	void outputChar(WCHAR character);
};

extern void layoutTranslatorsRegister();
extern LayoutTranslator layoutTranslatorsEnUs;
