#include <filesystem>

#include "llvm/DebugInfo/Symbolize/Symbolize.h"

#include "db.h"

#pragma once

namespace fs = std::filesystem;

class Tracefile {
public:
	Tracefile(DB &db) : db(db) {}
	~Tracefile();

	void putLine(std::string line);

private:
	AddrInfo addr2line(const std::string &addr);
	void initSymbolizer();

	DB &db;
	uint64_t exeId = -1;
	pid_t pid = -1;
	pid_t ppid = -1;
	std::string exe;
	time_t time = -1;
	bool initialized = false;
	llvm::symbolize::LLVMSymbolizer *Symbolizer;
};

void traceExec(DB &db, std::filesystem::path execFile);

