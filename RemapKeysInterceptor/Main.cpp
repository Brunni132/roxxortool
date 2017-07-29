#include <interception.h>
#include "Main.h"
#include "utils.h"
#include <stdio.h>

static unsigned translateToStrokeId(InterceptionKeyStroke& stroke) {
	int value = stroke.code;
	if (stroke.state & INTERCEPTION_FILTER_KEY_E0) value |= SC_E0;
	if (stroke.state & INTERCEPTION_FILTER_KEY_E1) value |= SC_E1;
	return value;
}

static void translateIdToStroke(unsigned strokeId, InterceptionKeyStroke& destStroke) {
	if (strokeId & SC_E0) destStroke.state |= INTERCEPTION_FILTER_KEY_E0;
	else destStroke.state &= ~INTERCEPTION_FILTER_KEY_E0;
	if (strokeId & SC_E1) destStroke.state |= INTERCEPTION_FILTER_KEY_E1;
	else destStroke.state &= ~INTERCEPTION_FILTER_KEY_E1;
	destStroke.code = strokeId & SC_STANDARD_LAST_KEY;
}

static bool processRemappingTable(unsigned &scanCode, RemappingTable table, int tableCount) {
	if (!scanCode) return false;
	for (int i = 0; i < tableCount; i += 1) {
		if (scanCode == table[i].source) {
			scanCode = table[i].dest;
			return true;
		}
	}
	return false;
}

/////////////////////////////////////// Remapping plugins ///////////////////////////////////////
bool basicRemappingPlugin(RemappingPluginContext& context) {
	static const RemappingEntry basicRemappings[] = {
		{ SC_LALT, SC_LCONTROL },
		{ SC_LCONTROL, SC_CAPITAL },
		{ SC_LWIN, SC_LALT },
		//{ VK_OEM_PA1, VK_LWIN }, // JP-eigo to LAlt
		{ SC_RWIN, SC_RALT },
		//{ 0xC1, 0xC0 }, // JP-[_] to `
		{ SC_CAPITAL, SC_LCONTROL },
	};
	return processRemappingTable(context.strokeId, basicRemappings, numberof(basicRemappings));
}


bool virtualFnRemappingPlugin(RemappingPluginContext& context) {
	static const RemappingEntry fnRemappings[] = {
		{ SC_LEFT, SC_HOME },
		{ SC_RIGHT, SC_END },
		{ SC_UP, SC_PGUP },
		{ SC_DOWN, SC_PGDN },
		{ SC_BACKSPACE, SC_DELETE },
	};
	static const unsigned virtualFnKey = SC_TEMP_HACK;
	static bool virtualFnIsDown = false;

	if (context.strokeId == virtualFnKey) {
		virtualFnIsDown = context.isPressingKey();
		return context.eatKey();
	}

	if (virtualFnIsDown) {
		return processRemappingTable(context.strokeId, fnRemappings, numberof(fnRemappings));
	}
}

static const RemappingPlugin plugins[] = {
	{ virtualFnRemappingPlugin, false },
	{ basicRemappingPlugin, true },
};


int main() {
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionKeyStroke stroke;

	raise_process_priority();
	context = interception_create_context();
	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

	while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0) {
		//printf("A: %x %x %x\n", stroke.code, stroke.state, stroke.information);
		//printf("%x\n", getStrokeId(stroke));
		unsigned scanCode = getStrokeId(stroke);
		if (scanCode == virtualFnKey) {
			virtualFnIsDown = !(stroke.state & 1);
			continue;
		}

		bool replaced = false;
		if (virtualFnIsDown) {
			replaced = processRemappings(scanCode, fnRemappings, numberof(fnRemappings));
		}
		replaced = replaced || processRemappings(scanCode, remappings, numberof(remappings));
		if (replaced) {
			if (scanCode == 0) continue;
			strokeIdToStroke(scanCode, stroke);
		}
		//printf("B: %x %x %x\n", stroke.code, stroke.state, stroke.information);
		interception_send(context, device, (InterceptionStroke *)&stroke, 1);
	}

	interception_destroy_context(context);

	return 0;
}
