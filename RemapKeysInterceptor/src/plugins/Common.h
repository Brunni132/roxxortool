#pragma once

#include "../PluginContext.h"

struct RemappingEntry {
	unsigned source, dest;
};

typedef const RemappingEntry* RemappingTable;

bool processRemappingTable(RemappingPluginContext &context, RemappingTable table, int tableCount);
