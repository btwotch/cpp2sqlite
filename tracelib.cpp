#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>
#include <thread>

class __traceOutput {
public:
        __traceOutput() {
		init();
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
		if (pid_new == pid && tid_new == tid) {
			return;
		}
		if (pid > 0) {
			fclose(fp_trace);
		}
		pid_t pid_old = pid;
		pid = pid_new;
		tid = tid_new;
		std::string dir;
		char *dirp = getenv("CPP2S_TRACE_DIR_OUTPUT");
		if (dirp && dirp[0] != '\0') {
			dir = std::string{dirp} + "/";
		}
		filename = dir + std::string{"trace.out"} + "." + std::to_string(static_cast<int>(pid)) + "." + std::to_string(static_cast<int>(tid));
		fp_trace = fopen(filename.c_str(), "w");
		fprintf(fp_trace, "i time %lu\n", time(nullptr));
		fprintf(fp_trace, "i pid %d\n", pid);
		fprintf(fp_trace, "i exe %s\n", getExecOfPid(pid).c_str());
		fflush(fp_trace);
	}

	std::string getExecOfPid(pid_t pid) __attribute__((always_inline)) {
		char buf[1024];
		std::string exeFile = std::string{"/proc/"} + std::to_string(static_cast<int>(pid)) + "/exe";
		ssize_t length = readlink(exeFile.c_str(), static_cast<char*>(buf), sizeof(buf));
		if (length > 1023) {
			return std::string{};
		}
		buf[length] = '\0';
		return std::string{buf};
	}

	FILE *fp_trace = nullptr;
	std::string filename;
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

