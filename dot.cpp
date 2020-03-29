#include <iostream>
#include <fstream>
#include <filesystem>

#include "dot.h"

void DotOutput::run() {
	std::vector<EnrichedTrace> ets = db.getEnrichedTraces();

	std::ofstream dotFile;
	std::filesystem::path path = outputDir / "graph";
	path.replace_extension(".dot");
	dotFile.open(path);

	std::cout << "enriched traces length: " << ets.size() << std::endl;
	dotFile << "digraph graphname {\n";

	for (const auto &et : ets) {
		dotFile << "\t\"" << et.caller.className << "::" << et.caller.functionName << "\" -> \"" << et.callee.className << "::" << et.callee.functionName << "\"" << std::endl;
	}
	dotFile << "\n}\n";
}
