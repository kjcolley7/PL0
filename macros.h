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
# define NDEBUG
#endif

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>


#define CMP(op, a, b) (((a) op (b)) ? (a) : (b))
#define MIN(a, b) CMP(<, a, b)
#define MAX(a, b) CMP(>, a, b)

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
#define REQUIRE(expr) (__typeof__(expr))_require((void*)(uintptr_t)(expr), #expr)
static inline void* _require(void* ptr, const char* expr) {
	if(!ptr) {
		printf("Unmet requirement: %s\n", expr);
		abort();
	}
	
	return ptr;
}

/* Fail-fast allocation functions that will abort on allocation failures */
static inline void* malloc_ff(size_t size) {
	void* ret = malloc(size);
	if(!ret) {
		perror("malloc_ff");
		abort();
	}
	
	return ret;
}

static inline void* calloc_ff(size_t count, size_t size) {
	void* ret = calloc(count, size);
	if(!ret) {
		perror("calloc_ff");
		abort();
	}
	
	return ret;
}

static inline void* realloc_ff(void* ptr, size_t size) {
	void* ret = realloc(ptr, size);
	if(!ret) {
		perror("realloc_ff");
		abort();
	}
	
	return ret;
}

static inline char* strdup_ff(const char* str) {
	char* ret = strdup(str);
	if(!ret) {
		perror("strdup_ff");
		abort();
	}
	
	return ret;
}

/* Fail-fast allocating format string printing functions that will abort on failures */
static inline int asprintf_ff(char** pstr, const char* format, ...)
	__attribute__((format(printf, 2, 3)));
static inline int asprintf_ff(char** pstr, const char* format, ...) {
	VARIADIC(format, ap, {
		int ret = vasprintf(pstr, format, ap);
		if(ret == -1) {
			perror("asprintf_ff");
			abort();
		}
		return ret;
	});
}

static inline int vasprintf_ff(char** pstr, const char* format, va_list ap)
	__attribute__((format(printf, 2, 0)));
static inline int vasprintf_ff(char** pstr, const char* format, va_list ap) {
	int ret = vasprintf(pstr, format, ap);
	if(ret == -1) {
		perror("vasprintf_ff");
		abort();
	}
	return ret;
}

static inline char* rsprintf_ff(const char* format, ...)
	__attribute__((format(printf, 1, 2)));
static inline char* rsprintf_ff(const char* format, ...) {
	VARIADIC(format, ap, {
		char* ret;
		if(vasprintf(&ret, format, ap) == -1) {
			perror("rsprintf");
			abort();
		}
		return ret;
	});
}

static inline char* vrsprintf_ff(const char* format, va_list ap)
	__attribute__((format(printf, 1, 0)));
static inline char* vrsprintf_ff(const char* format, va_list ap) {
	char* ret;
	if(vasprintf(&ret, format, ap) == -1) {
		perror("vrsprintf");
		abort();
	}
	return ret;
}

/*! Convert a pointer to a string */
static inline char* ptos(void* ptr) {
	return rsprintf_ff("%p", ptr);
}

/*! Helper macro for expanding a dynamic array. This will zero-initialize the extra
 allocated space and will abort if the requested allocation size cannot be allocated.
 */
#define expand(parr, pcount) \
_expand_array((void**)(parr), sizeof(**(parr)), pcount, sizeof(*(pcount)))
static inline void _expand_array(void** parr, size_t elem_size, void* pcount, size_t count_sz) {
	size_t old_count;
	switch(count_sz) {
		case 1: old_count = *(uint8_t*)pcount; break;
		case 2: old_count = *(uint16_t*)pcount; break;
		case 4: old_count = *(uint32_t*)pcount; break;
		case 8: old_count = *(uint64_t*)pcount; break;
		default: abort();
	}
	
	size_t new_count = MAX(1, old_count * 2);
	
	/* Expand allocation */
	void* ret = realloc_ff(*parr, elem_size * new_count);
	
	/* Zero-fill the expanded space */
	memset((char*)ret + elem_size * old_count, 0, elem_size * (new_count - old_count));
	
	switch(count_sz) {
		case 1: *(uint8_t*)pcount = (uint8_t)new_count; break;
		case 2: *(uint16_t*)pcount = (uint16_t)new_count; break;
		case 4: *(uint32_t*)pcount = (uint32_t)new_count; break;
		case 8: *(uint64_t*)pcount = (uint64_t)new_count; break;
		default: abort();
	}
	*parr = ret;
}

/*! Helper macro for inserting an element into a dynamic array */
#define insert_element(arr, index, elem, count) \
insert_index(arr, sizeof(*(arr)), index, elem, count)
static inline void insert_index(
	void* arr,
	size_t elem_size,
	size_t index,
	const void* elem,
	size_t count
) {
	/* Move from the target position */
	void* target = (char*)arr + index * elem_size;
	void* dst = (char*)target + elem_size;
	size_t move_size = (count - index) * elem_size;
	memmove(dst, target, move_size);
	memcpy(target, elem, elem_size);
}

/*! Helper macro for deleting an element from a dynamic array */
#define delete_element(arr, index, count) \
delete_index(arr, sizeof(*(arr)), index, count)
static inline void delete_index(void* arr, size_t elem_size, size_t index, size_t count) {
	void* dst = (char*)arr + index * elem_size;
	void* src = (char*)dst + elem_size;
	size_t move_size = (count - (index + 1)) * elem_size;
	memmove(dst, src, move_size);
}

/*! Concatenate two strings by doing a += b */
static inline void astrcat(char** a, const char* b) {
	char* tmp = rsprintf_ff("%s%s", *a ?: "", b ?: "");
	destroy(a);
	*a = tmp;
}

#endif /* PL0_MACROS_H */
