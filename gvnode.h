//
//  gvnode.h
//  PL/0
//
//  Created by Kevin Colley on 11/23/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_GVNODE_H
#define PL0_GVNODE_H

#include <stdarg.h>

typedef struct GVNode GVNode;

#include "object.h"
#include "graphviz.h"

struct GVNode {
	OBJECT_BASE;
	
	char* node_id;
	
	dynamic_array(struct {
		char* name;
		char* value;
	}) attrs;
};
DECL(GVNode);


GVNode* GVNode_initWithIdentifier(GVNode* self, const char* identifier, ...);
GVNode* GVNode_vInitWithIdentifier(GVNode* self, const char* identifier, va_list ap);

void GVNode_setLabel(GVNode* self, const char* label, ...);
void GVNode_vSetLabel(GVNode* self, const char* label, va_list ap);

void GVNode_addAttribute(GVNode* self, const char* attrib, const char* value, ...);
void GVNode_vAddAttribute(GVNode* self, const char* attrib, const char* value, va_list ap);

void GVNode_draw(GVNode* self, Graphviz* gv);


#endif /* PL0_GVNODE_H */
