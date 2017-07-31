#include "Common.h"
#include "MacBookPro2012Plugin.h"
#include "RoxxorPlugin.h"

bool processRemappingTable(RemappingPluginContext &context, RemappingTable table, int tableCount) {
	unsigned scanCode = context.strokeId;
	if (!scanCode) return false;
	for (int i = 0; i < tableCount; i += 1) {
		if (scanCode == table[i].source) {
			return context.replaceKey(table[i].dest);
		}
	}
	return false;
}
