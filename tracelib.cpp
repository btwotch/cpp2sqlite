#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <thread>

#include "fastwrite.hpp"

class __traceOutput {
public:
        __traceOutput() {
		init();
		fw = nullptr;
        }

        ~__traceOutput() {
		if(fw != nullptr) {
			delete fw;
			fw = nullptr;
		}
	}

	void enterFunc(void *func, void *caller) __attribute__((always_inline)) {
		init();
		char line[256];
		snprintf(line, sizeof(line), "e %p %p %lu\n", func, caller, time(nullptr));
		fw->append(line);
	}

	void exitFunc(void *func, void *caller) __attribute__((always_inline)) {
		init();
		char line[256];
		snprintf(line, sizeof(line), "x %p %p %lu\n", func, caller, time(nullptr));
		fw->append(line);
	}

	void close() {
		delete fw;
		fw = nullptr;
	}

private:
	void init() __attribute__((always_inline)) {
		char line[1024];
		pid_t pid_new = getpid();
		pid_t tid_new = gettid();
		if (pid_new == pid && tid_new == tid && fw != nullptr) {
			return;
		}
		if (pid > 0 && fw != nullptr) {
			delete fw;
			fw = nullptr;
		}
		pid_t pid_old = pid;
		pid = pid_new;
		tid = tid_new;
		char *dirp = getenv("CPP2S_TRACE_DIR_OUTPUT");
		snprintf(filename, sizeof(filename), "%s/trace.out.%d.%d", dirp, pid, tid);
		fw = new FastWrite(filename);
		snprintf(line, sizeof(line), "i time %lu\n", time(nullptr));
		fw->append(line);
		snprintf(line, sizeof(line), "i pid %d\n", pid);
		fw->append(line);
		snprintf(line, sizeof(line), "i ppid %d\n", getppid());
		fw->append(line);
		snprintf(line, sizeof(line), "i exe %s\n", getExecOfPid(pid));
		fw->append(line);
	}

	char* getExecOfPid(pid_t pid) __attribute__((always_inline)) {
		char path[1024];
		static char exePath[4096];

		snprintf(path, sizeof(path), "/proc/%d/exe", pid);
		ssize_t length = readlink(path, static_cast<char*>(exePath), sizeof(exePath));
		if (length > sizeof(exePath)) {
			return nullptr;
		}
		exePath[length] = '\0';

		return exePath;
	}

	FastWrite *fw = nullptr;
	char filename[4096];
	pid_t pid = -1;
	pid_t tid = -1;
};

static thread_local __traceOutput __tO;

extern "C" {
	void
	__cyg_profile_func_enter (void *func,  void *caller)
	{
		__tO.enterFunc(func, caller);
	}

	void
	__cyg_profile_func_exit (void *func, void *caller)
	{
		__tO.exitFunc(func, caller);
	}

	__attribute__((destructor))
	static void onExit() {
		__tO.close();
	}
}

