#include "Precompiled.h"
#include "WindowsExplorer.h"
#include <psapi.h>
#include <userenv.h>

bool WindowsExplorer::isActive() {
	HWND hWnd;
	DWORD processId;
	HANDLE hProcess;
	char processName[MAX_PATH];

	// Get the active window process name
	hWnd = GetForegroundWindow();
	GetWindowThreadProcessId(hWnd, &processId);
	hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	GetProcessImageFileName(hProcess, processName, numberof(processName));
	CloseHandle(hProcess);

	return !strcmp(processName + strlen(processName) - 13, "\\explorer.exe");
}

void WindowsExplorer::toggleShowHideFolders() {
	HKEY hKey;
	DWORD intValue, dataType, bufferLength;
	LONG lResult;
	const char *key1 = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";

	lResult = RegOpenKeyEx(HKEY_CURRENT_USER, key1, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
	if (lResult == ERROR_SUCCESS) {
		bufferLength = sizeof(intValue);
		RegQueryValueEx(hKey, "Hidden", 0, &dataType, (LPBYTE)&intValue, &bufferLength);

		// Les fichiers cachés sont actuellement actifs
		if (intValue)
			intValue = 0;
		else
			intValue = 1;

		RegSetValueEx(hKey, "Hidden", 0, REG_DWORD, (LPBYTE)&intValue, sizeof(intValue));
		RegSetValueEx(hKey, "ShowSuperHidden", 0, REG_DWORD, (LPBYTE)&intValue, sizeof(intValue));
		intValue = 1 - intValue;
		RegSetValueEx(hKey, "SuperHidden", 0, REG_DWORD, (LPBYTE)&intValue, sizeof(intValue));
		RegCloseKey(hKey);
	}
//	Sleep(50);
}

void WindowsExplorer::showHomeFolderWindow() {
	TCHAR szHomeDirBuf[MAX_PATH] = { 0 };
	HANDLE hToken = 0;
	OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	DWORD BufSize = MAX_PATH;
	GetUserProfileDirectory(hToken, szHomeDirBuf, &BufSize);
	CloseHandle(hToken);
	ShellExecuteA(NULL, "open", szHomeDirBuf, "", NULL, SW_SHOW);
}

