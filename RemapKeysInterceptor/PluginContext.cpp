#include "PluginContext.h"
#include <stdio.h>

static unsigned translateToStrokeId(InterceptionKeyStroke& stroke);
static void translateIdToStroke(unsigned strokeId, InterceptionKeyStroke& destStroke);

RemappingPluginContext::RemappingPluginContext(InterceptionKeyStroke& destStroke) : wrappedStroke(destStroke), eaten(false) {
	strokeId = translateToStrokeId(destStroke);
}

bool RemappingPluginContext::replaceKey(unsigned newStrokeId) {
	if (newStrokeId != strokeId) {
		//printf("Remapping %x to %x\n", strokeId, newStrokeId);
		strokeId = newStrokeId;
		translateIdToStroke(strokeId, wrappedStroke);
	}
	return true;
}

void runMainLoop(const RemappingPlugin *plugins, unsigned pluginCount) {
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionKeyStroke stroke;

	context = interception_create_context();
	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP | INTERCEPTION_FILTER_KEY_E0 | INTERCEPTION_FILTER_KEY_E1);

	while (interception_receive(context, device = interception_wait(context), (InterceptionStroke *)&stroke, 1) > 0) {
		RemappingPluginContext pluginContext(stroke);
		bool processed = false;
		//printf("A: %x %x %x -> %x\n", stroke.code, stroke.state, stroke.information, pluginContext.strokeId);

		for (unsigned i = 0; i < pluginCount; i += 1) {
			if (!processed || !plugins[i].onlyIfNotProcessedYet) {
				processed |= plugins[i].handler(pluginContext);
			}
			if (pluginContext.eaten) {
				break;
			}
		}

		if (!pluginContext.eaten) {
			interception_send(context, device, (InterceptionStroke *)&stroke, 1);
		}
	}

	interception_destroy_context(context);
}

unsigned translateToStrokeId(InterceptionKeyStroke& stroke) {
	int value = stroke.code;
	if (stroke.state & INTERCEPTION_KEY_E0) value |= SC_E0;
	if (stroke.state & INTERCEPTION_KEY_E1) value |= SC_E1;
	return value;
}

void translateIdToStroke(unsigned strokeId, InterceptionKeyStroke& destStroke) {
	if (strokeId & SC_E0) destStroke.state |= INTERCEPTION_KEY_E0;
	else destStroke.state &= ~INTERCEPTION_KEY_E0;
	if (strokeId & SC_E1) destStroke.state |= INTERCEPTION_KEY_E1;
	else destStroke.state &= ~INTERCEPTION_KEY_E1;
	destStroke.code = strokeId & SC_STANDARD_LAST_KEY;
}
