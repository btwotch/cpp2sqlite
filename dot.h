#include <filesystem>

#include "db.h"

#pragma once

namespace fs = std::filesystem;

class DotOutput {
public:
	DotOutput(DB &db, fs::path &outputDir) : db(db), outputDir(outputDir) {}

	void run();
private:
	DB &db;
	const fs::path outputDir;
};
