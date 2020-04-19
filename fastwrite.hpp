#include <cstdio>
#include <cstring>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class FastWrite {
public:
	FastWrite(const char *filename) {
		fd = open(filename, O_CREAT | O_RDWR, 0600);
		map();
        }

	void write(const char *buf, size_t len, size_t pos) {
		map(len + pos);

		char *cbuf = static_cast<char*>(addr);
		memcpy(&cbuf[pos], buf, len);
		currentPosition = len + pos;
	}

	void append(const char *buf, size_t len) {
		write(buf, len, currentPosition);
	}
	void append(const char *buf) {
		size_t len = strlen(buf);
		append(buf, len);
	}

	~FastWrite() {
		unmap();
		ftruncate(fd, currentLength);
		close(fd);
	}
private:
	void unmap() {
		munmap(addr, currentLength);
		addr = nullptr;
	}

	void map(size_t len) {
		if (len < currentLength) {
			return;
		}
		if (addr != nullptr) {
			unmap();
		}
		size_t mapSize = 1024 + (2 * len);
		ftruncate(fd, mapSize);
		fprintf(stderr, "mapping len: %zu\n", len);
		addr = mmap(NULL, mapSize, PROT_WRITE, MAP_SHARED_VALIDATE, fd, 0);
		currentLength = mapSize;
	}
	void map() {
		map(currentLength + 1);
	}

	void grow() {
		unmap();
		map();
	}

	int fd = -1;
	size_t currentLength = 0;
	size_t currentPosition = 0;
	void *addr = nullptr;
};
