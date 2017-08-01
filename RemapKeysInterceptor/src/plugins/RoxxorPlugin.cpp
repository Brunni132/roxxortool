#include <stdio.h>
#include "RoxxorPlugin.h"
#include <Windows.h>

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

static void simulateKeyPress(RemappingPluginContext& context, unsigned strokeId) {
	simulateKeyDown(context, strokeId);
	simulateKeyUp(context, strokeId);
}

static bool handleWinQ(RemappingPluginContext& context) {
	return context.eatKey();
}

static bool handleWinH(RemappingPluginContext& context) {
	simulateKeyPress(context, SC_RCONTROL); // To avoid bringing the menu
	ShowWindow(GetForegroundWindow(), SW_MINIMIZE);
	return context.eatKey();
}


bool roxxorRemappingPlugin(RemappingPluginContext &context) {
	static bool lWinPressed = false;

	// TODO translate virtual key to scan code and use these (as constants)
	switch (context.strokeId) {
	case SC_LWIN:
		lWinPressed = context.isPressingKey();
		break;
	case SC_Q:
		if (lWinPressed && context.isPressingKey()) return handleWinQ(context);
	case SC_H:
		if (lWinPressed && context.isPressingKey()) return handleWinH(context);
	}
	

	

	//if (context.strokeId == 


	//return context.eatKey();

	return false;
}
