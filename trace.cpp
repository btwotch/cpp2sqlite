#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>

#include "trace.h"

namespace fs = std::filesystem;

void Tracefile::putLine(std::string line) {
	std::vector<std::string> fields;
	std::istringstream splitter(line);
	std::string s;
	while (getline(splitter, s, ' ')) {
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
		db.addTrace(exeId, fields[1], fields[2], false, std::stoi(fields[3]));
	} else if (fields[0] == "x") {
		db.addTrace(exeId, fields[1], fields[2], true, std::stoi(fields[3]));
	}

	std::cout << "exe: " << exe << " pid: " << pid << " ppid: " << ppid << " time: " << time << std::endl;
	if (exeId == -1 && pid > 0 && ppid > 0 && !exe.empty() && time > 0) {
		std::cout << "adding trace_file" << std::endl;
		exeId = db.addTraceFile(exe, pid, ppid, time);
	}
}

void traceExec(DB &db, std::filesystem::path execFile) {
	std::unique_ptr<fs::path, std::function<void(fs::path*)>> tmpDir(
		new fs::path(std::tmpnam(nullptr)),
		[](fs::path *p){
			std::filesystem::remove_all(*p);
			delete p;
		});

	fs::create_directories(*tmpDir);
	setenv("CPP2S_TRACE_DIR_OUTPUT", tmpDir->string().c_str(), 1);
	fs::path tracelibPath("tracelib.so");
	setenv("LD_PRELOAD", fs::absolute(tracelibPath).string().c_str(), 1);
	std::system(execFile.string().c_str());

	for (const auto & entry : fs::directory_iterator(*tmpDir)) {
		std::string line;
		std::cout << entry.path() << std::endl;
		std::ifstream traceFile(entry.path());
		Tracefile tf(db);
		while(std::getline(traceFile, line)) {
			tf.putLine(line);
		}
	}
}
