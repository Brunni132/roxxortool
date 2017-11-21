#include "Precompiled.h"
#include "Config.h"

#define IMPLEMENT_BOOL_PROP(name, default)	if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) name = obj->value.toBool();
#define IMPLEMENT_UNSIGNED_PROP(name, default)	if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) name = (unsigned) obj->value.toNumber();
#define IMPLEMENT_INT_PROP(name, default)	if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) name = (int) obj->value.toNumber();
#define IMPLEMENT_FLOAT_PROP(name, default)	if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (!strcmp(obj->key, ""#name)) name = (float) obj->value.toNumber();
#define IMPLEMENT_ARRAY_PROP(name, default)	if (serializer) serializer->put(""#name, name, numberof(name)); else if (!obj) memcpy(name, default, sizeof(name)); else if (!strcmp(obj->key, ""#name)) parseNumberArray(name, numberof(name), obj->value);
//#define IMPLEMENT_STRING_MAP_PROP(name, default)	if (serializer) serializer->put(""#name, name); else if (!obj) name = default; else if (obj && !strcmp(obj->key, ""#name)) parseStringMap(name);

Config config;

void Config::process(JsonNode *obj, JsonWriterNode *serializer) {
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
	IMPLEMENT_INT_PROP(autoApplyGammaCurveDelay, 0);
	IMPLEMENT_BOOL_PROP(useSoftMediaKeys, false);
	IMPLEMENT_BOOL_PROP(forceReapplyGammaOnBrightnessChange, false);
	IMPLEMENT_BOOL_PROP(useCustomGammaCurve, false);
	IMPLEMENT_ARRAY_PROP(customGammaCurveArray, defaultGammaCurve);
	IMPLEMENT_FLOAT_PROP(customGammaCurveGamma, 1.0f);
	IMPLEMENT_BOOL_PROP(reloadConfigWithCtrlWinR, true);
	IMPLEMENT_BOOL_PROP(iAmAMac, false);
	IMPLEMENT_BOOL_PROP(multiDesktopLikeApplicationSwitcher, true);
	IMPLEMENT_BOOL_PROP(winFOpensYourFiles, true);
	IMPLEMENT_BOOL_PROP(winHHidesWindow, true);
	IMPLEMENT_BOOL_PROP(noNumPad, false);
	IMPLEMENT_BOOL_PROP(rightShiftContextMenu, false);
	IMPLEMENT_INT_PROP(brightnessCacheDuration, 60000);
	IMPLEMENT_BOOL_PROP(closeWindowWithWinQ, false);
	IMPLEMENT_BOOL_PROP(altGraveToStickyAltTab, false);
	IMPLEMENT_BOOL_PROP(winTSelectsLastTask, false);
	IMPLEMENT_INT_PROP(winTTaskMoveBy, 4);
	IMPLEMENT_BOOL_PROP(disableWinTabAnimation, false);
	IMPLEMENT_BOOL_PROP(altTabWithMouseButtons, false);
	IMPLEMENT_BOOL_PROP(selectHiraganaByDefault, false);
	IMPLEMENT_BOOL_PROP(japaneseMacBookPro, false);
	IMPLEMENT_BOOL_PROP(winSSuspendsSystem, false);
}

bool Config::readFile() {
	// Read & parse file
	FILE *fp = fopen("config.json", "rb");
	if (!fp) {
		fputs("error: can not open config.json\n", stderr);
		return false;
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
		fprintf(stderr, "Error at %ld, status: %d\n", long(endptr - buffer), status);
		return false;
	}

	// Init to default values
	process();

	// Parse file itself
	for (auto obj : value)
		process(obj);

	// writeSampleFile("sample.json");
	return true;
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

void Config::writeSampleFile(const char *fname) {
	JsonWriterNode node;
	FILE *outFile = fopen(fname, "w");
	if (outFile) {
		process(NULL, &node);
		writeJson(node, outFile);
		fclose(outFile);
	}
}
