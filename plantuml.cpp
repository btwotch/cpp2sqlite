#include <iostream>
#include <fstream>
#include <filesystem>

#include "plantuml.h"

void PlantumlOutput::addClass(const std::string &className) {
	std::ofstream classFile;
	std::filesystem::path path = outputDir / className;
	path.replace_extension(".pu");
	classFile.open(path);

	classFile << "class " << className << "\n";
}

void PlantumlOutput::run() {
	std::vector<ClassData> classes = db.getClasses();

	std::ofstream plantumlFile;
	std::filesystem::path path = outputDir / "index";
	path.replace_extension(".pu");
	plantumlFile.open(path);

	plantumlFile << "@startuml\n";
	for (const ClassData &cd : classes) {
		addClass(cd.className);
		plantumlFile << "!include " << cd.className << ".pu\n";
	}
	plantumlFile << "@enduml\n";
}
