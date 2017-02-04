//
//  tee.c
//  PL/0
//
//  Created by Kevin Colley on 9/9/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#include "tee.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "object.h"

/* Data structure passed to custom file operation functions */
struct tee_cookie {
	FILE* fp;
	int stdfd;
};


/* The custom file operation functions have different type signatures on linux vs BSD/Mac */
#ifdef __linux

static ssize_t tee_read(void* cookie, char* buf, size_t size) {
	struct tee_cookie* cook = cookie;
	return read(fileno(cook->fp), buf, size);
}

static ssize_t tee_write(void* cookie, const char* buf, size_t size) {
	struct tee_cookie* cook = cookie;
	ssize_t ret = write(fileno(cook->fp), buf, size);
	if(ret <= 0) {
		return ret;
	}
	
	size_t toWrite = ret;
	while(toWrite > 0) {
		ssize_t written = write(cook->stdfd, buf, size);
		if(written <= 0) {
			break;
		}
		
		toWrite -= written;
	}
	
	fflush(cook->fp);
	return ret;
}

static int tee_seek(void* cookie, off64_t* offset, int whence) {
	struct tee_cookie* cook = cookie;
	lseek(cook->stdfd, *offset, whence);
	*offset = lseek(fileno(cook->fp), *offset, whence);
	return (int)*offset;
}

#else /* __linux */

static int tee_read(void* cookie, char* buf, int nbyte) {
	struct tee_cookie* cook = cookie;
	return (int)read(fileno(cook->fp), buf, nbyte);
}

static int tee_write(void* cookie, const char* buf, int size) {
	struct tee_cookie* cook = cookie;
	int ret = (int)write(fileno(cook->fp), buf, size);
	if(ret <= 0) {
		return ret;
	}
	
	int toWrite = ret;
	while(toWrite > 0) {
		int written = (int)write(cook->stdfd, buf, size);
		if(written <= 0) {
			break;
		}
		
		toWrite -= written;
	}
	
	fflush(cook->fp);
	return ret;
}

static fpos_t tee_seek(void* cookie, fpos_t offset, int whence) {
	struct tee_cookie* cook = cookie;
	lseek(cook->stdfd, offset, whence);
	return (fpos_t)lseek(fileno(cook->fp), offset, whence);
}

#endif /* __linux */


/* Close is the same on both platforms */
static int tee_close(void* cookie) {
	/* Don't close stdout/stderr, just fp */
	struct tee_cookie* cook = cookie;
	int ret = fclose(cook->fp);
	destroy(&cookie);
	return ret;
}

FILE* ftee(FILE* fp, FILE* stdfp) {
	FILE* ret;
	
	/* Allocate cookie */
	struct tee_cookie* cook = malloc_ff(sizeof(*cook));
	
	/* Initialize files in tee cookie */
	cook->fp = fp;
	cook->stdfd = fileno(stdfp);
	
#ifdef __linux
	_IO_cookie_io_functions_t tee_funcs = {
		.read  = &tee_read,
		.write = &tee_write,
		.seek  = &tee_seek,
		.close = &tee_close
	};
	ret = fopencookie(cook, "w+", tee_funcs);
#else /* __linux */
	ret = funopen(cook, &tee_read, &tee_write, &tee_seek, &tee_close);
#endif /* __linux */
	
	return ret;
}
