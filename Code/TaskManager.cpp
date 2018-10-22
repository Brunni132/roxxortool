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

	DeferredAction(NamedTask taskName, function<void()> taskFn) : taskName(taskName) {
		taskFn = [=] {
			TaskManager::RunNamed(taskName, taskFn);
		};
	}
};

static HANDLE hThread = NULL;
static bool exitRequested = false;
static list<Action> immediateActionQueue;
static list<DeferredAction> deferredActionQueue;

static DWORD WINAPI ImmediateActionThread(LPVOID lpParam) {
	while (!exitRequested) {
		while (!immediateActionQueue.empty()) {
			Action action = immediateActionQueue.front();
			immediateActionQueue.pop_front();
			action.taskFn();
		}

		// TODO improve with semaphores
		Sleep(10);
	}

	return 0;
}

void TaskManager::init() {
	assert(!hThread);           // Already inited
	exitRequested = false;
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
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	hThread = NULL;
}

ULONGLONG TaskManager::GetSystemTimeInMs() {
	//SYSTEMTIME time;
	//GetSystemTime(&time);
	//return (time.wSecond * 1000) + time.wMilliseconds;
	return GetTickCount64();
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

void TaskManager::RunLater(std::function<void()> function, int delay) {
	ULONGLONG executionDate = GetSystemTimeInMs() + delay;
	actionQueue.push_back(Action(TASKID_UNNAMED, function, executionDate));
}

void TaskManager::RunNamed(NamedTask taskId, function<void()> function, int delay) {
	ULONGLONG executionDate = GetSystemTimeInMs() + delay;
	CancelTask(taskId);
	actionQueue.push_back(Action(taskId, function, executionDate));
}
