#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

// The following line declares a function pointer with the same prototype as the open function.  
int (*orig_open)(const char *pathname, int flags, ...);  // mode_t mode is needed when flags includes O_CREAT
//int (*orig_write)(const char *pathname, int flags, ...);  // mode_t mode is needed when flags includes O_CREAT
ssize_t (*orig_write)(int fd, const void *buf, size_t count);
ssize_t (*orig_read)(int fd, void *buf, size_t count);
int (*orig_close)(int fd);

ssize_t write(int fd, const void *buf, size_t count) {
	return orig_write(fd, buf, count);
}

ssize_t read(int fd, void *buf, size_t count) {
	return orig_read(fd, buf, count);
}

int close(int fd) {
	return orig_close(fd);
}

// This is our replacement for the open function from libc.
int open(const char *pathname, int flags, ...) {
	mode_t m=0;
	if (flags & O_CREAT) {
		va_list a;
		va_start(a, flags);
		m = va_arg(a, mode_t);
		va_end(a);
	}
	// we just print a message, then call through to the original open function (from libc)
	fprintf(stderr, "mylib: open called for path %s\n", pathname);
	return orig_open(pathname, flags, m);
}

// This function is automatically called when program is started
void _init(void) {
	// set function pointer orig_open to point to the original open function
	orig_open = dlsym(RTLD_NEXT, "open");
	orig_write = dlsym(RTLD_NEXT, "write");
	orig_read = dlsym(RTLD_NEXT, "read");
	orig_close = dlsym(RTLD_NEXT, "close");
	fprintf(stderr, "Init mylib\n");
}


