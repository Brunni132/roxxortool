#include "Precompiled.h"
#include "Config.h"
#include "AudioMixer.h"
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

int APIENTRY WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow) {
	MSG msg;

	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	killAlreadyExisting();
	if (!config.readFile()) {
		MessageBox(NULL, "Unable to read config.json", "Cannot start", MB_ICONEXCLAMATION);
		exit(EXIT_FAILURE);
	}

#ifdef _DEBUG
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
#endif

	AudioMixer::init();
	KbdHook::start();
	if (config.unixLikeMouseWheel)
		MouseHook::start();

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	AudioMixer::terminate();
	return (int) msg.wParam;
}

