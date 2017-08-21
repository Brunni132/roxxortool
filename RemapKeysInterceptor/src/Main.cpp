#include <stdio.h>
#include "plugins/MacBookPro2012Plugin.h"
#include "plugins\RoxxorPlugin.h"
#include "utils.h"

const RemappingPlugin plugins[] = {
	{ virtualFnRemappingPlugin, false },
	{ macRemappingPlugin, true },
	//{ roxxorRemappingPlugin, false },
};

int main() {
	RemappingPluginContext context;
	raise_process_priority();
	context.runMainLoop(plugins, numberof(plugins));
	return 0;
}
