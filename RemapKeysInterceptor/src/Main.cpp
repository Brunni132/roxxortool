#include <stdio.h>
#include "plugins/MacBookPro2012Plugin.h"
#include "plugins\RoxxorPlugin.h"
#include "utils.h"
#ifdef _WIN32_BUILD
#	include <windows.h>
#endif

const RemappingPlugin plugins[] = {
	{ virtualFnRemappingPlugin, false },
	{ macRemappingPlugin, true },
	//{ roxxorRemappingPlugin, false },
};

#ifdef _WIN32_BUILD
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#else
int main() {
#endif
	RemappingPluginContext context;
	raise_process_priority();
	context.runMainLoop(plugins, numberof(plugins));
	return 0;
}
