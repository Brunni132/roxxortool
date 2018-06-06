#include "Precompiled.h"
#include "Utilities.h"

static std::list<std::function<void()>> queue;
static int eventId = 0x2000;
static bool lockOnNextAction = false;

void CALLBACK PerformOnTimer(HWND, UINT, UINT_PTR idEvent, DWORD) {
	KillTimer(NULL, idEvent);
	if (!queue.empty()) {
		queue.front()();
		queue.pop_front();
	}
}

void RunAfterDelay(std::function<void()> code, int delayMs) {
	queue.push_back(code);
	SetTimer(NULL, eventId++, delayMs, PerformOnTimer);
}

void LockMachineOnNextAction() {
	lockOnNextAction = true;
}

void DidPerformAnAction() {
	if (lockOnNextAction) {
		LockWorkStation();
	}
}
