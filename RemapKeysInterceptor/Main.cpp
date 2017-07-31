#include <stdio.h>
#include "Main.h"
#include "PluginContext.h"
#include "utils.h"

#define MAC_KEYBOARD L"HID\\VID_05AC&PID_0254&REV_0219&MI_00&Col01"

struct RemappingEntry {
	unsigned source, dest;
};

typedef const RemappingEntry* RemappingTable;

static bool processRemappingTable(RemappingPluginContext &context, RemappingTable table, int tableCount) {
	unsigned scanCode = context.strokeId;
	if (!scanCode) return false;
	for (int i = 0; i < tableCount; i += 1) {
		if (scanCode == table[i].source) {
			return context.replaceKey(table[i].dest);
		}
	}
	return false;
}

/////////////////////////////////////// Remapping plugins ///////////////////////////////////////
bool macRemappingPlugin(RemappingPluginContext &context) {
	static const RemappingEntry basicRemappings[] = {
		{ SC_LALT, SC_LCONTROL },
		{ SC_LCONTROL, SC_CAPITAL },
		{ SC_LWIN, SC_LALT },
		{ SC_INTERNATIONAL1, SC_LWIN }, // JP-eigo to LAlt
		{ SC_JPYEN, SC_TILDE },
		{ SC_JPUNDERSCORE, SC_TILDE },
		//{ SC_CAPITAL, SC_LCONTROL },
		//{ SC_RWIN, SC_RALT },
		//{ SC_INTERNATIONAL3, SC_RWIN }, // JP-eigo to LAlt
		//{ 0xC1, 0xC0 }, // JP-[_] to `
	};
	if (context.getHardwareId() == MAC_KEYBOARD) {
		return processRemappingTable(context, basicRemappings, numberof(basicRemappings));
	}
	return false;
}

bool virtualFnRemappingPlugin(RemappingPluginContext &context) {
	static const RemappingEntry fnRemappings[] = {
		{ SC_LEFT, SC_HOME },
		{ SC_RIGHT, SC_END },
		{ SC_UP, SC_PGUP },
		{ SC_DOWN, SC_PGDN },
		{ SC_BACKSPACE, SC_DELETE },
	};
	static const unsigned virtualFnKey = SC_CAPITAL;
	static bool virtualFnIsDown = false;

	if (context.getHardwareId() == MAC_KEYBOARD) {
		if (context.strokeId == virtualFnKey) {
			virtualFnIsDown = context.isPressingKey();
			return context.eatKey();
		}

		if (virtualFnIsDown) {
			return processRemappingTable(context, fnRemappings, numberof(fnRemappings));
		}
	}
	return false;
}

bool roxxorRemappingPlugin(RemappingPluginContext &context) {
	return false;
}

static const RemappingPlugin plugins[] = {
	{ virtualFnRemappingPlugin, false },
	{ macRemappingPlugin, true },
	{ roxxorRemappingPlugin, false },
};


int main() {
	RemappingPluginContext context;
	raise_process_priority();
	context.runMainLoop(plugins, numberof(plugins));
	return 0;
}
