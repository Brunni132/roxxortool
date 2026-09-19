#pragma once

namespace StatusWindow {
	extern void showPgControlsBlocked(bool blocked);
	extern void showBrightness(int brightnessLevel);
	extern void showVolume(bool logarithmic, double volumeLevel);
	extern void ShowBlocked();

	extern void showMessage(const char *message, int timeToLeaveOpen);
	extern void hideMessage();
}
