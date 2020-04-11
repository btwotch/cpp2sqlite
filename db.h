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
	std::vector<std::string> manglings;
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

struct AddrInfo {
	std::string executable;
	std::string addr;
	std::string symbol;
	std::string symbol_demangled;
	std::string file;
	uint32_t line;
};

struct CallInfo {
	std::string className;
	std::string functionName;
	int lineNumber;
};

struct EnrichedTrace {
	CallInfo caller;
	CallInfo callee;
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
	uint64_t addTraceFile(std::string exe, pid_t pid, pid_t ppid, time_t time);
	void addTrace(uint64_t trace_file, const std::string &callee, const std::string &caller, bool exit, time_t time);
	void addAddrInfo(const AddrInfo &ai);

	std::vector<ClassData> getClasses();
	std::vector<std::pair<std::string, std::string> > getClassInheritances();
	std::vector<FunctionData> getMethodsOfClass(const std::string &className);
	std::vector<VarData> getVarsOfClass(const std::string &className);
	std::vector<EnrichedTrace> getEnrichedTraces();

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

	const std::string SQL_CREATE_FUNCTION_MANGLINGS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_manglings (
			function INTEGER REFERENCES function_declaration(id),
			name VARCHAR);)";

	const std::string SQL_CREATE_FUNCTION_ARGS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		function_args (
			function INTEGER REFERENCES function_declaration(id),
			type VARCHAR,
			name VARCHAR);)";

	const std::string SQL_CREATE_CLASS_DECLARATIONS_TABLE = R"(CREATE TABLE IF NOT EXISTS
		class_declaration (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			className VARCHAR UNIQUE,
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

	const std::string SQL_CREATE_TRACE_FILES_TABLE = R"(CREATE TABLE IF NOT EXISTS
		trace_files (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			exe VARCHAR,
			pid INTEGER,
			ppid INTEGER,
			time DATE);)";

	const std::string SQL_CREATE_TRACES_TABLE = R"(CREATE TABLE IF NOT EXISTS
		traces (
			trace_file INTEGER REFERENCES trace_files(id),
			callee VARCHAR,
			caller VARCHAR,
			exit BOOLEAN,
			time DATE);)";

	const std::string SQL_CREATE_ADDR_INFO_TABLE = R"(CREATE TABLE IF NOT EXISTS
		addr_info (
			exe VARCHAR REFERENCES trace_files(exe),
			addr VARCHAR,
			symbol VARCHAR,
			symbol_demangled VARCHAR,
			file VARCHAR,
			line INTEGER,
			PRIMARY KEY (exe, addr));)";

	const std::vector<std::string> createTables = {
		SQL_CREATE_FUNCTION_DECLARATIONS_TABLE,
		SQL_CREATE_CLASS_DECLARATIONS_TABLE,
		SQL_CREATE_CLASS_INHERITANCE_TABLE,
		SQL_CREATE_VARS_TABLE,
		SQL_CREATE_FUNCTION_ARGS_TABLE,
		SQL_CREATE_FUNCTION_MANGLINGS_TABLE,
		SQL_CREATE_TRACE_FILES_TABLE,
		SQL_CREATE_TRACES_TABLE,
		SQL_CREATE_ADDR_INFO_TABLE
	 };

	const std::string SQL_INSERT_CLASS_STMT = R"(INSERT OR REPLACE INTO class_declaration
		(className, filePath, lineNumber) VALUES (?, ?, ?);)";
	const std::string SQL_INSERT_FUNCTION_STMT = R"(INSERT OR REPLACE INTO function_declaration
		(visibility, virtual, returnTypeName, className, functionName, filePath, lineNumber) VALUES (?, ?, ?, ?, ?, ?, ?);)";
	const std::string SQL_INSERT_FUNCTION_MANGLING_STMT = R"(INSERT INTO function_manglings
		(function, name) VALUES (?, ?);)";
	const std::string SQL_INSERT_FUNCTION_ARG_STMT = R"(INSERT INTO function_args
		(function, type, name) VALUES (?, ?, ?);)";
	const std::string SQL_INSERT_INHERITANCE_STMT = R"(INSERT OR REPLACE INTO class_inheritance 
		(className, inherits_from) VALUES (?, ?);)";
	const std::string SQL_INSERT_VAR_STMT = R"(INSERT OR REPLACE INTO var_declaration
		(className, type, name) VALUES (?, ?, ?);)";
	const std::string SQL_INSERT_TRACE_FILE_STMT = R"(INSERT OR REPLACE INTO trace_files
		(exe, pid, ppid, time) VALUES (?, ?, ?, ?);)";
	const std::string SQL_INSERT_TRACE_STMT = R"(INSERT OR REPLACE INTO traces
		(trace_file, callee, caller, exit, time) VALUES (?, ?, ?, ?, ?);)";
	const std::string SQL_INSERT_ADDR_INFO_STMT = R"(INSERT OR REPLACE INTO addr_info
		(exe, addr, symbol, symbol_demangled, file, line) VALUES (?, ?, ?, ?, ?, ?);)";
	// TODO: build transitive hull only for symbols outside of source code dir
	const std::string SQL_INSERT_TRACES_HULL_STMT = R"(
		WITH is_calling(cr, ce) AS
			(SELECT caller, caller
				FROM traces
			UNION SELECT i.cr, callee
				FROM is_calling i, traces tt
			WHERE i.ce=tt.caller)
		INSERT INTO test_trace_hull(caller, callee) SELECT * FROM is_calling WHERE cr <> ce ORDER BY cr;
	)";

	const std::string SQL_SELECT_CLASSES_STMT = R"(SELECT className, filePath, lineNumber from class_declaration;)";
	const std::string SQL_SELECT_INHERITANCES_STMT = R"(SELECT className, inherits_from from class_inheritance;)";
	const std::string SQL_SELECT_METHODS_OF_CLASS_STMT = R"(SELECT id, visibility, virtual, returnTypeName, functionName, filepath, lineNumber FROM function_declaration WHERE className = ?;)";
	const std::string SQL_SELECT_ARGS_OF_METHOD_STMT = R"(SELECT type, name FROM function_args WHERE function = ?;)";
	const std::string SQL_SELECT_VARS_OF_CLASS_STMT = R"(SELECT type, name FROM var_declaration WHERE className = ?;)";

	const std::string SQL_SELECT_ENRICHED_CALLINGS_STMT = R"(
		SELECT DISTINCT fr.className, fr.functionName, fe.className, fe.functionName
		FROM traces t, addr_info ae, addr_info ar, function_manglings me, function_manglings mr, function_declaration fe, function_declaration fr
		WHERE t.callee = ae.addr
			AND t.caller = ar.addr
			AND me.name = ae.symbol
			AND mr.name = ar.symbol
			AND me.function = fe.id
			AND mr.function = fr.id
	;)";
};
