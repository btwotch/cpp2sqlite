#include <CLI/CLI.hpp>
#include <iostream>
#include <filesystem>
#include <string>

#include "plantuml.h"
#include "dot.h"
#include "cpp2sqlite.h"
#include "trace.h"

int main(int argc, char **argv) {
	CLI::App app("CPP2SQLITE and PLANTUML");
	app.set_help_all_flag("--help-all", "Expand all help");

	CLI::App *parse = app.add_subcommand("parse", "parse C++ code");
	std::filesystem::path compileCommandDir;
	std::filesystem::path databaseFile = "cpp2sqlite.db";
	std::filesystem::path outputDir;
	parse->add_option("path to compile_command.json directory", compileCommandDir, "Directory name")->required()->check(CLI::ExistingDirectory);
	parse->add_option("--dbname", databaseFile, "database path")->check(CLI::NonexistentPath);

	CLI::App *plantuml = app.add_subcommand("plantuml", "generate plantuml files from database");
	plantuml->add_option("path to database", databaseFile, "File name")->required()->check(CLI::ExistingFile);
	plantuml->add_option("output directory", outputDir, "Directory name")->required()->check(CLI::ExistingDirectory);

	CLI::App *dot = app.add_subcommand("dot", "generate dot callgraph file from database");
	dot->add_option("path to database", databaseFile, "File name")->required()->check(CLI::ExistingFile);
	dot->add_option("output directory", outputDir, "Directory name")->required()->check(CLI::ExistingDirectory);

	CLI::App *trace = app.add_subcommand("trace", "trace a binary that has been compiled with '-finstrument-functions -g'");
	std::filesystem::path execFile;
	trace->add_option("path to executable", execFile, "File name")->required()->check(CLI::ExistingFile);
	trace->add_option("--dbname", databaseFile, "database path");

	app.require_subcommand(1);
	CLI11_PARSE(app, argc, argv);

	DB d(databaseFile);
	if (*plantuml) {
		PlantumlOutput pu(d, outputDir);
		pu.run();
	} else if (*dot) {
		DotOutput dot(d, outputDir);
		dot.run();
	} else if (*parse) {
		cpp2sqlite(d, compileCommandDir);
	} else if (*trace) {
		traceExec(d, execFile);
	}

	return 0;
}
