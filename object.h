//
//  object.h
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_OBJECT_H
#define PL0_OBJECT_H

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
#define asprintf_ff(pstr, ...) do { \
	if(asprintf(pstr, __VA_ARGS__) == -1) { \
		perror("asprintf"); \
		abort(); \
	} \
} while(0)

#define vasprintf_ff(pstr, fmt, ap) do { \
	if(vasprintf(pstr, fmt, ap) == -1) { \
		perror("vasprintf"); \
		abort(); \
	} \
} while(0)

/*! Convert a pointer to a string */
static inline char* ptos(void* ptr) {
	char* ret;
	asprintf_ff(&ret, "%p", ptr);
	return ret;
}

/*! Helper macro for expanding a dynamic array. This will zero-initialize the extra
 allocated space and will abort if the requested allocation size cannot be allocated.
 */
#define expand(parr, pcount) \
xpand_array((void**)(parr), sizeof(**(parr)), pcount, sizeof(*(pcount)))
static inline void xpand_array(void** parr, size_t elem_size, void* pcount, size_t count_sz) {
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
static inline void insert_index(void* arr, size_t elem_size,
	size_t index, const void* elem, size_t count) {
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

/*! Concatenate two strings by doing a += b */
static inline void astrcat(char** a, const char* b) {
	char* tmp;
	asprintf_ff(&tmp, "%s%s", *a ?: "", b ?: "");
	destroy(a);
	*a = tmp;
}

/*! Number of elements in an array */
#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof(arr[0]))

/*! Adds the struct element for the object base */
#define OBJECT_BASE ObjBase _base

#define DECL_ALLOC(type) type* type##_alloc(void)
#define DECL_INIT(type)  type* type##_init(type* self)
#define DECL_NEW(type)   type* type##_new(void)

/*! Declares the allocator and plain initializer functions for the specified type */
#define DECL(type) \
DECL_ALLOC(type); \
DECL_INIT(type); \
DECL_NEW(type);

#define DEF_ALLOC(type) \
type* type##_alloc(void) { \
	return malloc_ff(sizeof(type)); \
} \
DECL_ALLOC(type)

#define DEF_INIT(type) \
type* type##_init(type* self) { \
	return ObjBase_init(self, type); \
} \
DECL_INIT(type)

#define DEF_NEW(type) \
type* type##_new(void) { \
	return type##_init(type##_alloc()); \
} \
DECL_NEW(type)

/*! Defines the allocator and plain initializer functions for the specified type */
#define DEF(type) \
DEF_ALLOC(type); \
DEF_INIT(type); \
DEF_NEW(type)

/*! Defines the allocator and custom initializer functions for the specified type */
#define DEF_CUSTOM(type) \
DEF_ALLOC(type); \
DEF_NEW(type); \
static type* _##type##_init(type* self); \
type* type##_init(type* self) { \
	if((self = ObjBase_init(self, type))) { \
		self = _##type##_init(self); \
	} \
	return self; \
} \
static type* _##type##_init(type* self)

/*! Define the destroyer function for objects of the specified type */
#define Destroyer(type) \
static void type##_destroy(__unused type* self)

/*! Base structure used for reference counted objects */
typedef struct ObjBase {
	/*! Reference count of the object */
	ssize_t refcnt;
	
	/*! Object destructor (doesn't call free()) */
	void (*dtor)(void*);
} ObjBase;

/*! Initializes the object base */
#define ObjBase_init(obj, type) \
(type*)_ObjBase_init(&obj->_base, sizeof(type), (void (*)(void*))&type##_destroy)
static inline void* _ObjBase_init(ObjBase* self, size_t size, void (*dtor)(void*)) {
	if(self) {
		memset(self, 0, size);
		self->refcnt = 1;
		self->dtor = dtor;
	}
	
	return self;
}

/*! Used after initialization to specify that an object is local and not dynamically allocated */
#define localize(obj) (__typeof__(obj))_localize(&(obj)->_base)
static inline void* _localize(ObjBase* base) {
	if(base->refcnt > 0) {
		base->refcnt = -base->refcnt;
	}
	
	return base;
}

/*! Increment the reference count of an object and return the object */
#define retain(obj) (__typeof__(obj))_retain(&(obj)->_base)
static inline void* _retain(ObjBase* base) {
	int dir = base->refcnt > 0 ? 1 : -1;
	base->refcnt += dir;
	return base;
}

/*! Release a reference to an object (and destroy it when reference count reaches zero)
 @note This sets the pointed to variable to NULL to prevent use after free bugs
 */
#define release(pobj) do { \
	__typeof__(pobj) _pobj = (pobj); \
	if(!_pobj || !*_pobj) { \
		break; \
	} \
	_release(&(*_pobj)->_base); \
	*_pobj = NULL; \
} while(0)
static inline void _release(ObjBase* base) {
	/* This shouldn't ever happen */
	assert(base->refcnt != 0);
	
	/* Stack-based objects have negative reference counts */
	int dir = base->refcnt > 0 ? 1 : -1;
	base->refcnt -= dir;
	
	/* Destroy object when there are no more references to it */
	if(base->refcnt == 0) {
		/* Call the destroy method */
		base->dtor(base);
		
		if(dir == 1) {
			/* Object had a positive reference count, so it was heap-allocated */
			destroy(&base);
		}
	}
}


#endif /* PL0_OBJECT_H */
