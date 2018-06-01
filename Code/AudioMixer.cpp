#include "Precompiled.h"
#include "AudioMixer.h"
#include "StatusWindow.h"
using namespace AudioMixer;

static bool needsUpdateMixerEachTime = true;			// Card with two separate channels for headphones and speakers
static IAudioEndpointVolume *g_endpointVolume = NULL;
static IMMDeviceEnumerator *deviceEnumerator = NULL;
static float g_minVolume, g_maxVolume;

static void updateWithDefaultEndpoint();

void AudioMixer::init() {
	HRESULT hr;
	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	updateWithDefaultEndpoint();
}

void AudioMixer::terminate() {
	CoUninitialize();
}

vol_t AudioMixer::getVolume() {
	float currentVolume;
	if (needsUpdateMixerEachTime)
		updateWithDefaultEndpoint();
	if (g_endpointVolume)
		g_endpointVolume->GetMasterVolumeLevel(&currentVolume);
	return currentVolume;
}

void AudioMixer::setVolume(vol_t newVolume) {
	if (g_endpointVolume)
		g_endpointVolume->SetMasterVolumeLevel(newVolume, NULL);
	StatusWindow::showVolume(newVolume);
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

	if (g_endpointVolume)
		g_endpointVolume->GetVolumeRange(&g_minVolume, &g_maxVolume, &dummy);
}
