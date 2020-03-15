#include <CLI/CLI.hpp>
#include <iostream>
#include <filesystem>
#include <string>

#include "plantuml.h"
#include "cpp2sqlite.h"

int main(int argc, char **argv) {
	CLI::App app("CPP2SQLITE and PLANTUML");
	app.set_help_all_flag("--help-all", "Expand all help");

	CLI::App *parse = app.add_subcommand("parse", "parse C++ code");
	std::filesystem::path compileCommandDir;
	std::filesystem::path databaseFile = "cpp2sqlite.db";
	parse->add_option("path to compile_command.json directory", compileCommandDir, "Directory name")->required()->check(CLI::ExistingDirectory);
	parse->add_option("--dbname", databaseFile, "database path", true)->check(CLI::NonexistentPath);
	CLI::App *plantuml = app.add_subcommand("plantuml", "generate plantuml files from database");
	std::filesystem::path outputDir;
	plantuml->add_option("path to database", databaseFile, "File name")->required()->check(CLI::ExistingFile);
	plantuml->add_option("output directory", outputDir, "Directory name")->required()->check(CLI::ExistingDirectory);

	app.require_subcommand(1);
	CLI11_PARSE(app, argc, argv);

	DB d(databaseFile);
	if (*plantuml) {
		PlantumlOutput pu(d, outputDir);
		pu.run();
	} else if (*parse) {
		cpp2sqlite(d, compileCommandDir);
	}

	return 0;
}
