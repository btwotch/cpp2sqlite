#include <filesystem>

#include "db.h"

#pragma once

namespace fs = std::filesystem;

class Tracefile {
public:
	Tracefile(DB &db) : db(db) {
	}

	void putLine(std::string line);

private:
	DB &db;
	uint64_t exeId = -1;
	pid_t pid = -1;
	pid_t ppid = -1;
	std::string exe;
	time_t time = -1;
};

void traceExec(DB &db, std::filesystem::path execFile);

