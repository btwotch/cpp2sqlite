#include <iostream>
#include <vector>

#include <sqlite3.h>

#include "db.h"

DB::DB(const std::string &path) {
	if (sqlite3_open(path.c_str(), &sdb) != SQLITE_OK) {
		std::cout << "could not open " << path << ": " << sqlite3_errmsg(sdb) << std::endl;
	}

	for (const std::string &stmt : createTables) {
		char *errmsg = nullptr;

		if (sqlite3_exec(sdb, stmt.c_str(), nullptr, 0, &errmsg) != SQLITE_OK) {
			std::cerr << "stmt: " << stmt << " failed: " << errmsg << std::endl;
			exit(1);
		}
	}
}

DB::~DB() {
	sqlite3_close(sdb);

}

void DB::beginTransaction() {
	if (sqlite3_exec(sdb, "BEGIN TRANSACTION;", nullptr, 0, nullptr) != SQLITE_OK) {
		std::cerr << "begin transaction failed" << std::endl;
		exit(1);
	}
}
void DB::endTransaction() {
	if (sqlite3_exec(sdb, "END TRANSACTION;", nullptr, 0, nullptr) != SQLITE_OK) {
		std::cerr << "end transaction failed" << std::endl;
		exit(1);
	}
}
void DB::addClass(const ClassData &cd) {
	static sqlite3_stmt *stmt = nullptr;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_INSERT_CLASS_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_text(stmt, 1, cd.className.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, cd.filepath.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, cd.lineNumber);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "stmt: " << SQL_INSERT_CLASS_STMT.c_str() << " failed" << sqlite3_errmsg(sdb) << std::endl;
	}

	sqlite3_reset(stmt);
}

std::vector<ClassData> DB::getClasses() {
	static sqlite3_stmt *stmt = nullptr;

	char *className = nullptr;
	char *filepath = nullptr;
	int lineNumber = -1;

	std::vector<ClassData> ret;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_SELECT_CLASSES_STMT.c_str(), -1, &stmt, 0);
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		className = (char*) sqlite3_column_text(stmt, 0);
		filepath = (char*) sqlite3_column_text(stmt, 1);
		lineNumber = sqlite3_column_int(stmt, 2);

		ClassData cd;
		cd.className = std::string{className};
		cd.filepath = std::string{filepath};
		cd.lineNumber = lineNumber;

		ret.emplace_back(cd);
	}
	sqlite3_reset(stmt);

	return ret;
}

void DB::addInheritance(const std::string &from, const std::string &to) {
	static sqlite3_stmt *stmt = nullptr;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_INSERT_INHERITANCE_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_text(stmt, 1, to.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, from.c_str(), -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "stmt: " << SQL_INSERT_INHERITANCE_STMT.c_str() << " failed" << sqlite3_errmsg(sdb) << std::endl;
	}

	sqlite3_reset(stmt);
}

std::vector<std::pair<std::string, std::string> > DB::getClassInheritances() {
	static sqlite3_stmt *stmt = nullptr;
	std::vector<std::pair<std::string, std::string> > ret;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_SELECT_INHERITANCES_STMT.c_str(), -1, &stmt, 0);
	}

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		char *className = nullptr;
		char *inheritsFrom = nullptr;

		className = (char*) sqlite3_column_text(stmt, 0);
		inheritsFrom = (char*) sqlite3_column_text(stmt, 1);

		std::pair<std::string, std::string> inheritance = {std::string{className}, std::string{inheritsFrom}};
		ret.emplace_back(inheritance);
	}
	sqlite3_reset(stmt);

	return ret;
}

void DB::addFunction(const FunctionData &fd) {
	static sqlite3_stmt *stmt = nullptr;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_INSERT_FUNCTION_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_text(stmt, 1, fd.visibility.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, fd.isVirtual);
	sqlite3_bind_text(stmt, 3, fd.className.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, fd.functionName.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 5, fd.filepath.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 6, fd.lineNumber);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "stmt: " << SQL_INSERT_FUNCTION_STMT.c_str() << " failed: " << sqlite3_errmsg(sdb) << std::endl;
	}

	uint64_t id = sqlite3_last_insert_rowid(sdb);
	for (const auto& arg : fd.args) {
		static sqlite3_stmt *stmt = nullptr;
		if (stmt == nullptr) {
			sqlite3_prepare_v2(sdb, SQL_INSERT_FUNCTION_ARG_STMT.c_str(), -1, &stmt, 0);
		}

		sqlite3_bind_int(stmt, 1, id);
		sqlite3_bind_text(stmt, 2, arg.type.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, arg.name.c_str(), -1, SQLITE_STATIC);
		if (sqlite3_step(stmt) != SQLITE_DONE) {
			std::cerr << "stmt: " << SQL_INSERT_FUNCTION_STMT.c_str() << " failed: " << sqlite3_errmsg(sdb) << std::endl;
		}

		sqlite3_reset(stmt);
	}

	sqlite3_reset(stmt);
}

std::vector<FunctionDataArgument> DB::getArgsOfMethod(const uint64_t id) {
	static sqlite3_stmt *stmt = nullptr;
	std::vector<FunctionDataArgument> ret;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_SELECT_ARGS_OF_METHOD_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_int(stmt, 1, id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		char *type = nullptr;
		char *name = nullptr;
		type = (char*) sqlite3_column_text(stmt, 0);
		name= (char*) sqlite3_column_text(stmt, 1);

		FunctionDataArgument fda;
		fda.type = std::string{type};
		fda.name = std::string{name};
		ret.emplace_back(fda);
	}

	sqlite3_reset(stmt);
	return ret;
}

std::vector<FunctionData> DB::getMethodsOfClass(const std::string &className) {
	static sqlite3_stmt *stmt = nullptr;
	std::vector<FunctionData> ret;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_SELECT_METHODS_OF_CLASS_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_text(stmt, 1, className.c_str(), -1, SQLITE_STATIC);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		FunctionData fd;
		fd.className = className;

		char *visibility = nullptr;
		bool isVirtual;
		char *functionName = nullptr;
		char *filepath = nullptr;
		int lineNumber = -1;
		uint64_t id = -1;

		id = sqlite3_column_int(stmt, 0);
		visibility = (char*) sqlite3_column_text(stmt, 1);
		isVirtual = (bool)sqlite3_column_int(stmt, 2);
		functionName = (char*) sqlite3_column_text(stmt, 3);
		filepath = (char*) sqlite3_column_text(stmt, 4);
		lineNumber = sqlite3_column_int(stmt, 5);

		fd.visibility = std::string{visibility};
		fd.isVirtual = isVirtual;
		fd.functionName = std::string{functionName};
		fd.filepath = std::string{filepath};
		fd.lineNumber = lineNumber;
		fd.args = getArgsOfMethod(id);

		ret.emplace_back(fd);
	}
	sqlite3_reset(stmt);

	return ret;
}
