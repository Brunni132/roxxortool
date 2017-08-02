#include "PluginContext.h"
#include <stdio.h>

RemappingPluginContext::RemappingPluginContext() : eaten(false), scannedKey(*(InterceptionKeyStroke*)strokeBuffer), strokeBufferCount(1) {
}

InterceptionKeyStroke& RemappingPluginContext::queueNewKey() {
	if (strokeBufferCount + 1 >= numberof(strokeBuffer)) throw "Full stroke buffer";
	InterceptionKeyStroke& newKey = (InterceptionKeyStroke&)strokeBuffer[strokeBufferCount++];
	newKey.code = 0;
	newKey.state = 0;
	newKey.information = 0;
	return newKey;
}

std::wstring RemappingPluginContext::getHardwareId() {
	if (!cachedHardwareId.length()) {
		wchar_t hardwareId[500];
		size_t length = interception_get_hardware_id(context, device, hardwareId, sizeof(hardwareId));
		if (length > 0 && length < sizeof(hardwareId)) {
			cachedHardwareId = hardwareId;
		}
		else {
			cachedHardwareId = L"(unavailable)";
		}
	}
	return cachedHardwareId;
}

void RemappingPluginContext::prepareForNewStroke() {
	strokeId = translateToStrokeId(scannedKey);
	eaten = false;
	strokeBufferCount = 1;
	cachedHardwareId = L"";
}

bool RemappingPluginContext::replaceKey(unsigned newStrokeId) {
	if (newStrokeId != strokeId) {
		//printf("Remapping %x to %x\n", strokeId, newStrokeId);
		strokeId = newStrokeId;
		translateIdToStroke(strokeId, scannedKey);
	}
	return true;
}

void RemappingPluginContext::runMainLoop(const RemappingPlugin *plugins, unsigned pluginCount) {
	context = interception_create_context();
	if (!context) {
		fprintf(stderr, "Interception not found on your system. Please install it and try again.\n");
		return;
	}
	interception_set_filter(context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP | INTERCEPTION_FILTER_KEY_E0 | INTERCEPTION_FILTER_KEY_E1);

	while (interception_receive(context, device = interception_wait(context), (InterceptionStroke*)strokeBuffer, 1) > 0) {
		bool processed = false;
		prepareForNewStroke();
		//printf("A: %x %x %x -> %x\n", scannedKey.code, scannedKey.state, scannedKey.information, this->strokeId);

		for (unsigned i = 0; i < pluginCount; i += 1) {
			if (!processed || !plugins[i].onlyIfNotProcessedYet) {
				processed |= plugins[i].handler(*this);
			}
			if (eaten) break;
		}

		if (!eaten) {
			//for (int i = 0; i < strokeBufferCount; i += 1) {
			//	InterceptionKeyStroke &k = (InterceptionKeyStroke&)strokeBuffer[i];
			//	printf("%d: %x %x %x \n", i, k.code, k.state, k.information);
			//}
			interception_send(context, device, (InterceptionStroke*)strokeBuffer, strokeBufferCount);
		}
		else if (strokeBufferCount > 1) {
			interception_send(context, device, (InterceptionStroke*)(strokeBuffer + 1), strokeBufferCount - 1);
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
