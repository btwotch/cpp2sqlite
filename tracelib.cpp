#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <string>
#include <thread>

class __traceOutput {
public:
        __traceOutput() {
		init();
		fp_trace = nullptr;
        }

        ~__traceOutput() {
		if(fp_trace != nullptr) {
			fclose(fp_trace);
		}
	}

	void enterFunc(void *func, void *caller) __attribute__((always_inline)) {
		init();
		fprintf(fp_trace, "e %p %p %lu\n", func, caller, time(nullptr));
		fflush(fp_trace);
	}

	void exitFunc(void *func, void *caller) __attribute__((always_inline)) {
		init();
		fprintf(fp_trace, "x %p %p %lu\n", func, caller, time(nullptr));
		fflush(fp_trace);
	}
private:
	void init() __attribute__((always_inline)) {
		pid_t pid_new = getpid();
		pid_t tid_new = gettid();
		if (pid_new == pid && tid_new == tid && fp_trace != nullptr) {
			return;
		}
		if (pid > 0 && fp_trace != nullptr) {
			fclose(fp_trace);
		}
		pid_t pid_old = pid;
		pid = pid_new;
		tid = tid_new;
		char *dirp = getenv("CPP2S_TRACE_DIR_OUTPUT");
		snprintf(filename, sizeof(filename), "%s/trace.out.%d.%d", dirp, pid, tid);
		fp_trace = fopen(filename, "w");
		if (fp_trace == nullptr) {
			fprintf(stderr, "could not open %s: %s\n", filename, strerror(errno));
		}
		fprintf(fp_trace, "i time %lu\n", time(nullptr));
		fprintf(fp_trace, "i pid %d\n", pid);
		fprintf(fp_trace, "i ppid %d\n", getppid());
		fprintf(fp_trace, "i exe %s\n", getExecOfPid(pid));
		fflush(fp_trace);
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

	FILE *fp_trace = nullptr;
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
}

