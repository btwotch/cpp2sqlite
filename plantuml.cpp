#include <iostream>
#include <fstream>
#include <filesystem>

#include "plantuml.h"

void PlantumlOutput::addClass(const std::string &className) {
	std::ofstream classFile;
	std::filesystem::path path = outputDir / className;
	path.replace_extension(".pu");
	classFile.open(path);

	classFile << "class " << className << "{ \n";

	for (FunctionData &fd : db.getMethodsOfClass(className)) {
		classFile << "    " << "+" << fd.functionName << "(";
		bool first = true;
		for (const auto &arg : fd.args) {
			if (!first) classFile << ", ";
			first = false;

			classFile << arg.type << " " << arg.name;
		}
		classFile << ")" << "\n";
	}

	classFile << "}\n";
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

	for (const auto &inheritance : db.getClassInheritances()) {
		plantumlFile << inheritance.first << " --|> " << inheritance.second << "\n";
	}
	plantumlFile << "@enduml\n";
}
