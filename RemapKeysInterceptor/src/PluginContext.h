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
	SC_F1 = 0x3B,
	SC_F2 = 0x3C,
	SC_F3 = 0x3D,
	SC_F4 = 0x3E,
	SC_F5 = 0x3f,
	SC_F6 = 0x40,
	SC_F7 = 0x41,
	SC_F8 = 0x42,
	SC_F9 = 0x43,
	SC_F10 = 0x44,
	SC_F11 = 0x57,
	SC_F12 = 0x58,

	// http://www.quadibloc.com/comp/scan.htm
	SC_VOLUMEMUTE = SC_E0 | 0x20,
	SC_VOLUMEDOWN = SC_E0 | 0x2E,
	SC_VOLUMEUP = SC_E0 | 0x30,
	SC_PLAY = SC_E0 | 0x22,
	SC_STOP = SC_E0 | 0x24,
	SC_NEXTTRACK = SC_E0 | 0x19,
	SC_PREVTRACK = SC_E0 | 0x10,
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
		return !(strokes[0].state & 1);
	}
	bool isDepressingKey() const {
		return (strokes[0].state & 1) != 0;
	}
	InterceptionKeyStroke& queueNewKey();
	bool replaceKey(unsigned newStrokeId);
	void runMainLoop(const RemappingPlugin *plugins, unsigned pluginCount);

private:
	InterceptionContext context;
	InterceptionDevice device;
	// Read only one, can write up to 31 other entries to the output (1..31, 0 being reserved for the read stroke)
	InterceptionKeyStroke strokes[32];
	// To optimize multiple queries via getHardwareId()
	std::wstring cachedHardwareId;
	// If the key is eaten, strokeBuffer[0] is not sent to output
	bool eaten;
	// Number of strokes, always >= 1 (read stroke only)
	unsigned strokeCount;

	void prepareForNewStroke();
};

extern unsigned translateToStrokeId(InterceptionKeyStroke& stroke);
extern void translateIdToStroke(unsigned strokeId, InterceptionKeyStroke& destStroke);
