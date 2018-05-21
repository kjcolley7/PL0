//
//  object.h
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_OBJECT_H
#define PL0_OBJECT_H

#include "macros.h"
#include "dynamic_array.h"
#include "dynamic_string.h"

/*! Adds the struct element for the object base */
#define OBJECT_BASE ObjBase _base

#define DECL_ALLOC(type) type* type##_alloc(void)
#define DECL_INIT(type)  type* type##_init(type* self)
#define DECL_NEW(type)   type* type##_new(void)

/*! Declares the allocator and plain initializer functions for the specified type */
#define DECL(type) \
DECL_ALLOC(type); \
DECL_INIT(type); \
DECL_NEW(type)

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
	size_t refcnt;
	
	/*! Object destructor (doesn't call free()) */
	void (*dtor)(void*);
} ObjBase;

/*! Initializes the object base */
#define ObjBase_init(obj, type) \
(type*)_ObjBase_init(&(obj)->_base, sizeof(type), (void (*)(void*))&type##_destroy)
static inline void* _ObjBase_init(ObjBase* self, size_t size, void (*dtor)(void*)) {
	if(self) {
		/* Zero-initialize the object's storage */
		memset(self, 0, size);
		
		/* 2 because the low bit is used to determine if the object shouldn't be freed */
		self->refcnt = 2;
		self->dtor = dtor;
	}
	
	return self;
}

/*! Used after initialization to specify that an object should never be freed,
 * such as when it is a stack allocated variable rather than dynamically allocated
 */
#define localize(obj) (__typeof__(obj))_localize(&(obj)->_base)
static inline void* _localize(ObjBase* base) {
	base->refcnt |= 1;
	return base;
}

/*! Increment the reference count of an object and return the object */
#define retain(obj) (__typeof__(obj))_retain(&(obj)->_base)
static inline void* _retain(ObjBase* base) {
	/*
	 * If incrementing the reference count would cause it to overflow, saturate the
	 * reference count instead. This will intentionally leak the memory, which is a
	 * better outcome than a use-after-free (UAF) security vulnerability.
	 */
	if((base->refcnt | 1) == SIZE_MAX) {
		return base;
	}
	
	/* Increment the reference count */
	base->refcnt += 2;
	return base;
}

/*! Release a reference to an object, destroying the object when its reference count reaches zero
 * @note This sets the pointed to variable to NULL to help avoid simple use after free bugs
 */
#define release(pobj) do { \
	__typeof__(pobj) _pobj = (pobj); \
	if(!_pobj) { \
		break; \
	} \
	release_local(*_pobj); \
	*_pobj = NULL; \
} while(0)

/*! Release a reference to an object, destroying the object when its reference count reaches zero */
#define release_local(obj) do { \
	__typeof__(obj) _obj = (obj); \
	if(!_obj) { \
		break; \
	} \
	_release(&_obj->_base); \
} while(0)
static inline void _release(ObjBase* base) {
	/* This shouldn't ever happen */
	ASSERT(base->refcnt >= 2);
	
	/* This object's reference count has been saturated, so it will never be freed */
	if((base->refcnt | 1) == SIZE_MAX) {
		return;
	}
	
	/* Decrement the reference count */
	base->refcnt -= 2;
	
	/* Destroy object when there are no more references to it */
	if((base->refcnt & ~1) == 0) {
		/* Call the destroy method */
		base->dtor(base);
		
		if((base->refcnt & 1) == 0) {
			/* Reference count's low bit was unset, so the object is heap-allocated */
			destroy(&base);
		}
	}
}

/*! Helper macro to release all elements in a dynamic array and then destroy the array */
#define array_release(parr) do { \
	__typeof__(parr) _array_release_parr = (parr); \
	foreach(_array_release_parr, _array_release_pcur) { \
		release(_array_release_pcur); \
	} \
	array_clear(_array_release_parr); \
} while(0)


#endif /* PL0_OBJECT_H */
