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
	SC_LCONTROL = 0x1D,
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
	//SC_TEMP_HACK = 0x56,
};

struct RemappingPlugin {
	// Should return true if it has processed the key, false otherwise
	bool(*handler)(struct RemappingPluginContext &context);
	bool onlyIfNotProcessedYet;
};

struct RemappingPluginContext {
	unsigned strokeId;
	bool eaten;

	RemappingPluginContext();
	bool eatKey() {
		return eaten = true;
	}
	std::wstring getHardwareId();
	bool isPressingKey() const {
		return !(stroke.state & 1);
	}
	bool isDepressingKey() const {
		return (stroke.state & 1) != 0;
	}
	bool replaceKey(unsigned newStrokeId);
	void runMainLoop(const RemappingPlugin *plugins, unsigned pluginCount);

private:
	InterceptionContext context;
	InterceptionDevice device;
	InterceptionKeyStroke stroke;
	std::wstring cachedHardwareId;
	void prepareForNewStroke();
};
