#include "Precompiled.h"
#include "AudioMixer.h"
#include "StatusWindow.h"
using namespace AudioMixer;

static bool needsUpdateMixerEachTime = true;			// Card with two separate channels for headphones and speakers
static IAudioEndpointVolume *g_endpointVolume = NULL;
static IMMDeviceEnumerator *deviceEnumerator = NULL;
static bool g_logarithmic;
static float g_minVolume, g_maxVolume;

static void updateWithDefaultEndpoint();

void AudioMixer::init(bool logarithmic) {
	HRESULT hr;
	g_logarithmic = logarithmic;

	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	updateWithDefaultEndpoint();
}

void AudioMixer::terminate() {
	CoUninitialize();
}

vol_t AudioMixer::getVolume() {
	float currentVolume = 0.f;
	if (needsUpdateMixerEachTime)
		updateWithDefaultEndpoint();
	if (g_endpointVolume) {
		if (g_logarithmic) {
			g_endpointVolume->GetMasterVolumeLevel(&currentVolume);
		}
		else {
			g_endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
			currentVolume *= 100;
		}
	}
	return currentVolume;
}

void AudioMixer::setVolume(vol_t newVolume) {
	if (g_endpointVolume) {
		BOOL isMute;
		if (g_logarithmic) {
			g_endpointVolume->SetMasterVolumeLevel(newVolume, NULL);
		}
		else {
			g_endpointVolume->SetMasterVolumeLevelScalar(newVolume / 100, NULL);
		}
		g_endpointVolume->GetMute(&isMute);
		if (isMute) {
			g_endpointVolume->SetMute(FALSE, NULL);
		}
	}
	StatusWindow::showVolume(g_logarithmic, newVolume);
}

void AudioMixer::incrementVolume(float increment) {
	vol_t volume = getVolume();
	volume += increment;
	if (volume > g_maxVolume)
		volume = g_maxVolume;
	setVolume(volume);
}

void AudioMixer::decrementVolume(float increment) {
	vol_t volume = getVolume();
	volume -= increment;
	if (volume < g_minVolume)
		volume = g_minVolume;
	setVolume(volume);
}

void updateWithDefaultEndpoint() {
	HRESULT hr;
	IMMDevice *defaultDevice = NULL;
	float dummy;

	g_minVolume = 0, g_maxVolume = 100;

	if (g_endpointVolume) {
		g_endpointVolume->Release();
		g_endpointVolume = NULL;
	}

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	//	deviceEnumerator->Release();
	//	deviceEnumerator = NULL;

	if (defaultDevice) {
		hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&g_endpointVolume);
		defaultDevice->Release();
		defaultDevice = NULL;
	}

	if (g_endpointVolume && g_logarithmic)
		g_endpointVolume->GetVolumeRange(&g_minVolume, &g_maxVolume, &dummy);
}
