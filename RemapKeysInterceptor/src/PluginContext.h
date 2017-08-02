#pragma once

#include <interception.h>
#include <string>

#define numberof(o) (sizeof(o) / sizeof(*(o)))

enum ScanCode {
	SC_E0 = 0x100,
	SC_E1 = 0x200,
	SC_MAX_KEY_COUNT = 0x400,
	SC_STANDARD_LAST_KEY = 0xFF,

	SC_CAPITAL = 0x3A,
	SC_LSHIFT = 0x2A,
	SC_RSHIFT = 0x36,
	SC_LCONTROL = 0x1D,
	SC_RCONTROL = SC_E0 | 0x1D,
	SC_LALT = 0x38, // LALT has state 0 (down) / 1 (up) and RALT has state 2 (down) / 3 (up)
	SC_INTERNATIONAL1 = 0x7B,
	SC_INTERNATIONAL3 = 0x70,
	SC_JPYEN = 0x7D,
	SC_JPUNDERSCORE = 0x73,
	SC_TILDE = 0x29,
	SC_RALT = SC_E0 | 0x38,
	SC_LWIN = SC_E0 | 0x5B,
	SC_RWIN = SC_E0 | 0x5C,
	SC_LEFT = SC_E0 | 0x4B,
	SC_RIGHT = SC_E0 | 0x4D,
	SC_UP = SC_E0 | 0x48,
	SC_DOWN = SC_E0 | 0x50,
	SC_BACKSPACE = 0x0E,
	SC_HOME = SC_E0 | 0x47,
	SC_END = SC_E0 | 0x4F,
	SC_PGUP = SC_E0 | 0x49,
	SC_PGDN = SC_E0 | 0x51,
	SC_DELETE = SC_E0 | 0x53,
	SC_CONTEXTMENU = SC_E0 | 0x5D,

	SC_Q = 0x10,
	SC_W = 0x11,
	SC_H = 0x23,
	SC_0 = 0x01,
	SC_9 = 0x0A,
	SC_NUMPAD0 = 0x52,
	SC_NUMPAD1 = 0x4F,
	SC_NUMPAD2 = 0x50,
	SC_NUMPAD3 = 0x51,
	SC_NUMPAD4 = 0x4B,
	SC_NUMPAD5 = 0x4C,
	SC_NUMPAD6 = 0x4D,
	SC_NUMPAD7 = 0x47,
	SC_NUMPAD8 = 0x48,
	SC_NUMPAD9 = 0x49,
	SC_F4 = 0x3E,
};

struct RemappingPlugin {
	// Should return true if it has processed the key, false otherwise
	bool(*handler)(struct RemappingPluginContext &context);
	bool onlyIfNotProcessedYet;
};

struct RemappingPluginContext {
	unsigned strokeId;

	RemappingPluginContext();
	bool eatKey() {
		return eaten = true;
	}
	std::wstring getHardwareId();
	bool isPressingKey() const {
		return !(scannedKey.state & 1);
	}
	bool isDepressingKey() const {
		return (scannedKey.state & 1) != 0;
	}
	InterceptionKeyStroke& queueNewKey();
	bool replaceKey(unsigned newStrokeId);
	void runMainLoop(const RemappingPlugin *plugins, unsigned pluginCount);

private:
	InterceptionContext context;
	InterceptionDevice device;
	// Read only one, can write up to 31 other entries to the output (1..31, 0 being reserved for the read stroke)
	InterceptionKeyStroke strokeBuffer[32];
	// To optimize multiple queries via getHardwareId()
	std::wstring cachedHardwareId;
	// If the key is eaten, strokeBuffer[0] is not sent to output
	bool eaten;
	// Number of strokes, always >= 1 (read stroke only)
	unsigned strokeBufferCount;

	void prepareForNewStroke();

	// First key stroke (strokeBuffer[0]), the one scanned
	InterceptionKeyStroke& scannedKey;
	// Additional strokes (for buffering purposes)
	//InterceptionKeyStroke& keyStrokes(unsigned no) {
	//	if (no >= numberof(strokeBuffer)) throw "Illegal access";
	//	return *(InterceptionKeyStroke*)(&strokeBuffer[no]);
	//}
};

extern unsigned translateToStrokeId(InterceptionKeyStroke& stroke);
extern void translateIdToStroke(unsigned strokeId, InterceptionKeyStroke& destStroke);
