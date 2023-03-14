#include "Precompiled.h"
#include "Utilities.h"

using namespace std;

const TCHAR* extractFileNameFromPath(const TCHAR* fullPath) {
	const TCHAR* lastSlash = fullPath;

	while (*fullPath) {
		if (*fullPath == '\\' || *fullPath == '/')
			lastSlash = fullPath + 1;
		fullPath++;
	}

	return lastSlash;
}
