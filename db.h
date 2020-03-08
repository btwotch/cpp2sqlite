#include <iostream>
#include <vector>

#include <sqlite3.h>

#pragma once

struct FunctionData {
	std::string visibility;
	bool isVirtual;
	std::string className;
	std::string functionName;
	std::string filepath;
	int lineNumber;
};

struct ClassData {
	std::string className;
	std::string filepath;
	int lineNumber;
};

class DB {
public:
	DB(const std::string &path);
	~DB();

	void beginTransaction();
	void endTransaction();
	void addClass(const ClassData &cd);
	void addFunction(const FunctionData &cd);
	void addInheritance(const std::string &from, const std::string &to);

	std::vector<ClassData> getClasses();
private:
	sqlite3 *sdb;

	const std::string SQL_CREATE_FUNCTION_DECLARATIONS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_declaration (
			visibility VARCHAR,
			virtual BOOLEAN,
			className VARCHAR,
			functionName VARCHAR,
			filepath VARCHAR,
			lineNumber INTEGER,
			PRIMARY KEY(className, functionName));)";

	const std::string SQL_CREATE_CLASS_DECLARATIONS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		class_declaration (
			className VARCHAR PRIMARY KEY,
			filepath VARCHAR,
			lineNumber INTEGER);)";

	const std::string SQL_CREATE_CLASS_INHERITANCE_TABLE = R"(CREATE TABLE IF NOT EXISTS
		class_inheritance (
			className VARCHAR,
			inherits_from VARCHAR,
			PRIMARY KEY (className, inherits_from));)";

	const std::vector<std::string> createTables = {
		SQL_CREATE_FUNCTION_DECLARATIONS_TABLE,
		SQL_CREATE_CLASS_DECLARATIONS_TABLE,
		SQL_CREATE_CLASS_INHERITANCE_TABLE
	};

	const std::string SQL_INSERT_CLASS_STMT = R"(INSERT OR REPLACE INTO class_declaration
		(className, filePath, lineNumber) VALUES (?, ?, ?);)";
	const std::string SQL_INSERT_FUNCTION_STMT = R"(INSERT OR REPLACE INTO function_declaration
		(visibility, virtual, className, functionName, filePath, lineNumber) VALUES (?, ?, ?, ?, ?, ?);)";
	const std::string SQL_INSERT_INHERITANCE_STMT = R"(INSERT OR REPLACE INTO class_inheritance 
		(className, inherits_from) VALUES (?, ?);)";

	const std::string SQL_SELECT_CLASSES_STMT = R"(SELECT className, filePath, lineNumber from class_declaration;)";
};
