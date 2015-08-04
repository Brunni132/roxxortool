#pragma once

namespace Monitor {
	/**
	 * Initialize the system. Call this once at beginning.
	 * @param autoApplyGammaCurveDelay if non null, the gamma curve for all detected
	 * monitors will be reapplied at this interval in milliseconds
	 */
	void init(int autoApplyGammaCurveDelay);
	/**
	 * Increase the brightness of the display under the mouse, and apply it to the
	 * display.
	 * @param by by how much to increase it; the value is clamped to the max
	 */
	void increaseBrightnessBy(int by);
	/**
	 * Decrease the brightness of the display under the mouse, and apply it to the
	 * display.
	 * @param by by how much to decrease it; the value is clamped to the min
	 */
	void decreaseBrightnessBy(int by);
}
