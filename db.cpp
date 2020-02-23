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
		std::cerr << "stmt: " << SQL_INSERT_CLASS_STMT.c_str() << " failed" << std::endl;
	}

	sqlite3_reset(stmt);
}

void DB::addInheritance(const std::string &from, const std::string &to) {
	static sqlite3_stmt *stmt = nullptr;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_INSERT_INHERITANCE_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_text(stmt, 1, to.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, from.c_str(), -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "stmt: " << SQL_INSERT_INHERITANCE_STMT.c_str() << " failed" << std::endl;
	}

	sqlite3_reset(stmt);
}

void DB::addFunction(const FunctionData &fd) {
	static sqlite3_stmt *stmt = nullptr;

	if (stmt == nullptr) {
		sqlite3_prepare_v2(sdb, SQL_INSERT_FUNCTION_STMT.c_str(), -1, &stmt, 0);
	}

	sqlite3_bind_text(stmt, 1, fd.visibility.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, fd.className.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, fd.functionName.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, fd.filepath.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 5, fd.lineNumber);

	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "stmt: " << SQL_INSERT_FUNCTION_STMT.c_str() << " failed" << std::endl;
	}

	sqlite3_reset(stmt);
}
