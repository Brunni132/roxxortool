#include <stdio.h>
#include "RoxxorPlugin.h"
#include <Windows.h>
#include <functional>
#include <Shldisp.h>

// config_enableShowDesktop not working yet
static const bool config_enableWinH = false, config_enableWinQ = false, config_enableShowDesktop = true;
static std::function<void(RemappingPluginContext&)> onHRelease, onQRelease;


static void simulateKeyDown(RemappingPluginContext& context, unsigned strokeId) {
	InterceptionKeyStroke& stroke = context.queueNewKey();
	translateIdToStroke(strokeId, stroke);
	stroke.state |= INTERCEPTION_KEY_DOWN;
}

static void simulateKeyUp(RemappingPluginContext& context, unsigned strokeId) {
	InterceptionKeyStroke& stroke = context.queueNewKey();
	translateIdToStroke(strokeId, stroke);
	stroke.state |= INTERCEPTION_KEY_UP;
}

//static void simulateKeyPress(RemappingPluginContext& context, unsigned strokeId) {
//	simulateKeyDown(context, strokeId);
//	simulateKeyUp(context, strokeId);
//}

static void handleWinQ(RemappingPluginContext& context) {
	context.replaceKey(SC_LALT);
	simulateKeyDown(context, SC_F4);

	onQRelease = [] (RemappingPluginContext& context) {
		context.replaceKey(SC_LALT);
		simulateKeyUp(context, SC_F4);
		onQRelease = nullptr;
	};
}

static void handleWinH(RemappingPluginContext& context) {
	//ShowWindow(GetForegroundWindow(), SW_MINIMIZE);
	//context.replaceKey(SC_RCONTROL);

	//onHRelease = [] (RemappingPluginContext& context) {
	//	context.replaceKey(SC_RCONTROL);
	//	onHRelease = nullptr;
	//};
}

static void handleShowDesktop(RemappingPluginContext& context) {
	char className[128], title[128];
	HWND hWnd = GetForegroundWindow();
	GetClassNameA(hWnd, className, 128);
	GetWindowTextA(hWnd, title, 128);
	if (strcmp(className, "Windows.UI.Core.CoreWindow") /*|| strcmp(title, "Search")*/) {
		ShowWindow(GetForegroundWindow(), SW_MINIMIZE);
	}

	CoInitialize(NULL);
	// Create an instance of the shell class
	IShellDispatch4 *pShellDisp = NULL;
	HRESULT sc = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_SERVER, IID_IDispatch, (LPVOID *)&pShellDisp);
	// Show the desktop
	sc = pShellDisp->ToggleDesktop();
	pShellDisp->Release();
}


bool roxxorRemappingPlugin(RemappingPluginContext &context) {
	static bool lWinPressed = false, lCtrlPressed = false, lAltPressed = false;

	// L-win flag toggle (special command prefix)
	if (context.strokeId == SC_LWIN) {
		lWinPressed = context.isPressingKey();
	}
	if (context.strokeId == SC_LALT) {
		lAltPressed = context.isPressingKey();
	}
	if (context.strokeId == SC_LCONTROL) {
		lCtrlPressed = context.isPressingKey();
	}

	if (context.isPressingKey()) {
		// TODO translate virtual key to scan code and use these (as constants)
		switch (context.strokeId) {
		case SC_Q:
			if (lWinPressed && config_enableWinQ) handleWinQ(context);
			break;
		case SC_H:
			if (lWinPressed && config_enableWinH) handleWinH(context);
			break;
		case SC_DOWN:
			if (lWinPressed && lCtrlPressed && lAltPressed && config_enableShowDesktop) handleShowDesktop(context);
			break;
		}
	}
	else if (context.isDepressingKey()) {
		switch (context.strokeId) {
		case SC_Q:
			if (onQRelease) onQRelease(context);
			break;
		case SC_H:
			if (onHRelease) onHRelease(context);
			break;
		}
	}

	return false;
}
