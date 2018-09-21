#include "Precompiled.h"
#include "Utilities.h"

using namespace std;

// Simply stores a pair eventId, callback
struct TaskInfo {
	UINT_PTR timerIdEvent;
	function<void()> callback;
	// Always executed, even if the task is canceled
	function<void()> onFinish;

	TaskInfo() : timerIdEvent(0), callback(nullptr) {}
	bool isValid() const { return timerIdEvent != 0; }
	void set(UINT_PTR timerIdEvent, function<void()> callback, function<void()> onFinish) {
		assert(!this->timerIdEvent && !this->callback && !this->onFinish);
		this->timerIdEvent = timerIdEvent;
		this->callback = callback;
		this->onFinish = onFinish;
	}
	// Cancels the task but doesn't execute the onFinish callback
	void clear() {
		timerIdEvent = 0;
		callback = nullptr;
		onFinish = nullptr;
	}
};

static list<function<void()>> queue;
static int eventId = 0x2000;
static bool lockOnNextAction = false;
// Timer event ID => task ID
static map<UINT_PTR, NamedTask> eventIdsToTaskId;
// Task ID => (timer event ID, other)
static map<NamedTask, TaskInfo> taskIdsToInfo;

void CALLBACK PerformOnTimer(HWND, UINT, UINT_PTR idEvent, DWORD) {
	KillTimer(NULL, idEvent);
	if (!queue.empty()) {
		queue.front()();
		queue.pop_front();
	}
}

void RunAfterDelay(function<void()> code, int delayMs) {
	queue.push_back(code);
	SetTimer(NULL, eventId++, delayMs, PerformOnTimer);
}

void CALLBACK PerformNamedOnTimer(HWND, UINT, UINT_PTR idEvent, DWORD) {
	// Timer can be called only once
	KillTimer(NULL, idEvent);
	eventIdsToTaskId[idEvent] = (NamedTask)0;

	NamedTask taskId = eventIdsToTaskId[idEvent];
	TaskInfo &task = taskIdsToInfo[taskId];
	function<void()> mainCb = task.callback;
	function<void()> finishCb = task.onFinish;

	// Won't be triggered again
	task.clear();
	mainCb();
	if (finishCb) finishCb();
}

void RunNamedTaskAfterDelay(NamedTask taskId, int delayMs, function<void()> code, function<void()> onFinish) {
	assert(delayMs > 0);	// Race condition may occur otherwise
	CancelNamedTask(taskId);

	// The issue is that SetTimer always regenerates an ID and doesn't pass it back to the callback, so we need double linked lists
	UINT_PTR idEvent = SetTimer(NULL, 0, delayMs, PerformNamedOnTimer);
	eventIdsToTaskId[idEvent] = taskId;

	TaskInfo &task = taskIdsToInfo[taskId];
	task.set(idEvent, code, onFinish);
}

void CancelNamedTask(NamedTask taskId) {
	TaskInfo &task = taskIdsToInfo[taskId];
	if (task.isValid()) {
		function<void()> finishCb = task.onFinish;
		KillTimer(NULL, task.timerIdEvent);
		eventIdsToTaskId[task.timerIdEvent] = (NamedTask)0;

		task.clear();
		if (finishCb) finishCb();
	}
}

//void LockMachineOnNextAction() {
//	lockOnNextAction = true;
//}
//
//void DidPerformAnAction() {
//	if (lockOnNextAction) {
//		LockWorkStation();
//	}
//}
