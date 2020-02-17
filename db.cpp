#include <iostream>

#include "db.h"

DB::DB(const std::string &path) {
	if (sqlite3_open(path.c_str(), &sdb) != SQLITE_OK) {
		std::cout << "could not open " << path << ": " << sqlite3_errmsg(sdb) << std::endl;
	}

}

DB::~DB() {
	sqlite3_close(sdb);

}

void DB::beginTransaction() {
}
void DB::endTransaction() {
}
void DB::addFuncCall(const FunctionCallDataFrom &from, const FunctionCallData &to) {
}
void DB::addClass(const ClassData &cd) {
}

