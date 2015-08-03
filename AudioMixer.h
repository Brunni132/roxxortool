#pragma once

namespace AudioMixer {
	// Volume type
	typedef float vol_t;

	// Initializes the mixer, necessary before use
	extern void init();
	extern void terminate();
	extern vol_t getVolume();
	extern void setVolume(vol_t newVolume);
	extern void incrementVolume(float increment);
	extern void decrementVolume(float increment);
}
