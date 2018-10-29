#include "Precompiled.h"

using namespace std;
using namespace TaskManager;

struct Action {
	NamedTask taskName;
	function<void()> taskFn;
	uint64_t executionTime;

	Action(NamedTask taskName, function<void()> taskFn, uint64_t executionTime) : taskFn(taskFn), taskName(taskName), executionTime(executionTime) {}
};

static HANDLE hThread = NULL;
static HANDLE hQueueReadyEvent = NULL;
static bool exitRequested = false;
static list<Action> actionQueue;
static UINT_PTR checkRemoteDesktopTimer = 0;
bool TaskManager::isInTeamViewer = false, TaskManager::isBeingRemoteDesktopd = false;

static DWORD WINAPI ActionWorkerThread(LPVOID lpParam) {
	while (!exitRequested) {
		auto now = CurrentTime();
		// Not super fast: even if we add something in the middle the loop will terminate with the number of items that there was at the start
		// Then because the now is computed only once the newly added actions wouldn't be executed anyway (because planned in the future)
		// In any case it's not that bad because WaitForSingleObjectEx will do nothing since the event will already be set
		for (auto it = actionQueue.begin(); it != actionQueue.end(); ) {
			if (now >= (*it).executionTime) {
				Action action = *it;
				it = actionQueue.erase(it);
				action.taskFn();
			}
			else ++it;
		}

		WaitForSingleObjectEx(hQueueReadyEvent, actionQueue.empty() ? INFINITE : 10, true);
	}

	return 0;
}

void CALLBACK CheckRemoteDesktop(HWND, UINT, UINT_PTR, DWORD) {
	char className[128];
	HWND hWnd = GetForegroundWindow();
	GetClassName(hWnd, className, numberof(className));
	//isInMsRDP = !strcmp(className, "TscShellContainerClass");
	isInTeamViewer = !strcmp(className, "TV_CClientWindowClass");
	// TODO only works with MS TSC
	isBeingRemoteDesktopd = GetSystemMetrics(SM_REMOTESESSION);
}

void TaskManager::init() {
	assert(!hThread);           // Already inited
	exitRequested = false;

	hQueueReadyEvent = CreateEvent(NULL, false, false, NULL);
	hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ActionWorkerThread,     // thread function name
		NULL,                   // argument to thread function 
		0,                      // use default creation flags 
		NULL);                  // returns the thread identifier 

	checkRemoteDesktopTimer = SetTimer(NULL, 0, 2000, CheckRemoteDesktop);
}

void TaskManager::terminate() {
	KillTimer(NULL, checkRemoteDesktopTimer);
	exitRequested = true;
	SetEvent(hQueueReadyEvent);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	hThread = NULL;
}

uint64_t TaskManager::CurrentTime() {
	return GetTickCount64();
}

extern void Internal_CancelTask(NamedTask taskId);

void TaskManager::CancelNamed(NamedTask taskId) {
	// TODO proper mutex
	for (auto it = actionQueue.begin(); it != actionQueue.end(); ) {
		auto &action = *it;
		if (action.taskName == taskId) it = actionQueue.erase(it);
		else ++it;
	}
	Internal_CancelTask(taskId);
}

void TaskManager::Run(std::function<void()> function) {
	RunNamedLater(TASKID_UNNAMED, function, 0);
}

void TaskManager::RunNamed(NamedTask taskName, std::function<void()> function) {
	RunNamedLater(taskName, function, 0);
}

void TaskManager::RunLater(std::function<void()> function, int delayMs) {
	RunNamedLater(TASKID_UNNAMED, function, delayMs);
}

void TaskManager::RunNamedLater(NamedTask taskName, std::function<void()> function, int delay) {
	if (taskName != TASKID_UNNAMED) CancelNamed(taskName);

	actionQueue.push_back(Action(taskName, function, CurrentTime() + delay));
	SetEvent(hQueueReadyEvent);
}
