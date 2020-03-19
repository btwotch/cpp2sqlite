cpp2sqlite: cpp2sqlite.cpp
	clang++ cpp2sqlite.cpp `llvm-config --cflags --libs` -lclang-cpp -o cpp2sqlite -lsqlite3 -Wall -pedantic -ggdb

.PHONY: clean
clean:
	rm -fv cpp2sqlite test.db core.*
