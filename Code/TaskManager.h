#pragma once

enum NamedTask {
	TASKID_UNNAMED = 0,
	TASKID_SWITCH_TO_HIRAGANA
};

namespace TaskManager {

	void init();
	void terminate();

	uint64_t CurrentTime();
	extern bool isInTeamViewer;
	extern bool isBeingRemoteDesktopd;

	void Run(std::function<void()> function);
	void RunNamed(NamedTask name, std::function<void()> function);
	void RunLater(std::function<void()> function, int delayMs);
	// Cancels any task with the same name
	void RunNamedLater(NamedTask taskName, std::function<void()> function, int delayMs);
	void CancelNamed(NamedTask taskName);

	void RunLaterOnSameThread(std::function<void()> code, int delayMs = 0);
	void RunNamedOnSameThread(NamedTask taskId, int delayMs, std::function<void()> code, std::function<void()> onFinish);
}

