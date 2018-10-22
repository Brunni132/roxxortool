#include "Precompiled.h"
#include "TaskManager.h"

using namespace std;
using namespace TaskManager;

struct Action {
	NamedTask taskName;
	function<void()> taskFn;

	Action(NamedTask taskName, function<void()> taskFn) : taskFn(taskFn), taskName(taskName) {}
};

struct DeferredAction {
	NamedTask taskName;
	function<void()> taskFn;
	DWORD timerIdEvent;

	DeferredAction(NamedTask taskName, function<void()> taskFn) : taskName(taskName), taskFn(taskFn) {}
};

static HANDLE hThread = NULL;
static HANDLE hQueueReadyEvent = NULL;
static bool exitRequested = false;
static list<Action> immediateActionQueue;
static list<DeferredAction> deferredActionQueue;

static DWORD WINAPI ImmediateActionThread(LPVOID lpParam) {
	while (!exitRequested) {
		printf("Loop executed\n");
		while (!immediateActionQueue.empty()) {
			Action action = immediateActionQueue.front();
			immediateActionQueue.pop_front();
			action.taskFn();
		}

		WaitForSingleObjectEx(hQueueReadyEvent, 10000, true);
	}

	return 0;
}

void CALLBACK PerformDeferredActionOnTimer(HWND, UINT, UINT_PTR idEvent, DWORD) {
	// Timer can be called only once
	KillTimer(NULL, idEvent);

	printf("Timer executed\n");
	for (auto it = deferredActionQueue.begin(); it != deferredActionQueue.end(); ) {
		if ((*it).timerIdEvent == idEvent) {
			DeferredAction action = *it;
			it = deferredActionQueue.erase(it);
			// Do not run from the timer thread
			RunNamed(action.taskName, [=] {
				action.taskFn();
			});
		}
		else ++it;
	}
}

void TaskManager::init() {
	assert(!hThread);           // Already inited
	exitRequested = false;

	hQueueReadyEvent = CreateEvent(NULL, false, false, NULL);
	hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ImmediateActionThread,  // thread function name
		NULL,                   // argument to thread function 
		0,                      // use default creation flags 
		NULL);                  // returns the thread identifier 
}

void TaskManager::terminate() {
	exitRequested = true;
	SetEvent(hQueueReadyEvent);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	hThread = NULL;
}

void TaskManager::CancelTask(NamedTask taskId) {
	// TODO proper mutex
	for (auto it = immediateActionQueue.begin(); it != immediateActionQueue.end(); ) {
		auto &action = *it;
		if (action.taskName == taskId) it = immediateActionQueue.erase(it);
		else ++it;
	}

	for (auto it = immediateActionQueue.begin(); it != immediateActionQueue.end(); ) {
		auto &action = *it;
		if (action.taskName == taskId) it = immediateActionQueue.erase(it);
		else ++it;
	}
}

void TaskManager::Run(std::function<void()> function) {
	immediateActionQueue.push_back(Action(TASKID_UNNAMED, function));
	SetEvent(hQueueReadyEvent);
}

void TaskManager::RunNamed(NamedTask taskName, std::function<void()> function) {
	if (taskName != TASKID_UNNAMED) CancelTask(taskName);
	immediateActionQueue.push_back(Action(taskName, function));
	SetEvent(hQueueReadyEvent);
}

void TaskManager::RunLater(std::function<void()> function, int delayMs) {
	RunNamedLater(TASKID_UNNAMED, function, delayMs);
}

void TaskManager::RunNamedLater(NamedTask taskName, std::function<void()> function, int delay) {
	if (taskName != TASKID_UNNAMED) CancelTask(taskName);

	DeferredAction action(taskName, function);
	action.timerIdEvent = SetTimer(NULL, 0, delay, PerformDeferredActionOnTimer);
	deferredActionQueue.push_back(action);
}

