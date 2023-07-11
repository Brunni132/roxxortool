#include "Precompiled.h"
#include "StatusWindow.h"

#define szWindowClass "Brunni.media.volume.6.0.0"
#define szWindowTitle ""
#define WS_EX_TOPWINDOW 0x00000008

static bool alreadyRegistered = false;
static HWND g_hWnd, g_hStaticText;
static char text[100];
static int windowWidth, windowHeight;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
		g_hStaticText = CreateWindow("static", text, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, 0, 0, windowWidth, windowHeight, hWnd, NULL, GetModuleHandle(NULL), 0);
		break;

	//case WM_WINDOWPOSCHANGED: {
	//	WINDOWPOS *pos = (WINDOWPOS*)lParam;
	//	printf("SetWindowPos %d %d\n", pos->cx, pos->cy);
	//	SetWindowPos(hWnd, g_hStaticText, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
	//	break;
	//}

	case WM_TIMER:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		g_hWnd = NULL;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL;
	wcex.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

// Needs the text variable to be set to the message text to show
// If the text is an empty string, the window is hidden if active / nothing is shown
static void showMessageInternal(int delayMs, int width = 200, int height = 50) {
	MONITORINFOEX monitorInfo;
	HMONITOR hMonitor = MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTOPRIMARY);
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);
	bool isEmptyMessage = !text[0];

	if (g_hWnd && (windowWidth != width || windowHeight != height)) {
		KillTimer(g_hWnd, 0);
		DestroyWindow(g_hWnd);
	}

	windowWidth = width, windowHeight = height;
	if (g_hWnd) {
		KillTimer(g_hWnd, 0);
		SetTimer(g_hWnd, 0, delayMs, NULL);
	} else if (!isEmptyMessage) {
		if (!alreadyRegistered)
			MyRegisterClass(GetModuleHandle(NULL));
		alreadyRegistered = true;
		g_hWnd = CreateWindowEx(WS_EX_TOPMOST, szWindowClass, szWindowTitle, WS_POPUP,
			0, 0, width, height, NULL, NULL, GetModuleHandle(NULL), 0);
		ShowWindow(g_hWnd, SW_SHOW);
		SetTimer(g_hWnd, 0, delayMs, NULL);
	}

	if (!isEmptyMessage) {
		SetWindowText(g_hStaticText, text);
		SetWindowPos(g_hWnd, HWND_TOPMOST,
			(monitorInfo.rcMonitor.right + monitorInfo.rcMonitor.left - width) / 2,
			(monitorInfo.rcMonitor.bottom + monitorInfo.rcMonitor.top - height) / 2,
			width, height, SWP_NOZORDER);
		SetWindowPos(g_hWnd, g_hStaticText, 0, 0, windowWidth, windowHeight, SWP_NOMOVE | SWP_NOZORDER);
	}
}

void StatusWindow::showBrightness(int brightnessLevel) {
	sprintf(text, "Brightness: %d", brightnessLevel);
	showMessageInternal(2000);
}

void StatusWindow::showVolume(double volumeLevel) {
	// Int vs double
	if (volumeLevel - floor(volumeLevel) < DBL_EPSILON)
		sprintf(text, "Volume: %d dB", (int)volumeLevel);
	else
		sprintf(text, "Volume: %.1f dB", volumeLevel);
	showMessageInternal(2000);
}

void StatusWindow::ShowBlocked() {
	MONITORINFOEX monitorInfo;
	HMONITOR hMonitor = MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTOPRIMARY);
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);
	strcpy_s(text, "CapsLock set; ignoring keystroke");
	showMessageInternal(2000, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
}

void StatusWindow::showMessage(const char *message, int timeToLeaveOpen) {
	strcpy_s(text, message);
	showMessageInternal(timeToLeaveOpen);
}

void StatusWindow::hideMessage() {
	showMessage("", 0);
}
