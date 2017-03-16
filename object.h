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
