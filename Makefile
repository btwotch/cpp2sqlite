CC = clang++
#CFLAGS = $(shell llvm-config --cflags) -ggdb -fsanitize=address -fno-omit-frame-pointer -std=c++17 -Wall -pedantic
CFLAGS = $(shell llvm-config --cflags) -ggdb -fno-omit-frame-pointer -std=c++17 -Wall -pedantic
#LDFLAGS = -fsanitize=address

#cpp2sqlite: cpp2sqlite.cpp db.cpp db.h plantuml.h plantuml.cpp
#	clang++ cpp2sqlite.cpp db.cpp plantuml.cpp `llvm-config --cflags --libs` -lclang-cpp -o cpp2sqlite -lsqlite3 -Wall -pedantic -ggdb -fsanitize=address -fno-omit-frame-pointer -std=c++17

all: cpp2sqlite tracelib.so

#LDFLAGS = -D_REENTRANT=1 -DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RTREE -DSQLITE_ENABLE_GEOPOLY -DSQLITE_HAVE_ZLIB -DSQLITE_ENABLE_EXPLAIN_COMMENTS -DSQLITE_ENABLE_DBPAGE_VTAB -DSQLITE_ENABLE_STMTVTAB -DSQLITE_ENABLE_DBSTAT_VTAB /usr/local/lib/libsqlite3.a -ldl -lpthread -lm

cpp2sqlite: cpp2sqlite.o db.o plantuml.o main.o trace.o dot.o
	${CC} $^ `llvm-config  --libfiles --libs --link-static --ldflags` `pkg-config --libs --static sqlite3` -static ${LDFLAGS} -std=c++17 -lstdc++fs
	#${CC} `llvm-config  --libs --link-static` -lclang-cpp -o cpp2sqlite -lsqlite3 ${LDFLAGS} -std=c++17 -lstdc++fs $^

%.o: %.cpp %.h Makefile
	${CC} ${CFLAGS} -c $<

main.o: main.cpp Makefile
	${CC} ${CFLAGS} -c $<

tracelib.so: tracelib.cpp fastwrite.hpp
	${CC} -shared -fPIC  $< -o tracelib.so -O0 -ggdb -ldl -rdynamic

.PHONY: clean
.PHONY: all
.PHONY: mrproper
clean:
	rm -fv cpp2sqlite test.db core.* *.o *.h.gch tracelib.so trace.so

mrproper: clean
	rm -fv cpp2sqlite.db
