#include "Precompiled.h"
#include "Config.h"
#include "AudioMixer.h"
#include "Main.h"
#include "KbdHook.h"
#include "MouseHook.h"

char originalExeCommand[1024];

static void killAlreadyExisting() {
	PROCESSENTRY32 entry;
	DWORD processId;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	processId = GetCurrentProcessId();

	if (Process32First(snapshot, &entry) == TRUE)
		while (Process32Next(snapshot, &entry) == TRUE)
			if (_stricmp(entry.szExeFile, "RoxxorTool.exe") == 0 && entry.th32ProcessID != processId) {  
				DWORD nExitCode;
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
				GetExitCodeProcess(hProcess, &nExitCode);
				TerminateProcess(hProcess, nExitCode);
				CloseHandle(hProcess);
			}

	CloseHandle(snapshot);
}

void Main::editConfigAndRelaunch() {
	// Stop the hooks so we can type and use the mouse
	KbdHook::terminate();
	MouseHook::terminate();
	// Edit file
	system("echo You pressed Ctrl+Win+R & echo When you close the notepad, RoxxorTool will restart with the new parameters. & notepad config.json");
	TCHAR reexecute[1024];
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof si;
	GetModuleFileName(NULL, reexecute, 1024);
	CreateProcess(reexecute, NULL,
		NULL, NULL, FALSE, 0, NULL,
		NULL, &si, &pi);
}

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow) {
	MSG msg;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	killAlreadyExisting();
	config.readFile();

#ifdef _DEBUG
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
#endif

	AudioMixer::init();
	KbdHook::start();
	MouseHook::start();
	TaskManager::init();

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	AudioMixer::terminate();
	KbdHook::terminate();
	MouseHook::terminate();
	TaskManager::terminate();
	return (int) msg.wParam;
}

