#pragma once

namespace Wmi {
	bool GetBrightnessInfo(int *brightness, int *levels);
	bool SetBrightness(int val);
}
