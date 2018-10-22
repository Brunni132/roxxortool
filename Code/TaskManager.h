#pragma once

enum NamedTask {
	TASKID_UNNAMED = 0,
	TASKID_SWITCH_TO_HIRAGANA
};

namespace TaskManager {

	void init();
	void terminate();

	ULONGLONG GetSystemTimeInMs();

	void Run(std::function<void()> function);
	void RunNamed(NamedTask name, std::function<void()> function);
	void RunLater(std::function<void()> function, int delay);
	// Cancels any task with the same name
	void RunNamedLater(NamedTask taskId, std::function<void()> function, int delay);
	void CancelTask(NamedTask taskId);
}

