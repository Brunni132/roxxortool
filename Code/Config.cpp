#include "Precompiled.h"
#include "Config.h"

Config config;

#define DEFINE_PROPERTY(name, default_value) processProperty(context, obj, serializer, name, ""#name, default_value)

void Config::process(ProcessContext& context, JsonNode *obj, JsonWriterNode *serializer) {
	//unsigned short defaultGammaCurve[3 * 256];
	//memset(defaultGammaCurve, 0, sizeof(defaultGammaCurve));

	bool success =
		DEFINE_PROPERTY(toggleHideFolders, true)
		//|| DEFINE_PROPERTY(rightCtrlContextMenu, false)
		|| DEFINE_PROPERTY(startScreenSaverWithInsert, false)
		|| DEFINE_PROPERTY(smoothVolumeControl, true)
		|| DEFINE_PROPERTY(volumeIncrementQuantity, 1.5f)
		|| DEFINE_PROPERTY(ddcCiBrightnessControl, true)
		|| DEFINE_PROPERTY(wmiLogarithmicBrightness, false)
		|| DEFINE_PROPERTY(brightnessIncrementQuantity, 5)
		|| DEFINE_PROPERTY(allowNegativeBrightness, false)
		|| DEFINE_PROPERTY(autoApplyGammaCurveDelay, 0)
		|| DEFINE_PROPERTY(useSoftMediaKeys, false)
		|| DEFINE_PROPERTY(forceReapplyGammaOnBrightnessChange, false)
		|| DEFINE_PROPERTY(reloadConfigWithCtrlWinR, true)
		|| DEFINE_PROPERTY(iAmAMac, false)
		|| DEFINE_PROPERTY(multiDesktopLikeApplicationSwitcher, true)
		|| DEFINE_PROPERTY(winEOpensThisPC, false)
		|| DEFINE_PROPERTY(winEOpensYourFiles, false)
		|| DEFINE_PROPERTY(winFOpensYourFiles, false)
		|| DEFINE_PROPERTY(winHHidesWindow, true)
		|| DEFINE_PROPERTY(noNumPad, false)
		|| DEFINE_PROPERTY(rightShiftContextMenu, false)
		|| DEFINE_PROPERTY(rightShiftContextMenuOpensExtendedMenu, false)
		|| DEFINE_PROPERTY(brightnessCacheDuration, 3000L)
		|| DEFINE_PROPERTY(closeWindowWithWinQ, false)
		|| DEFINE_PROPERTY(altGraveToStickyAltTab, false)
		|| DEFINE_PROPERTY(winTSelectsLastTask, false)
		|| DEFINE_PROPERTY(winTTaskMoveBy, 4)
		|| DEFINE_PROPERTY(altTabWithMouseButtons, false)
		|| DEFINE_PROPERTY(selectHiraganaByDefault, false)
		|| DEFINE_PROPERTY(selectHiraganaDelay, 200)
		|| DEFINE_PROPERTY(japaneseMacBookPro, false)
		|| DEFINE_PROPERTY(japaneseWindowsKeyboard, false)
		|| DEFINE_PROPERTY(japaneseMacKeyboard, false)
		|| DEFINE_PROPERTY(winSSuspendsSystem, false)
		|| DEFINE_PROPERTY(doNotUseWinSpace, false)
		|| DEFINE_PROPERTY(internationalUsKeyboardForFrench, false)
		|| DEFINE_PROPERTY(resetDefaultGammaCurve, false)
		|| DEFINE_PROPERTY(scrollAccelerationEnabled, false)
		|| DEFINE_PROPERTY(scrollAccelerationFactor, 0.0f)
		|| DEFINE_PROPERTY(scrollAccelerationIntertia, 50)
		|| DEFINE_PROPERTY(scrollAccelerationMaxScrollFactor, 2000.0f)
		|| DEFINE_PROPERTY(scrollAccelerationSendMultipleMessages, false)
		|| DEFINE_PROPERTY(scrollAccelerationBaseValue, 0)
		|| DEFINE_PROPERTY(capsPageControls, false)
		|| DEFINE_PROPERTY(disableCapsLock, false)
		|| DEFINE_PROPERTY(processAltTabWithMouseButtonsEvenFromRdp, false)
		|| DEFINE_PROPERTY(mediaKeysWithCapsLockFnKeys, false)
		|| DEFINE_PROPERTY(mediaKeysWithCapsLockSpaceArrow, false)
		|| DEFINE_PROPERTY(disableWinKey, 0);

	if (obj && !success) {
		context.errors.push_back(string("Unrecognized entry ") + obj->key);
	}
}

template<typename T>
bool Config::processProperty(ProcessContext& context, JsonNode* for_read, JsonWriterNode* for_write, T& property, const char* property_name, const T& default_value) {
	if (!for_read && !for_write) {
		context.found_props[property_name] = false;
		property = default_value;
	}
	else if (for_read && !strcmp(for_read->key, property_name)) {
		context.found_props[property_name] = true;

		if constexpr (std::is_same_v<T, bool>) {
			property = for_read->value.toBool();
		}
		else if constexpr (std::is_same_v<T, std::string>) {
			property = for_read->value.toString();
		}
		else if constexpr (std::is_arithmetic_v<T>) {
			property = static_cast<T>(for_read->value.toNumber());
		}
		else if constexpr (std::is_array_v<T>) { // raw arrays (e.g., int property[5])
			using ElementType = std::remove_extent_t<T>;
			constexpr size_t ArraySize = std::extent_v<T>;

			size_t index = 0;
			if (for_read->value.getTag() != JSON_TAG_ARRAY) {
				context.errors.push_back(property_name + ": expected an array");
				return;
			}

			memset(property, 0, ArraySize * sizeof(property[0]));
			for (auto obj : for_read->value) {
				if (index >= ArraySize) {
					context.errors.push_back(property_name + ": exceeded array capacity");
					return;
				}
				property[index++] = static_cast<ElementType>(obj->value.toNumber());
			}
		}
		else {
			static_assert(always_false_v<T>, "Unsupported type passed to processProperty");
		}
		return true;
	}
	else if (for_write) {
		for_write->put(property_name, property);
	}
	return false;
}

void Config::readFile() {
	ProcessContext context;
	// Init to default values and count recognized properties
	process(context);

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
		context.errors.push_back(error);
	}
	else {
		// Parse file itself
		for (auto obj : value)
			process(context, obj);

		for (auto prop : context.found_props) {
			if (!prop.second) {
				context.errors.push_back(string("Missing entry: ") + prop.first);
			}
		}
	}

	if (!context.errors.empty()) {
		std::string message("The following happened when loading the JSON config file:");
		for (string &error : context.errors) {
			message.append("\n- ").append(error);
		}
		message.append("\nThis can be normal after an update. We can fix the file for you if you click on Yes (current is renamed as config.old.json), start as is if you click on No, or quit if you click on Cancel.");

		switch (MessageBox(NULL, message.c_str(), "Invalid config file", MB_ICONWARNING | MB_YESNOCANCEL)) {
		case IDYES:
			DeleteFile("config.old.json");
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

void Config::writeSampleFile(const char *fname, bool showInExplorer) {
	ProcessContext context;
	JsonWriterNode node;
	FILE *outFile = fopen(fname, "w");
	if (outFile) {
		process(context, nullptr, &node);
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
