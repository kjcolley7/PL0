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

/*! Helper macro for defining a dynamic array that holds elements of a given type */
#define dynamic_array(type...) \
struct { \
	size_t count; \
	size_t cap; \
	type* elems; \
}

/*! Helper macro to get the type of an element in a dynamic array */
#define element_type(arr) __typeof__(*(arr).elems)

/*! Helper macro to get the size of an element in a dynamic array */
#define element_size(arr) sizeof(*(arr).elems)

/*! Helper macro for iterating over a dynamic array */
#define foreach(parr, pcur) UNIQUIFY(foreach_, parr, pcur)
#define foreach_(id, parr, pcur) enumerate_(id, parr, _iter_idx_##id, pcur)

/*! Helper macro for iterating over a dynamic array with access to the index variable */
#define enumerate(parr, idx, pcur) UNIQUIFY(enumerate_, parr, idx, pcur)
#define enumerate_(id, parr, idx, pcur) \
for(int _iter_once_idx_##id = 1; _iter_once_idx_##id; _iter_once_idx_##id = 0) \
for(__typeof__(parr) _parr_##id = (parr); _iter_once_idx_##id; _iter_once_idx_##id = 0) \
for(size_t idx = 0; idx < _parr_##id->count; idx++) \
	for(int _iter_once_cur_##id = 1; _iter_once_cur_##id; _iter_once_cur_##id = 0) \
	for(__typeof__(_parr_##id->elems) pcur = &_parr_##id->elems[idx]; \
		_iter_once_cur_##id; _iter_once_cur_##id = 0)

/*! Helper macro for expanding a dynamic array. This will zero-initialize the extra
 allocated space and will abort if the requested allocation size cannot be allocated.
 */
#define expand(parr) do { \
	__typeof__(parr) _expand_parr = (parr); \
	expand_array(&_expand_parr->elems, &_expand_parr->cap); \
} while(0)
#define expand_array(parr, pcap) \
_expand_array((void**)(parr), sizeof(**(parr)), pcap, sizeof(*(pcap)))
static inline void _expand_array(void** parr, size_t elem_size, void* pcap, size_t cap_sz) {
	size_t old_count;
	switch(cap_sz) {
		case 1: old_count = *(uint8_t*)pcap; break;
		case 2: old_count = *(uint16_t*)pcap; break;
		case 4: old_count = *(uint32_t*)pcap; break;
		case 8: old_count = *(uint64_t*)pcap; break;
		default: ASSERT(!"Unexpected cap_sz");
	}
	
	size_t new_count = MAX(1, old_count * 2);
	
	/* Expand allocation */
	void* ret = realloc_ff(*parr, elem_size * new_count);
	
	/* Zero-fill the expanded space */
	memset((char*)ret + elem_size * old_count, 0, elem_size * (new_count - old_count));
	
	switch(cap_sz) {
		case 1: *(uint8_t*)pcap = (uint8_t)new_count; break;
		case 2: *(uint16_t*)pcap = (uint16_t)new_count; break;
		case 4: *(uint32_t*)pcap = (uint32_t)new_count; break;
		case 8: *(uint64_t*)pcap = (uint64_t)new_count; break;
	}
	*parr = ret;
}

/*! Helper macro to expand a dynamic array when it's full */
#define expand_if_full(parr) do { \
	__typeof__(parr) _expand_if_full_parr = (parr); \
	if(_expand_if_full_parr->count == _expand_if_full_parr->cap) { \
		expand(_expand_if_full_parr); \
	} \
} while(0)

/*! Helper macro to append an element to the end of a dynamic array */
#define append(parr, elem) do { \
	__typeof__(parr) _append_parr = (parr); \
	expand_if_full(_append_parr); \
	_append_parr->elems[_append_parr->count++] = elem; \
} while(0)

/*! Helper macro for inserting an element into a dynamic array */
#define insert_element(parr, index, pelem) do { \
	__typeof__(parr) _insert_element_parr = (parr); \
	expand_if_full(_insert_element_parr); \
	insert_element_array(_insert_element_parr->elems, index, pelem, _insert_element_parr->count++); \
} while(0)
#define insert_element_array(arr, index, pelem, count) \
insert_index_array(arr, sizeof(*(arr)), index, pelem, count)
static inline void insert_index_array(
	void* arr,
	size_t elem_size,
	size_t index,
	const void* pelem,
	size_t count
) {
	/* Move from the target position */
	void* target = (char*)arr + index * elem_size;
	void* dst = (char*)target + elem_size;
	size_t move_size = (count - index) * elem_size;
	memmove(dst, target, move_size);
	memcpy(target, pelem, elem_size);
}

/*! Helper macro for removing an element from a dynamic array by its pointer */
#define remove_element(parr, pelem) do { \
	__typeof__(parr) _remove_element_parr = (parr); \
	remove_index(_remove_element_parr, (pelem) - _remove_element_parr->elems); \
} while(0)

/*! Helper macro for removing an element from a dynamic array, assuming index is valid */
#define remove_index(parr, index) do { \
	__typeof__(parr) _remove_index_parr = (parr); \
	remove_index_array(_remove_index_parr->elems, index, _remove_index_parr->count--); \
} while(0)
#define remove_index_array(arr, index, count) \
_remove_index_array(arr, sizeof(*(arr)), index, count)
static inline void _remove_index_array(void* arr, size_t elem_size, size_t index, size_t count) {
	void* dst = (char*)arr + index * elem_size;
	void* src = (char*)dst + elem_size;
	size_t move_size = (count - (index + 1)) * elem_size;
	memmove(dst, src, move_size);
}

/*! Helper macro to clear the contents of an array */
#define clear_array(parr) do { \
	__typeof__(parr) _clear_array_parr = (parr); \
	destroy(&_clear_array_parr->elems); \
	_clear_array_parr->count = _clear_array_parr->cap = 0; \
} while(0)

/*! Helper macro to destroy all elements in an array and then the array itself */
#define destroy_array(parr) do { \
	__typeof__(parr) _destroy_array_parr = (parr); \
	foreach(_destroy_array_parr, _destroy_array_pcur) { \
		destroy(_destroy_array_pcur); \
	} \
	clear_array(_destroy_array_parr); \
} while(0)

/*! Helper macro to release all elements in an array and then destroy the array */
#define release_array(parr) do { \
	__typeof__(parr) _release_array_parr = (parr); \
	foreach(_release_array_parr, _release_array_pcur) { \
		release(_release_array_pcur); \
	} \
	clear_array(_release_array_parr); \
} while(0)

/*! Concatenate two strings by doing a += b */
static inline void astrcat(char** a, const char* b) {
	char* tmp = rsprintf_ff("%s%s", *a ?: "", b ?: "");
	destroy(a);
	*a = tmp;
}

#endif /* PL0_MACROS_H */
