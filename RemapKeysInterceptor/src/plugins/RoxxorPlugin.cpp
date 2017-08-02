#include <stdio.h>
#include "RoxxorPlugin.h"
#include <Windows.h>
#include <functional>

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


bool roxxorRemappingPlugin(RemappingPluginContext &context) {
	static bool lWinPressed = false;

	// L-win flag toggle (special command prefix)
	if (context.strokeId == SC_LWIN) {
		lWinPressed = context.isPressingKey();
	}

	if (context.isPressingKey()) {
		// TODO translate virtual key to scan code and use these (as constants)
		switch (context.strokeId) {
		case SC_Q:
			if (lWinPressed) handleWinQ(context);
			break;
		case SC_H:
			if (lWinPressed) handleWinH(context);
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
