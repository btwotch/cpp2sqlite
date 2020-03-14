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
		std::string visibility;
		if (fd.visibility == "public") {visibility = "+";}
		else if (fd.visibility == "protected") {visibility = "#";}
		else if (fd.visibility == "private") {visibility = "-";}
		std::string methodLine = std::string{"    "} + visibility + fd.returnTypeName + " " + fd.functionName + "(";
		bool first = true;
		int argsLineLength = 0;
		for (const auto &arg : fd.args) {
			if (!first) methodLine += ", ";
			first = false;
			if (argsLineLength > 80) {
				argsLineLength = 0;
				methodLine = methodLine + "\\n\\t";
			}

			methodLine += arg.type + " " + arg.name;
			argsLineLength += methodLine.length();
		}
		methodLine = methodLine + ")" + "\n";

		classFile << methodLine;
	}
	classFile << "\n";

	for (VarData &vd : db.getVarsOfClass(className)) {
		classFile << "\t" << vd.type << " " << vd.name << "\n";
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
	plantumlFile << "left to right direction\n";
	for (const ClassData &cd : classes) {
		addClass(cd.className);
		plantumlFile << "!include " << cd.className << ".pu\n";
	}

	for (const auto &inheritance : db.getClassInheritances()) {
		plantumlFile << inheritance.first << " --|> " << inheritance.second << "\n";
	}
	plantumlFile << "@enduml\n";
}
