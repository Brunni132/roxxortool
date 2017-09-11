#include "Precompiled.h"
#include "DisableAnimationsForWinTab.h"

static BOOL wereClientAnimationsEnabled = FALSE;
static bool isWinTabCheckTimerActive = false;
static const unsigned CHECK_DELAY = 50;

static BOOL isWinTabActive() {
	char name[128];
	GetClassNameA(GetForegroundWindow(), name, sizeof(name));
	return !strcmp(name, "MultitaskingViewFrame");
}

static void CALLBACK winTabCheckTimerProc(HWND, UINT, UINT_PTR idEvent, DWORD) {
	// End the "thread" when Win+Tab is not active anymore
	if (!isWinTabActive()) {
		KillTimer(NULL, idEvent);
		// Restore animations
		// TODO pas besoin du UPDATEWININIFILE
		SystemParametersInfo(SPI_SETCLIENTAREAANIMATION, NULL, (PVOID)(wereClientAnimationsEnabled ? 1 : 0), /*SPIF_UPDATEINIFILE |*/ SPIF_SENDCHANGE);
		isWinTabCheckTimerActive = false;
		return;
	}
}

void disableAnimationsForDurationOfWinTab() {
	if (!isWinTabCheckTimerActive) {
		isWinTabCheckTimerActive = true;
		SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, NULL, &wereClientAnimationsEnabled, 0);
		if (wereClientAnimationsEnabled) {
			// TODO pas besoin du UPDATEWININIFILE
			SystemParametersInfo(SPI_SETCLIENTAREAANIMATION, NULL, (PVOID)0, /*SPIF_UPDATEINIFILE |*/ SPIF_SENDCHANGE);
			SetTimer(NULL, 0x1001, CHECK_DELAY, winTabCheckTimerProc);
		}
	}
}
