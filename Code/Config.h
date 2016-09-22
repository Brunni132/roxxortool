#pragma once

#include "Ref.h"

extern char originalExeCommand[1024];

struct Config: public RefClass {
	// The right control key becomes the context menu key
	bool rightCtrlContextMenu;
	// Also bring the context menu when pressing on AltGr
	bool altGrContextMenu;
	// Toggle hidden folder view in explorer by pressing Ctrl+H
	bool toggleHideFolders;
	// Start screensaver using Insert key
	bool startScreenSaverWithInsert;
	// Enables smoother, logarithmic volume control (necessary if the keyboard has volume control)
	bool smoothVolumeControl;
	// For a single press, uses this value; for a press with Ctrl, does it 4x
	float volumeIncrementQuantity;
	// Brightness control if a monitor supporting DDC/CI is connected using Ctrl+Win+Left/Right
	bool ddcCiBrightnessControl;
	int brightnessIncrementQuantity;
	// in milliseconds; 0 = disable
	int autoApplyGammaCurveDelay;
	// Ctrl+Win+Home: stop, End: play/pause, PgUp: previous, PgDn: next, Up: vol+, Down: vol-
	bool useSoftMediaKeys;
	// Set this to true to reapply the gamma curve even when changing the brightness only (i.e. brightness >= 0)
	bool forceReapplyGammaOnBrightnessChange;
	bool useCustomGammaCurve;
	unsigned short customGammaCurveArray[3 * 256];
	float customGammaCurveGamma;
	bool reloadConfigWithCtrlWinR;
	// Mac specifics - The right alt key becomes the context menu key
	// Allows to use the Mac keys Left Win+left/right/up/down to move the cursor
	// Allows to use left win+number as ctrl+win+number (right is still accessible)
	bool iAmAMac;
	bool multiDesktopLikeApplicationSwitcher;
	bool winFOpensYourFiles;
	bool winHHidesWindow;
	bool unixLikeMouseWheel;
	float horizontalScrollFactor;
	// Uses LWin+[0-9] instead of Ctrl+[num0-num9] to switch tasks
	bool noNumPad;
	// Also bring the context menu when pressing on Right Shift
	bool rightShiftContextMenu;

	// Reload the values from the config file. Will affect all members of this instance. Must be called at least once before use of the instance.
	bool readFile();

private:
	// Does initialization (if obj = null), read (if serializer = null) or write for any instance variable
	void process(struct JsonNode *obj = NULL, struct JsonWriterNode *serializer = NULL);
	void parseNumberArray(unsigned short array[], unsigned maxLength, struct JsonValue &val);
	void writeSampleFile(const char *fname);
};

// Common instance to normally query the config from.
extern Config config;

