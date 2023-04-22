#pragma once

#include "Ref.h"

extern char originalExeCommand[1024];

struct Config: public RefClass {
	// Set to null when found
	std::vector<const char*> foundProps;
	std::vector<std::string> errors;
	// The right control key becomes the context menu key
	//bool rightCtrlContextMenu;
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
	// Set that to true on a Mac, it will use a logarithmic scale ofr the brightness setting.
	bool wmiLogarithmicBrightness;
	int brightnessIncrementQuantity;
	// If false, just doesn't touch the gamma curve (unless you have useCustomGammaCurve)
	bool allowNegativeBrightness;
	// in milliseconds; 0 = disable
	int autoApplyGammaCurveDelay;
	// Ctrl+Win+Home: stop, End: play/pause, PgUp: previous, PgDn: next, Up: vol+, Down: vol-
	bool useSoftMediaKeys;
	// Set this to true to reapply the gamma curve even when changing the brightness only (i.e. brightness >= 0)
	bool forceReapplyGammaOnBrightnessChange;
	bool reloadConfigWithCtrlWinR;
	// Mac specifics
	// Eat accidental left/right presses after home/end on Mac
	bool iAmAMac;
	bool multiDesktopLikeApplicationSwitcher;
	bool winFOpensYourFiles;
	bool winHHidesWindow;
	// Uses LWin+[0-9] instead of Ctrl+[num0-num9] to switch tasks
	bool noNumPad;
	// Also bring the context menu when pressing on Right Shift
	bool rightShiftContextMenu;
	// Keep the current brightness in cache for the following duration before rereading it (milliseconds)
	long brightnessCacheDuration;
	bool closeWindowWithWinQ;
	bool altGraveToStickyAltTab;
	bool winTSelectsLastTask;
	int winTTaskMoveBy;
	bool altTabWithMouseButtons;
	// Sends Ctrl+Caps after Win+Space
	bool selectHiraganaByDefault;
	bool japaneseMacBookPro;
	bool japaneseMacKeyboard;
	bool japaneseWindowsKeyboard;
	bool winSSuspendsSystem;
	bool doNotUseWinSpace;
	bool internationalUsKeyboardForFrench;
	// If set, doesn't use the current gamma curve as a base, but always resets it
	bool resetDefaultGammaCurve;
	float scrollAccelerationFactor;
	int scrollAccelerationIntertia;
	float scrollAccelerationMaxScrollFactor;
	bool scrollAccelerationSendMultipleMessages;
	// If nonzero, ignores all scroll events that are not equal to this. Typically 120.
	int scrollAccelerationBaseValue;
	bool capsPageControls;
	bool disableCapsLock;
	bool processAltTabWithMouseButtonsEvenFromRdp;
	bool mediaKeysWithCapsLockFnKeys, mediaKeysWithCapsLockSpaceArrow;

	// Reload the values from the config file. Will affect all members of this instance. Must be called at least once before use of the instance.
	void readFile();

private:
	// Does initialization (if obj = null), read (if serializer = null) or write for any instance variable
	void process(struct JsonNode *obj = NULL, struct JsonWriterNode *serializer = NULL);
	void parseNumberArray(unsigned short array[], unsigned maxLength, struct JsonValue &val);
	void writeSampleFile(const char *fname, bool showInExplorer);
};

// Common instance to normally query the config from.
extern Config config;

