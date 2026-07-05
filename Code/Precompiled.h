
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <TlHelp32.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "lib/Ref.h"
#include "lib/gason.h"
#include "lib/JsonSerializer.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <list>
#include "TaskManager.h"

template <typename...> inline constexpr bool always_false_v = false;

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}
