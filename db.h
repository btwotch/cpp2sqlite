#include <iostream>

#include <sqlite3.h>

#pragma once

static std::string SQL_CREATE_FUNCTION_DECLARATIONS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_declaration (
			namespaceName VARCHAR,
			className VARCHAR,
			functionName VARCHAR,
			filepath VARCHAR,
			visibility BOOLEAN,
			startLineNumber INTEGER,
			endLineNumber INTEGER);)";

static std::string SQL_CREATE_FUNCTION_CALLSS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_declaration (
			namespaceNameFrom VARCHAR,
			classNameFrom VARCHAR,
			functionNameFrom VARCHAR,
			filepathFrom VARCHAR,
			lineNumberFrom INTEGER,
			namespaceNameTo VARCHAR,
			classNameTo VARCHAR,
			functionNameTo VARCHAR,
			filepathTo VARCHAR);)";

static std::string SQL_CREATE_CLASS_DECLARATIONS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		class_declaration (
			namespaceName VARCHAR,
			className VARCHAR,
			filepath VARCHAR,
			startLineNumber INTEGER,
			endLineNumber INTEGER);)";

struct FunctionCallData {
	std::string namespaceName;
	std::string className;
	std::string functionName;
	std::string filepath;
};

struct FunctionCallDataFrom : public FunctionCallData {
	int currentLineNumber;
};

struct FunctionData : public FunctionCallData{
	bool publicVisibility;
	int startLineNumber;
	int endLineNumber;
};

struct ClassData {
	std::string namespaceName;
	std::string className;
	std::string filepath;
	int startLineNumber;
	int endLineNumber;
};

class DB {
	public:
		DB(const std::string &path);
		~DB();

		void beginTransaction();
		void endTransaction();
		void addFuncCall(const FunctionCallDataFrom &from, const FunctionCallData &to);
		void addClass(const ClassData &cd);

	private:
		sqlite3 *sdb;

};
