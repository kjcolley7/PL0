//
//  macros.h
//  PL/0
//
//  Created by Kevin Colley on 3/14/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#ifndef PL0_MACROS_H
#define PL0_MACROS_H

/* Negated debug option, used by <assert.h> to disable asserts for release builds */
#if !DEBUG
# define NDEBUG 1
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>


/* On release builds, ASSERT(cond) tells the compiler that cond must be true */
#if NDEBUG
#define ASSERT(expr) do { \
	if(!(expr)) { \
		__builtin_unreachable(); \
	} \
} while(0)
#else /* NDEBUG */
#define ASSERT(expr) assert(expr)
#endif /* NDEBUG */


#define CMP(op, a, b) ({ \
	__typeof__((a)+(b)) _a = (a); \
	__typeof__(_a) _b = (b); \
	(_a op _b) ? _a : _b; \
})
#define MIN(a, b) CMP(<, a, b)
#define MAX(a, b) CMP(>, a, b)

/*! Test if the flag varaible has all of the given flags set */
#define HAS_ALL_FLAGS(flagvar, testflags) ({ \
	__typeof__(flagvar) _testflags = (testflags); \
	((flagvar) & _testflags) == _testflags; \
})

/*! Test if the flag variable has any of the given flags set */
#define HAS_ANY_FLAGS(flagvar, testflags) (!!((flagvar) & (testflags)))

/*! Frees the pointer pointed to by the parameter and then sets it to NULL */
#define destroy(pptr) do { \
	__typeof__(*(pptr)) volatile* _pptr = (pptr); \
	if(!_pptr) { \
		break; \
	} \
	free(*_pptr); \
	*_pptr = NULL; \
} while(0)

/*! Used to implement a variadic function */
#define VARIADIC(param, ap, ...) do { \
	va_list ap; \
	va_start(ap, param); \
	{__VA_ARGS__} \
	va_end(ap); \
} while(0)

/*! Used to pop a variadic argument in a type-safe manner */
#define VA_POP(ap, lvalue) do { \
	lvalue = va_arg(ap, __typeof__(lvalue)); \
} while(0)

/*! Number of elements in an array */
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

/*! Require an expression to be nonzero, and return it */
#define REQUIRE(expr) \
(__typeof__(expr))_require((uintptr_t)(expr), #expr)
static inline uintptr_t _require(uintptr_t val, const char* expr) {
	if(!val) {
		printf("Unmet requirement: %s\n", expr);
		abort();
	}
	
	return val;
}

#define REQUIRE_PERROR(expr, perror_message) \
(__typeof__(expr))_require_perror((uintptr_t)(expr), perror_message)
static inline uintptr_t _require_perror(uintptr_t val, const char* perror_message) {
	if(!val) {
		perror(perror_message);
		abort();
	}
	
	return val;
}

/* Fail-fast allocation functions that will abort on allocation failures */
static inline void* malloc_ff(size_t size) {
	return REQUIRE_PERROR(malloc(size), "malloc_ff");
}

static inline void* calloc_ff(size_t count, size_t size) {
	return REQUIRE_PERROR(calloc(count, size), "calloc_ff");
}

static inline void* realloc_ff(void* ptr, size_t size) {
	return REQUIRE_PERROR(realloc(ptr, size), "realloc_ff");
}

static inline char* strdup_ff(const char* str) {
	return REQUIRE_PERROR(strdup(str), "strdup_ff");
}

static inline FILE* fopen_ff(const char* fname, const char* mode) {
	return REQUIRE_PERROR(fopen(fname, mode), fname);
}

/* Fail-fast allocating format string printing functions that will abort on failures */
static inline int asprintf_ff(char** pstr, const char* format, ...)
	__attribute__((format(printf, 2, 3)));
static inline int asprintf_ff(char** pstr, const char* format, ...) {
	VARIADIC(format, ap, {
		int ret = vasprintf(pstr, format, ap);
		(void)REQUIRE_PERROR(ret != -1, "asprintf_ff");
		return ret;
	});
}

static inline int vasprintf_ff(char** pstr, const char* format, va_list ap)
	__attribute__((format(printf, 2, 0)));
static inline int vasprintf_ff(char** pstr, const char* format, va_list ap) {
	int ret = vasprintf(pstr, format, ap);
	(void)REQUIRE_PERROR(ret != -1, "vasprintf_ff");
	return ret;
}

static inline char* rsprintf_ff(const char* format, ...)
	__attribute__((format(printf, 1, 2)));
static inline char* rsprintf_ff(const char* format, ...) {
	VARIADIC(format, ap, {
		char* ret;
		(void)REQUIRE_PERROR(vasprintf(&ret, format, ap) != -1, "rsprintf_ff");
		return ret;
	});
}

static inline char* vrsprintf_ff(const char* format, va_list ap)
	__attribute__((format(printf, 1, 0)));
static inline char* vrsprintf_ff(const char* format, va_list ap) {
	char* ret;
	(void)REQUIRE_PERROR(vasprintf(&ret, format, ap) != -1, "vrsprintf_ff");
	return ret;
}

/*! Convert a pointer to a string */
static inline char* ptos(void* ptr) {
	return rsprintf_ff("%p", ptr);
}

#define UNIQUIFY(macro, args...) UNIQUIFY_(macro, __COUNTER__, ##args)
#define UNIQUIFY_(macro, id, args...) UNIQUIFY__(macro, id, ##args)
#define UNIQUIFY__(macro, id, args...) macro(id, ##args)

#include "dynamic_array.h"
#include "dynamic_string.h"

#endif /* PL0_MACROS_H */
