cpp2sqlite: cpp2sqlite.cpp db.cpp
	clang++ cpp2sqlite.cpp db.cpp `llvm-config --cflags --libs` -lclang-cpp -o cpp2sqlite -lsqlite3 -Wall -pedantic -ggdb -fsanitize=address -fno-omit-frame-pointer -std=c++17

.PHONY: clean
clean:
	rm -fv cpp2sqlite test.db core.*
