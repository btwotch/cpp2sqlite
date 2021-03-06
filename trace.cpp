#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>

#include "llvm/DebugInfo/Symbolize/Symbolize.h"

#include "utils.h"
#include "trace.h"

namespace fs = std::filesystem;

void Tracefile::putLine(std::string line) {
	std::vector<std::string> fields;
	std::istringstream splitter(line);
	std::string s;
	while (std::getline(splitter, s, ' ')) {
		fields.emplace_back(s);
	}
	if (fields[0] == "i") {
		if (fields[1] == "time") {
			time = std::stoi(fields[2]);
		} else if (fields[1] == "pid") {
			pid = std::stoi(fields[2]);
		} else if (fields[1] == "ppid") {
			ppid = std::stoi(fields[2]);
		} else if (fields[1] == "exe") {
			exe = fields[2];
		}
	} else if (fields[0] == "e") {
		if (omitTime) {
			std::tuple<std::string, std::string> callers = {fields[1], fields[2]};
			if (callingTupleStored.count(callers)) {
				return;
			}
		}
		db.addTrace(exeId, fields[1], fields[2], false, std::stoi(fields[3]));
		saveCallingAddrs(fields[1], fields[2]);
		if (omitTime) {
			callingTupleStored[std::tuple<std::string, std::string>{fields[1], fields[2]}] = true;
		}
	} else if (fields[0] == "x") {
		if (omitTime) {
			std::tuple<std::string, std::string> callers = {fields[1], fields[2]};
			if (backCallingTupleStored.count(callers)) {
				return;
			}
		}
		db.addTrace(exeId, fields[1], fields[2], true, std::stoi(fields[3]));
		saveCallingAddrs(fields[1], fields[2]);
		if (omitTime) {
			backCallingTupleStored[std::tuple<std::string, std::string>{fields[1], fields[2]}] = true;
		}
	}

	if (!initialized && exeId == -1 && pid > 0 && ppid > 0 && !exe.empty() && time > 0) {
		initSymbolizer();
		exeId = db.addTraceFile(exe, pid, ppid, time);
		initialized = true;
	}
}

void Tracefile::saveCallingAddrs(const std::string &first, const std::string &second) {
	AddrInfo ai;

	if (addrInfoStored.count(first) == 0) {
		ai = addr2line(first);
		if (!ai.executable.empty()) {
			db.addAddrInfo(ai);
		}
		addrInfoStored[first] = true;
	}

	if (addrInfoStored.count(second) == 0) {
		ai = addr2line(second);
		if (!ai.executable.empty()) {
			db.addAddrInfo(ai);
		}
		addrInfoStored[second] = true;
	}
}

void Tracefile::initSymbolizer() {
	llvm::symbolize::LLVMSymbolizer::Options Opts;
	Opts.Demangle = false;
	Opts.RelativeAddresses = true;

	Symbolizer = new llvm::symbolize::LLVMSymbolizer(Opts);
}

Tracefile::~Tracefile() {
	delete Symbolizer;
}

AddrInfo Tracefile::addr2line(const std::string &addrString) {
	AddrInfo ai;

	if (exe.empty()) {
		std::cerr << "exe is empty" << std::endl;
	}
	uint64_t addr = std::stoull(addrString, 0, 16);
	llvm::Expected<llvm::DILineInfo> resOrErr = Symbolizer->symbolizeCode(exe, {addr, llvm::object::SectionedAddress::UndefSection});
	if (!resOrErr) {
		std::cerr << "err in symbolizer" << std::endl;
		return ai;
	}

	const llvm::DILineInfo &lineInfo = resOrErr.get();

	ai.executable = exe;
	ai.addr = addrString;
	ai.symbol = lineInfo.FunctionName;
	ai.file = lineInfo.FileName;
	ai.line = lineInfo.Line;

	return ai;
}

void traceExec(DB &db, std::filesystem::path execFile) {
	TemporaryDir tmpDir("/tmp/cpp2s_trace_dir-XXXXXX");
	setenv("CPP2S_TRACE_DIR_OUTPUT", tmpDir.string().c_str(), 1);
	fs::path tracelibPath("tracelib.so");
	setenv("LD_PRELOAD", fs::absolute(tracelibPath).string().c_str(), 1);
	std::system(execFile.string().c_str());

	for (const auto & entry : fs::directory_iterator(tmpDir)) {
		std::string line;
		std::ifstream traceFile(entry.path());
		std::cout << "Parsing " << entry.path() << std::endl;
		Tracefile tf(db);
		while(std::getline(traceFile, line)) {
			tf.putLine(line);
		}
	}
}
