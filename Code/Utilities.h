#pragma once

// Codes: https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html or https://www.codeproject.com/Articles/7305/Keyboard-Events-Simulation-using-keybd-event-funct
void kbddown(int vkCode, BYTE scanCode, int flags = 0);
void kbdup(int vkCode, BYTE scanCode, int flags = 0);
void kbdpress(int vkCode, BYTE scanCode, int flags = 0);

void RunAfterDelay(std::function<void()> code, int delayMs = 0);
void LockMachineOnNextAction();
void DidPerformAnAction();
