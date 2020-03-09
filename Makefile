CC = clang++
#CFLAGS = $(shell llvm-config --cflags) -ggdb -fsanitize=address -fno-omit-frame-pointer -std=c++17 -Wall -pedantic
CFLAGS = $(shell llvm-config --cflags) -ggdb -fno-omit-frame-pointer -std=c++17 -Wall -pedantic
#LDFLAGS = -fsanitize=address

#cpp2sqlite: cpp2sqlite.cpp db.cpp db.h plantuml.h plantuml.cpp
#	clang++ cpp2sqlite.cpp db.cpp plantuml.cpp `llvm-config --cflags --libs` -lclang-cpp -o cpp2sqlite -lsqlite3 -Wall -pedantic -ggdb -fsanitize=address -fno-omit-frame-pointer -std=c++17

cpp2sqlite: cpp2sqlite.o db.o plantuml.o
	${CC} `llvm-config  --libs` -lclang-cpp -o cpp2sqlite -lsqlite3 ${LDFLAGS} -std=c++17 $^

%.o: %.cpp %.h Makefile
	${CC} ${CFLAGS} -c $<

.PHONY: clean
clean:
	rm -fv cpp2sqlite test.db core.* *.o *.h.gch
