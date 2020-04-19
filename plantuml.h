#include <filesystem>

#include "db.h"

#pragma once

namespace fs = std::filesystem;

class PlantumlOutput {
public:
	PlantumlOutput(DB &db, fs::path &outputDir) : db(db), outputDir(outputDir) {}

	void addClass(const std::string &className);
	void runClassDiagram();
	void runSequenceDiagram();
	void run();
private:
	DB &db;
	const fs::path outputDir;
};
