#include "Precompiled.h"
#include "Config.h"
#include "Monitor.h"
#include "WMI.h"
#include "StatusWindow.h"

// [int(pow(i / 110.0, 2) * 100) for i in range(10, 111)]
static const int macBrightnessGamma[] = {
	0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 9, 9, 10, 10, 11, 11, 12, 13, 13, 14, 15, 16, 16,
	17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 38, 39, 40, 41, 42, 44, 45, 46,
	47, 48, 50, 51, 52, 54, 55, 56, 58, 59, 61, 62, 64, 65, 66, 68, 69, 71, 73, 74, 76, 77, 79, 81, 82, 84, 85, 87, 89, 91,
	92, 94, 96, 98, 100
};

/******** Preliminary stuff ********/
struct MonitorInfo {
	bool inited, useWmi;
	int currentBrightness, maxBrightness, minBrightness, lastBrightness;
	unsigned short gammaRamp[3 * 256];
	char deviceName[1024];
};

typedef struct _PHYSICAL_MONITOR {
	HANDLE hPhysicalMonitor;
	WCHAR  szPhysicalMonitorDescription[128];
} PHYSICAL_MONITOR, *LPPHYSICAL_MONITOR;

BOOL (WINAPI *GetMonitorCapabilities)(
	__in   HANDLE hMonitor,
	__out  LPDWORD pdwMonitorCapabilities,
	__out  LPDWORD pdwSupportedColorTemperatures
	) = NULL;

BOOL (WINAPI *GetPhysicalMonitorsFromHMONITOR)(
	__in   HMONITOR hMonitor,
	__in   DWORD dwPhysicalMonitorArraySize,
	__out  LPPHYSICAL_MONITOR pPhysicalMonitorArray
	) = NULL;

BOOL (WINAPI *GetNumberOfPhysicalMonitorsFromHMONITOR)(
	__in   HMONITOR hMonitor,
	__out  LPDWORD pdwNumberOfPhysicalMonitors
	) = NULL;


BOOL (WINAPI *DestroyPhysicalMonitors)(
	__in  DWORD dwPhysicalMonitorArraySize,
	__in  LPPHYSICAL_MONITOR pPhysicalMonitorArray
	) = NULL;


BOOL (WINAPI *GetMonitorBrightness)(
	__in   HANDLE hMonitor,
	__out  LPDWORD pdwMinimumBrightness,
	__out  LPDWORD pdwCurrentBrightness,
	__out  LPDWORD pdwMaximumBrightness
	) = NULL;


BOOL (WINAPI *SetMonitorBrightness)(
	__in  HANDLE hMonitor,
	__in  DWORD dwNewBrightness
	) = NULL;

BOOL (WINAPI *SetMonitorContrast)(
	__in  HANDLE hMonitor,
	__in  DWORD dwNewContrast
	) = NULL;

typedef enum _MC_GAIN_TYPE {
	MC_RED_GAIN     = 0,
	MC_GREEN_GAIN   = 1,
	MC_BLUE_GAIN    = 2 
} MC_GAIN_TYPE;

BOOL (WINAPI *SetMonitorRedGreenOrBlueGain)(
	__in  HANDLE hMonitor,
	__in  MC_GAIN_TYPE gtGainType,
	__in  DWORD dwNewGain
	) = NULL;

static void CALLBACK autoApplyTimer(HWND, UINT, UINT_PTR, DWORD);

static const int MAX_MONITORS_NUM = 128;
static MonitorInfo *monitors[MAX_MONITORS_NUM];
static int currentMonitors = 0;
static DWORD lastReadTime;
static bool hasBeenAttenuated = false;
static UINT_PTR timerProcPtr = NULL;

/******** Actual code ********/
// Create the screen control structure
void Monitor::init(int autoApplyGammaCurveDelay) {
	HMODULE lib = LoadLibrary("dxva2.dll");
	GetMonitorCapabilities = (BOOL (WINAPI*)(HANDLE,LPDWORD,LPDWORD))GetProcAddress(lib, "GetMonitorCapabilities");
	GetPhysicalMonitorsFromHMONITOR = (BOOL (WINAPI*)(HMONITOR,DWORD,LPPHYSICAL_MONITOR))GetProcAddress(lib, "GetPhysicalMonitorsFromHMONITOR");
	GetNumberOfPhysicalMonitorsFromHMONITOR = (BOOL (WINAPI*)(HMONITOR,LPDWORD))GetProcAddress(lib, "GetNumberOfPhysicalMonitorsFromHMONITOR");
	DestroyPhysicalMonitors = (BOOL (WINAPI*)(DWORD,LPPHYSICAL_MONITOR))GetProcAddress(lib, "DestroyPhysicalMonitors");
	GetMonitorBrightness = (BOOL (WINAPI*)(HANDLE,LPDWORD,LPDWORD,LPDWORD))GetProcAddress(lib, "GetMonitorBrightness");
	SetMonitorBrightness = (BOOL (WINAPI*)(HANDLE,DWORD))GetProcAddress(lib, "SetMonitorBrightness");
	SetMonitorContrast = (BOOL (WINAPI*)(HANDLE,DWORD))GetProcAddress(lib, "SetMonitorContrast");
	SetMonitorRedGreenOrBlueGain = (BOOL (WINAPI*)(HANDLE,MC_GAIN_TYPE,DWORD))GetProcAddress(lib, "SetMonitorRedGreenOrBlueGain");
	// Read once
	//if (config.brightnessCacheDuration != 0)
	//	increaseBrightnessBy(0);
	//autoApplyTimer(NULL, 0, 0, 0);
	if (autoApplyGammaCurveDelay)
		timerProcPtr = SetTimer(NULL, 0x1000, autoApplyGammaCurveDelay, autoApplyTimer);
}

static HMONITOR getCurrentMonitor() {
	return MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTOPRIMARY);
}

static PHYSICAL_MONITOR *getHandleToPhysicalMonitors(HMONITOR hMonitor, DWORD *out_size) {
	// Get the number of physical monitors.
	*out_size = 0;
	BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, out_size);
	PHYSICAL_MONITOR *physicalMonitors;
	if (!bSuccess || *out_size == 0)
		return NULL;

	// Allocate the array of PHYSICAL_MONITOR structures.
	physicalMonitors = (LPPHYSICAL_MONITOR)malloc(*out_size * sizeof(PHYSICAL_MONITOR));
	if (physicalMonitors) {
		// Get the array.
		bSuccess = GetPhysicalMonitorsFromHMONITOR(
			hMonitor, *out_size, physicalMonitors);
	}
	
	if (!bSuccess) {
		free(physicalMonitors);
		return NULL;
	}

	return physicalMonitors;
}

static void GetGammaRampForDevice(const char *deviceName, unsigned short ramp[256 * 3]) {
	//if (config.useCustomGammaCurve) {
	//	if (config.customGammaCurveGamma == 1.0f) {				// Speed optimization
	//		for (int i = 0; i < 256 * 3; i++)
	//			ramp[i] = config.customGammaCurveArray[i];
	//	}
	//	else {
	//		for (int i = 0; i < 256 * 3; i++)
	//			ramp[i] = (unsigned short) (pow((float) config.customGammaCurveArray[i] / 0xff00, config.customGammaCurveGamma) * 0xff00);
	//	}
	//}
	//else {
		HDC hDevice = CreateDC("DISPLAY", deviceName, NULL, NULL);
		GetDeviceGammaRamp(hDevice, ramp);
		DeleteDC(hDevice);
	//}
}

static void SetGammaRampForDevice(const char *deviceName, unsigned short initialRamp[3 * 256], int brightnessPercentage, bool forceApply) {
	int brightness = brightnessPercentage * 256 / 100;
	unsigned short values[3 * 256], *ptr = values;
	for (int i = 0; i < 256 * 3; i++) {
		unsigned short arrayVal = initialRamp[i] * brightness >> 8;
		*ptr++ = arrayVal;
	}
	HDC hDevice = CreateDC("DISPLAY", deviceName, NULL, NULL);
	if (forceApply) {
		values[0] += 256;
		SetDeviceGammaRamp(hDevice, values);
		values[0] -= 256;
	}
	SetDeviceGammaRamp(hDevice, values);
	DeleteDC(hDevice);
}

/**
 * Read the current information from the current monitor (the one on which the
 * mouse is).
 * @param mi [OUT] destination for display information, inited is set to true
 */
static void sc_get(MonitorInfo *mi, HANDLE hPhysicalMonitor, MONITORINFO *monitorInfo) {
	mi->useWmi = !GetMonitorBrightness(hPhysicalMonitor, (LPDWORD)&mi->minBrightness, (LPDWORD)&mi->currentBrightness, (LPDWORD)&mi->maxBrightness);
	GetGammaRampForDevice(mi->deviceName, mi->gammaRamp);
	if (mi->useWmi) {
		int maxBright = 100, previousBrightness = mi->currentBrightness;
		bool success = Wmi::GetBrightnessInfo(&mi->currentBrightness, &maxBright);
		if (success) {
			// FIXME: should read the level list and not assume 100 (the Levels can be 15 for instance).
			maxBright = 101;
			if (config.wmiLogarithmicBrightness) {
				// Translate to non linear using the table
				for (int i = 0; i < numberof(macBrightnessGamma); i++)
					if (macBrightnessGamma[i] >= mi->currentBrightness) {
						if (previousBrightness >= 0 && previousBrightness <= 100 &&
							macBrightnessGamma[previousBrightness] != macBrightnessGamma[i])
							mi->currentBrightness = i;
						else
							mi->currentBrightness = previousBrightness;
						break;
					}
			}
			mi->minBrightness = 0, mi->maxBrightness = maxBright - 1;
		} else {
			mi->maxBrightness = mi->minBrightness = mi->currentBrightness = 0;
		}
	}
	mi->inited = true;
	mi->lastBrightness = mi->currentBrightness;
}

/**
 * Write the current status of the structure to a monitor. Shows the current
 * brightness on that screen.
 * @param mi [IN] information to set. Only currentBrightness is used
 * @param hPhysicalMonitor handle to the physical monitor on which to apply the modification
 * @param monitorInfo info about the physical monitor on which to apply the modification
 */
static void sc_set(MonitorInfo *mi, HANDLE hPhysicalMonitor, MONITORINFO *monitorInfo) {
	if (!mi->inited)
		sc_get(mi, hPhysicalMonitor, monitorInfo);
	if (mi->inited) {
		// Use brightness level
		int monitorBrightness = max(min(mi->currentBrightness, mi->maxBrightness), mi->minBrightness);
		if (mi->useWmi) {
			if (config.wmiLogarithmicBrightness) {
				monitorBrightness = macBrightnessGamma[monitorBrightness];
			}
			Wmi::SetBrightness(monitorBrightness);
		} else
			SetMonitorBrightness(hPhysicalMonitor, monitorBrightness);
		// Needs gamma ramp too?
		bool needsGammaRamp = mi->lastBrightness < mi->minBrightness || mi->currentBrightness < 0;
		if (needsGammaRamp || config.forceReapplyGammaOnBrightnessChange) {
			int attenuation = mi->minBrightness - mi->currentBrightness;
			SetGammaRampForDevice(mi->deviceName, mi->gammaRamp, min(100, 100 - attenuation), !needsGammaRamp && config.forceReapplyGammaOnBrightnessChange);
			hasBeenAttenuated = attenuation > 0;
		}
		StatusWindow::showBrightness(mi->currentBrightness);
		mi->lastBrightness = mi->currentBrightness;
	}
}

static MonitorInfo *getMonitorInfoForName(const char *szDeviceName) {
	for (int i = currentMonitors - 1; i >= 0; i--)
		if (!strcmp(monitors[i]->deviceName, szDeviceName))
			return monitors[i];

	// Avoid overflows
	if (currentMonitors >= MAX_MONITORS)
		currentMonitors = 0;
	// Not found
	monitors[currentMonitors] = new MonitorInfo();
	strcpy_s(monitors[currentMonitors]->deviceName, sizeof(monitors[currentMonitors]->deviceName), szDeviceName);
	return monitors[currentMonitors++];
}

static bool shouldReadBrightnessNow(MonitorInfo *mi) {
	if (!mi->inited) return true;
	if (mi->currentBrightness < mi->minBrightness) return false;
	if (config.brightnessCacheDuration == 0) return true;
	// Reread after selected duration
	DWORD nowTime = GetTickCount(), last = lastReadTime;
	bool shouldRead = nowTime - lastReadTime >= DWORD(config.brightnessCacheDuration);
	lastReadTime = nowTime;
	return shouldRead;
}

void Monitor::exit() {
	if (timerProcPtr) {
		KillTimer(NULL, timerProcPtr);
		timerProcPtr = NULL;
	}
	// Restore gamma if it was modified
	if (hasBeenAttenuated) {
		for (int i = 0; i < currentMonitors; i++) {
			MonitorInfo *mi = monitors[i];
			SetGammaRampForDevice(mi->deviceName, mi->gammaRamp, 100, false);
		}
	}
}

void Monitor::increaseBrightnessBy(int by) {
	DWORD numMonitors;
	HMONITOR hMonitor = getCurrentMonitor();
	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);
	PHYSICAL_MONITOR *monitors = getHandleToPhysicalMonitors(hMonitor, &numMonitors);
	MonitorInfo *mi = getMonitorInfoForName(monitorInfo.szDevice);
	if (shouldReadBrightnessNow(mi)) {
		sc_get(mi, monitors->hPhysicalMonitor, &monitorInfo);
		// Round up
		if (by != 0) {
			mi->currentBrightness = (int)(mi->currentBrightness / by) * by;
		}
	}
	if (mi->inited) {
		mi->currentBrightness += by;
		if (mi->currentBrightness > mi->maxBrightness)
			mi->currentBrightness = mi->maxBrightness;
		if (mi->lastBrightness != mi->currentBrightness)
			sc_set(mi, monitors->hPhysicalMonitor, &monitorInfo);
	}
	DestroyPhysicalMonitors(numMonitors, monitors);
}

void Monitor::decreaseBrightnessBy(int by) {
	DWORD numMonitors;
	HMONITOR hMonitor = getCurrentMonitor();
	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(hMonitor, &monitorInfo);
	PHYSICAL_MONITOR *monitors = getHandleToPhysicalMonitors(hMonitor, &numMonitors);
	MonitorInfo *mi = getMonitorInfoForName(monitorInfo.szDevice);
	if (shouldReadBrightnessNow(mi))
		sc_get(mi, monitors->hPhysicalMonitor, &monitorInfo);
	if (mi->inited) {
		int allowedNegativeBy = config.allowNegativeBrightness ? 40 : 0;
		mi->currentBrightness -= by;
		if (mi->currentBrightness < mi->minBrightness - allowedNegativeBy)
			mi->currentBrightness = mi->minBrightness - allowedNegativeBy;
		if (mi->lastBrightness != mi->currentBrightness)
			sc_set(mi, monitors->hPhysicalMonitor, &monitorInfo);
	}
	DestroyPhysicalMonitors(numMonitors, monitors);
}

void CALLBACK autoApplyTimer(HWND, UINT, UINT_PTR, DWORD) {
	for (int i = 0; i < currentMonitors; i++) {
		MonitorInfo *mi = monitors[i];
		int attenuation = mi->minBrightness - mi->currentBrightness;
		if (attenuation > 0 || hasBeenAttenuated) {
			SetGammaRampForDevice(mi->deviceName, mi->gammaRamp, min(100, 100 - attenuation), false);
			hasBeenAttenuated = attenuation > 0;
		}
	}
}
