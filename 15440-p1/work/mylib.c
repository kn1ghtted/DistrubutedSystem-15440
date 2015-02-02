#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <stdint.h>

#define MAXMSGLEN 100

int sockfd;

void init_socket() {
	static char *serverip;
	static char *serverport;
	static unsigned short port;
	static int rv;
	static struct sockaddr_in srv;

	// Get environment variable indicating the ip address of the server
	serverip = getenv("server15440");
	if (serverip) fprintf(stderr ,"Got environment variable server15440: %s\n", serverip);
	else {
		fprintf(stderr ,"Environment variable server15440 not found.  Using 127.0.0.1\n");
		serverip = "127.0.0.1";
	}

	// Get environment variable indicating the port of the server
	serverport = getenv("serverport15440");
	if (serverport) fprintf(stderr, "Got environment variable serverport15440: %s\n", serverport);
	else {
		fprintf(stderr, "Environment variable serverport15440 not found.  Using 15440\n");
		serverport = "15440";
	}
	port = (unsigned short)atoi(serverport);

	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); // TCP/IP socket
	if (sockfd<0) err(1, 0);      // in case of error

	// setup address structure to point to server
	memset(&srv, 0, sizeof(srv));     // clear it first
	srv.sin_family = AF_INET;     // IP family
	srv.sin_addr.s_addr = inet_addr(serverip);  // IP address of server
	srv.sin_port = htons(port);     // server port

	// actually connect to the server
	rv = connect(sockfd, (struct sockaddr*)&srv, sizeof(struct sockaddr));
	if (rv<0) err(1,0);
}

void send_to_server(const char* msg) {
	send(sockfd, msg, strlen(msg), 0);
}

void send_int_to_server(int32_t* msg, const int32_t size) {
	send(sockfd, msg, size * sizeof(int32_t), 0);
}

#include <dlfcn.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "../include/dirtree.h"
//#define MAX_SIZE 4000

// The following line declares a function pointer with the same prototype as the open function.  
int (*orig_open)(const char *pathname, int flags, ...);  // mode_t mode is needed when flags includes O_CREAT
int (*orig_close)(int fildes);
ssize_t (*orig_read)(int fildes, void *buf, size_t nbyte);
ssize_t (*orig_write)(int fildes, const void *buf, size_t nbyte);
off_t (*orig_lseek)(int fildes, off_t offset, int whence);
//int (*orig_stat)(const char *path, struct stat *buf);
int (*orig_unlink)(const char *path);
ssize_t (*orig_getdirentries)(int fd, char *buf, size_t nbytes , off_t *basep);
struct dirtreenode* (*orig_getdirtree)(const char *path);
void (*orig_freedirtree)(struct dirtreenode* dt);
int (*orig_xstat)(int ver, const char * path, struct stat * stat_buf);


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
	//marshall
	int32_t length = strlen(pathname);
	//int* open = malloc(sizeof(int));
	int open = 0;
	send_int_to_server(&open, 1); // open for 0
	//send_int_to_server(length, 1);
	char* msg = malloc(length*sizeof(char));
	memcpy(msg, pathname, length*sizeof(char));
	send_to_server(msg);
	send_int_to_server(&flags, 1);
	send_int_to_server((int32_t*)&m, 1);
	int rv, cur_fd;
	int32_t buf[1];
	if((rv = recv(sockfd, buf, sizeof(int32_t), 0) > 0)) {
		//buf[rv] = 0;
		//cur_fd = buf[0] - '0';
		cur_fd = *(int*)buf;
		fprintf(stderr, "finish open , fd is %d", cur_fd);
		return cur_fd;
	}
	//fprintf(stderr, "fd is %s\n", buf);
	//return orig_open(pathname, flags, m);
	return 0;
}

ssize_t read(int fildes, void *buf, size_t nbyte) {
	fprintf(stderr, "mylib: read called for fd %d\n", fildes);
	int read =1;
	send_int_to_server(&read,1); // read for 1
	send_int_to_server(&fildes,1); //send fildes
	send_int_to_server((int32_t*)&nbyte, 1);
	int rv, numb_bytes;
	int32_t buffer[1];
	//fprintf(stderr, "ready");
	//fprintf(stderr, "sockfd is %i \n", sockfd);
	//fprintf(stderr, "buffer is %i \n", *buffer);
	//fprintf(stderr, "numb_bytes is %i \n", numb_bytes);
	if((rv = recv(sockfd, buffer, sizeof(int32_t), 0) > 0)) {
		numb_bytes = *(int32_t*)buffer;
		//fprintf(stderr, "recv buffer is %i \n", buffer[0]);
		fprintf(stderr, "read in size \n");
	}

	fprintf(stderr, "sockfd is %i \n", sockfd);
	if(numb_bytes == 0) {
		return 0;
	}
	if((rv = recv(sockfd, buf, numb_bytes, 0) > 0)) {
		//fprintf(stderr, "recv buffer is %i \n", buffer[0]);
		fprintf(stderr, "finishi read in buf \n");
	}
	fprintf(stderr, "jump");
	return numb_bytes;
	//return 0;
	//return orig_read(fildes, buf, nbyte);
}

ssize_t write(int fildes, const void *buf, size_t nbyte) {
	fprintf(stderr, "mylib: write called for fd %d\n", fildes);
	int write = 2;
	send_int_to_server(&write,1); // write for 2
	send_int_to_server(&fildes,1); //send fildes
	send_int_to_server((int32_t*)&nbyte, 1);
	int rv, numb_bytes;
	int32_t buffer[1];
	if(nbyte == 0) {
		return 0;
	}
	char* msg = malloc(nbyte);
	memcpy(msg, buf, nbyte);
	send_to_server(msg);
	//send_to_server("write\n");
	if((rv = recv(sockfd, buffer, sizeof(int32_t), 0) > 0)) {
		//buffer[rv] = 0;
		//numb_bytes = buffer[0] - '0';
		numb_bytes = *(int32_t*)buffer;
		//return numb_bytes;
	}
	//if((rv = recv(sockfd, (void*)buf, numb_bytes, 0) > 0)) {
	//	//fprintf(stderr, "recv buffer is %i \n", buffer[0]);
	//	fprintf(stderr, "finishi write in buf \n");
	//}
	return numb_bytes;
	//return orig_write(fildes, buf, nbyte);
}

int close(int fildes) {
	fprintf(stderr, "mylib: close called for fd %d\n", fildes);
	int close = 3;
	send_int_to_server(&close,1); // close for 3
	send_int_to_server(&fildes,1);
	int rv, numb_bytes;
	int buf[1];
	if((rv = recv(sockfd, buf, sizeof(int), 0) > 0)) {
		//buf[rv] = 0;
		//numb_bytes = buf[0] - '0';
		numb_bytes = *(int*)buf;
		return numb_bytes;
	}
	return 0;
	//send_to_server("close\n");
	//return orig_close(fildes);
}

off_t lseek(int fildes, off_t offset, int whence) {
	fprintf(stderr, "mylib: lseek called for fd %d\n", fildes);
	send_to_server("lseek\n");
	return orig_lseek(fildes, offset, whence);
}

//int stat(const char *path, struct stat *buf) {
//	fprintf(stderr, "mylib: stat called for path %s", path);
//  send_to_server("stat\n");
//  return orig_stat(path, buf);
//}

int unlink(const char *path) {
	fprintf(stderr, "mylib: unlink called for path %s\n", path);
	send_to_server("unlink\n");
	return orig_unlink(path);
}

ssize_t getdirentries(int fd, char *buf, size_t nbytes , off_t *basep) {
	fprintf(stderr, "mylib: orig_getdirentries called for fd %d\n", fd);
	send_to_server("getdirentries\n");
	return orig_getdirentries(fd, buf, nbytes, basep);
}

struct dirtreenode* getdirtree(const char *path) {
	fprintf(stderr, "mylib: orig_getdirtree called for path %s\n", path);
	send_to_server("getdirtree\n");
	return orig_getdirtree(path);
}

void freedirtree(struct dirtreenode* dt) {
	fprintf(stderr, "mylib: freedirtree called for path %s\n", dt->name);
	send_to_server("freedirtree\n");
	orig_freedirtree(dt);
}

int __xstat(int ver, const char * path, struct stat * stat_buf) {
	fprintf(stderr, "mylib: __xstat called for path %s", path);
	//send_to_server("stat\n");
	send_to_server("__xstat\n");
	return orig_xstat(ver, path, stat_buf);
}

// This function is automatically called when program is started
void _init(void) {
	// set function pointer orig_open to point to the original open function
	orig_open = dlsym(RTLD_NEXT, "open");
	orig_close = dlsym(RTLD_NEXT, "close");
	orig_read = dlsym(RTLD_NEXT, "read");
	orig_write = dlsym(RTLD_NEXT, "write");
	orig_lseek = dlsym(RTLD_NEXT, "lseek");
	orig_xstat = dlsym(RTLD_NEXT, "__xstat");
	orig_unlink = dlsym(RTLD_NEXT, "unlink");
	orig_getdirentries = dlsym(RTLD_NEXT, "getdirentries");
	orig_getdirtree = dlsym(RTLD_NEXT, "getdirtree");
	orig_freedirtree = dlsym(RTLD_NEXT, "freedirtree");

	init_socket();

	fprintf(stderr, "Init mylib\n");
}
