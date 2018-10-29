#include "Precompiled.h"
#include "Config.h"

#define IMPLEMENT_BOOL_PROP(name, default)      { if (obj) mkentry(""#name); if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) found(), name = obj->value.toBool(); }
#define IMPLEMENT_UNSIGNED_PROP(name, default)	{ if (obj) mkentry(""#name); if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) found(), name = (unsigned) obj->value.toNumber(); }
#define IMPLEMENT_INT_PROP(name, default)       { if (obj) mkentry(""#name); if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) found(), name = (int) obj->value.toNumber(); }
#define IMPLEMENT_FLOAT_PROP(name, default)     { if (obj) mkentry(""#name); if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) found(), name = (float) obj->value.toNumber(); }
#define IMPLEMENT_ARRAY_PROP(name, default)     { if (obj) mkentry(""#name); if (serializer) serializer->put(""#name, name, numberof(name)); else if (!obj) memcpy(name, default, sizeof(name)); else if (!strcmp(obj->key, ""#name)) found(), parseNumberArray(name, numberof(name), obj->value); }
//#define IMPLEMENT_STRING_MAP_PROP(name, default)	{ propId++; if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (obj && !strcmp(obj->key, ""#name)) found = true, parseStringMap(name); }

Config config;

void Config::process(JsonNode *obj, JsonWriterNode *serializer) {
	// Checking whether the key exists
	// If obj != null, foundProps should be all null (means that properties are found) and for each process call (key in the JSON) there thisEntryFound should be true.
	int propId = -1;
	bool thisEntryFound = false;
	auto mkentry = [&](const char *key) {
		propId++;
		if (foundProps.size() < propId + 1) {
			foundProps.push_back(key);
		}
	};
	auto found = [&] {
		foundProps[propId] = nullptr;
		thisEntryFound = true;
	};

	unsigned short defaultGammaCurve[3 * 256];
	memset(defaultGammaCurve, 0, sizeof(defaultGammaCurve));

	//IMPLEMENT_BOOL_PROP(rightCtrlContextMenu, false);
	IMPLEMENT_BOOL_PROP(toggleHideFolders, true);
	IMPLEMENT_BOOL_PROP(startScreenSaverWithInsert, false);
	IMPLEMENT_BOOL_PROP(smoothVolumeControl, true);
	IMPLEMENT_FLOAT_PROP(volumeIncrementQuantity, 1.5f);
	IMPLEMENT_BOOL_PROP(ddcCiBrightnessControl, true);
	IMPLEMENT_BOOL_PROP(wmiLogarithmicBrightness, false);
	IMPLEMENT_INT_PROP(brightnessIncrementQuantity, 5);
	IMPLEMENT_BOOL_PROP(allowNegativeBrightness, true);
	IMPLEMENT_INT_PROP(autoApplyGammaCurveDelay, 0);
	IMPLEMENT_BOOL_PROP(useSoftMediaKeys, false);
	IMPLEMENT_BOOL_PROP(forceReapplyGammaOnBrightnessChange, false);
	IMPLEMENT_BOOL_PROP(reloadConfigWithCtrlWinR, true);
	IMPLEMENT_BOOL_PROP(iAmAMac, false);
	IMPLEMENT_BOOL_PROP(multiDesktopLikeApplicationSwitcher, true);
	IMPLEMENT_BOOL_PROP(winFOpensYourFiles, true);
	IMPLEMENT_BOOL_PROP(winHHidesWindow, true);
	IMPLEMENT_BOOL_PROP(noNumPad, false);
	IMPLEMENT_BOOL_PROP(rightShiftContextMenu, false);
	IMPLEMENT_INT_PROP(brightnessCacheDuration, 3000);
	IMPLEMENT_BOOL_PROP(closeWindowWithWinQ, false);
	IMPLEMENT_BOOL_PROP(altGraveToStickyAltTab, false);
	IMPLEMENT_BOOL_PROP(winTSelectsLastTask, false);
	IMPLEMENT_INT_PROP(winTTaskMoveBy, 4);
	IMPLEMENT_BOOL_PROP(disableWinTabAnimation, false);
	IMPLEMENT_BOOL_PROP(altTabWithMouseButtons, false);
	IMPLEMENT_BOOL_PROP(selectHiraganaByDefault, false);
	IMPLEMENT_BOOL_PROP(japaneseMacBookPro, false);
	IMPLEMENT_BOOL_PROP(japaneseWindowsKeyboard, false);
	IMPLEMENT_BOOL_PROP(japaneseMacKeyboard, false);
	IMPLEMENT_BOOL_PROP(winSSuspendsSystem, false);
	IMPLEMENT_BOOL_PROP(doNotUseWinSpace, false);
	IMPLEMENT_BOOL_PROP(internationalUsKeyboardForFrench, false);
	IMPLEMENT_BOOL_PROP(resetDefaultGammaCurve, false);
	IMPLEMENT_FLOAT_PROP(scrollAccelerationFactor, 0.0f);
	IMPLEMENT_INT_PROP(scrollAccelerationIntertia, 50);
	IMPLEMENT_FLOAT_PROP(scrollAccelerationMaxScrollFactor, 2000);
	IMPLEMENT_BOOL_PROP(scrollAccelerationSendMultipleMessages, false);
	IMPLEMENT_BOOL_PROP(scrollAccelerationDismissTrackpad, true);

	if (obj && !thisEntryFound) {
		char error[1024];
		sprintf(error, "Unrecognized entry %s", obj->key);
		errors.push_back(error);
	}
}

void Config::readFile() {
	// Read & parse file
	FILE *fp = fopen("config.json", "rb");
	if (!fp) {
		switch (MessageBox(NULL, "Cannot open config.json, do you want to create one for you?\nInspect it, then start the app again.", "Cannot start", MB_ICONWARNING | MB_OKCANCEL)) {
		case IDOK:
			writeSampleFile("config.json", true);
		case IDCANCEL:
			exit(EXIT_FAILURE);
			break;
		}
	}
	fseek(fp, 0, SEEK_END);
	size_t buffer_size = ftell(fp) + 1;
	fseek(fp, 0, SEEK_SET);
	char *buffer = (char *)malloc(buffer_size);
	fread(buffer, 1, buffer_size - 1, fp);
	fclose(fp);
	buffer[buffer_size - 1] = 0;
	char *endptr;
	JsonValue value;
	JsonAllocator allocator;
	JsonParseStatus status = json_parse(buffer, &endptr, &value, allocator);
	if (status != JSON_PARSE_OK) {
		char error[1024];
		sprintf(error, "Error at %ld, status: %d\n", long(endptr - buffer), status);
		errors.push_back(error);
	}
	else {
		// Init to default values
		foundProps.clear();
		process();

		// Parse file itself
		for (auto obj : value)
			process(obj);

		for (auto prop : foundProps) {
			if (prop) {
				char error[1024];
				sprintf(error, "Missing entry %s", prop);
				errors.push_back(error);
			}
		}
	}

	if (!errors.empty()) {
		std::string message("The following happened when loading the JSON config file:");
		for (string &error : errors) {
			message.append("\n- ").append(error);
		}
		message.append("\nWe can fix the file for you if you click on Yes (current is renamed as config.old.json), start as is if you click on No, or quit if you click on Cancel.");

		switch (MessageBox(NULL, message.c_str(), "Invalid config file", MB_ICONWARNING | MB_YESNOCANCEL)) {
		case IDYES:
			if (!MoveFile("config.json", "config.old.json")) {
				LPSTR messageBuffer = nullptr;
				size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
				MessageBox(NULL, messageBuffer, "Unable to rename the file to config.old.json", MB_ICONEXCLAMATION);
				exit(EXIT_FAILURE);
			}
			else
				writeSampleFile("config.json", false);
			break;
		case IDCANCEL:
			exit(EXIT_FAILURE);
			break;
		case IDNO:
			break;
		}
	}
}

void Config::parseNumberArray(unsigned short array[], unsigned maxLength, JsonValue &val) {
	int index = 0;
	if (val.getTag() != JSON_TAG_ARRAY) {
		printf("Expected an array (consts)\n");
		return;
	}
	memset(array, 0, maxLength * sizeof(array[0]));
	for (auto obj : val) {
		if (index >= int(maxLength))
			break;
		array[index++] = short(obj->value.toNumber());
	}
}

void Config::writeSampleFile(const char *fname, bool showInExplorer) {
	JsonWriterNode node;
	FILE *outFile = fopen(fname, "w");
	if (outFile) {
		process(NULL, &node);
		writeJson(node, outFile);
		fclose(outFile);

		if (showInExplorer) {
			char directoryAnsi[1024];
			char fullCommand[1080];
			GetCurrentDirectory(numberof(directoryAnsi), directoryAnsi);
			sprintf(fullCommand, "/select,%s\\%s", directoryAnsi, fname);
			ShellExecute(NULL, "open", "explorer.exe", fullCommand, directoryAnsi, SW_SHOW);
		}
	}
	else {
		MessageBox(NULL, "Unable to create file in this directory, please check your rights", "Error", MB_ICONEXCLAMATION);
	}
}
