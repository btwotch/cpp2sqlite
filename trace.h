#include <filesystem>
#include <map>
#include <tuple>

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
	void saveCallingAddrs(const std::string &first, const std::string &second);
	void initSymbolizer();

	DB &db;
	uint64_t exeId = -1;
	pid_t pid = -1;
	pid_t ppid = -1;
	std::string exe;
	time_t time = -1;
	bool initialized = false;
	llvm::symbolize::LLVMSymbolizer *Symbolizer;
	std::map<std::string, bool> addrInfoStored;
	std::map<std::tuple<std::string, std::string>, bool> callingTupleStored;
	std::map<std::tuple<std::string, std::string>, bool> backCallingTupleStored;
	bool omitTime = true;
};

void traceExec(DB &db, std::filesystem::path execFile);

