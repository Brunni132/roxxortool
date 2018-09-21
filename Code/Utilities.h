#pragma once

// Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html or https://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct
void kbddown(int vkCode, BYTE scanCode, int flags = 0);
void kbdup(int vkCode, BYTE scanCode, int flags = 0);
void kbdpress(int vkCode, BYTE scanCode, int flags = 0);

extern bool lCtrlPressed, rCtrlPressed, lWinPressed, rWinPressed, lShiftPressed, rShiftPressed, lAltPressed;

enum NamedTask {
	TASKID_SWITCH_TO_HIRAGANA
};

void RunNamedTaskAfterDelay(NamedTask taskId, int delayMs, std::function<void()> code, std::function<void()> onFinish = nullptr);
void RunAfterDelay(std::function<void()> code, int delayMs = 0);
void CancelNamedTask(NamedTask taskId);

//void LockMachineOnNextAction();
//void DidPerformAnAction();
