#include <iostream>
#include <vector>

#include <sqlite3.h>

#pragma once

struct FunctionDataArgument {
	std::string type;
	std::string name;
};

struct FunctionData {
	std::string visibility;
	bool isVirtual;
	std::string className;
	std::string functionName;
	std::string filepath;
	std::string returnTypeName;
	int lineNumber;
	std::vector<FunctionDataArgument> args;
};

struct VarData {
	std::string className;
	std::string type;
	std::string name;
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
	void addVarData(const VarData &vd);

	std::vector<ClassData> getClasses();
	std::vector<std::pair<std::string, std::string> > getClassInheritances();
	std::vector<FunctionData> getMethodsOfClass(const std::string &className);
	std::vector<VarData> getVarsOfClass(const std::string &className);

private:
	sqlite3 *sdb;

	std::vector<FunctionDataArgument> getArgsOfMethod(const uint64_t id);

	const std::string SQL_CREATE_FUNCTION_DECLARATIONS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_declaration (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			visibility VARCHAR,
			virtual BOOLEAN,
			className VARCHAR,
			returnTypeName VARCHAR,
			functionName VARCHAR,
			filepath VARCHAR,
			lineNumber INTEGER);)";

	const std::string SQL_CREATE_FUNCTION_ARGS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_args (
			function INTEGER REFERENCES function_declaration(id),
			type VARCHAR,
			name VARCHAR);)";

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

	const std::string SQL_CREATE_VARS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		var_declaration (
			className VARCHAR,
			type VARCHAR,
			name VARCHAR,
			PRIMARY KEY (className, name));)";

	const std::vector<std::string> createTables = {
		SQL_CREATE_FUNCTION_DECLARATIONS_TABLE,
		SQL_CREATE_CLASS_DECLARATIONS_TABLE,
		SQL_CREATE_CLASS_INHERITANCE_TABLE,
		SQL_CREATE_VARS_TABLE,
		SQL_CREATE_FUNCTION_ARGS_TABLE
	};

	const std::string SQL_INSERT_CLASS_STMT = R"(INSERT OR REPLACE INTO class_declaration
		(className, filePath, lineNumber) VALUES (?, ?, ?);)";
	const std::string SQL_INSERT_FUNCTION_STMT = R"(INSERT OR REPLACE INTO function_declaration
		(visibility, virtual, returnTypeName, className, functionName, filePath, lineNumber) VALUES (?, ?, ?, ?, ?, ?, ?);)";
	const std::string SQL_INSERT_FUNCTION_ARG_STMT = R"(INSERT INTO function_args
		(function, type, name) VALUES (?, ?, ?);)";
	const std::string SQL_INSERT_INHERITANCE_STMT = R"(INSERT OR REPLACE INTO class_inheritance 
		(className, inherits_from) VALUES (?, ?);)";
	const std::string SQL_INSERT_VAR_STMT = R"(INSERT OR REPLACE INTO var_declaration
		(className, type, name) VALUES (?, ?, ?);)";

	const std::string SQL_SELECT_CLASSES_STMT = R"(SELECT className, filePath, lineNumber from class_declaration;)";
	const std::string SQL_SELECT_INHERITANCES_STMT = R"(SELECT className, inherits_from from class_inheritance;)";
	const std::string SQL_SELECT_METHODS_OF_CLASS_STMT = R"(SELECT id, visibility, virtual, returnTypeName, functionName, filepath, lineNumber FROM function_declaration WHERE className = ?;)";
	const std::string SQL_SELECT_ARGS_OF_METHOD_STMT = R"(SELECT type, name FROM function_args WHERE function = ?;)";
	const std::string SQL_SELECT_VARS_OF_CLASS_STMT = R"(SELECT type, name FROM var_declaration WHERE className = ?;)";
};
