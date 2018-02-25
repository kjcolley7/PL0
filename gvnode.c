//
//  gvnode.c
//  PL/0
//
//  Created by Kevin Colley on 11/23/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "gvnode.h"


Destroyer(GVNode) {
	destroy(&self->node_id);
	
	foreach(&self->attrs, attr) {
		destroy(&attr->name);
		destroy(&attr->value);
	}
	clear_array(&self->attrs);
}
DEF(GVNode);

GVNode* GVNode_initWithIdentifier(GVNode* self, const char* identifier, ...) {
	VARIADIC(identifier, ap, {
		return GVNode_vInitWithIdentifier(self, identifier, ap);
	});
}
GVNode* GVNode_vInitWithIdentifier(GVNode* self, const char* identifier, va_list ap) {
	if((self = GVNode_init(self))) {
		char* escaped = html_str(identifier);
		vasprintf_ff(&self->node_id, escaped, ap);
		destroy(&escaped);
	}
	
	return self;
}

void GVNode_setLabel(GVNode* self, const char* label, ...) {
	VARIADIC(label, ap, {
		return GVNode_vSetLabel(self, label, ap);
	});
}
void GVNode_vSetLabel(GVNode* self, const char* label, va_list ap) {
	GVNode_vAddAttribute(self, "label", label, ap);
}

void GVNode_addAttribute(GVNode* self, const char* attrib, const char* value, ...) {
	VARIADIC(value, ap, {
		return GVNode_vAddAttribute(self, attrib, value, ap);
	});
}
void GVNode_vAddAttribute(GVNode* self, const char* attrib, const char* value, va_list ap) {
	element_type(self->attrs) attr;
	attr.name = html_str(attrib);
	attr.value = vrsprintf_ff(value, ap);
	append(&self->attrs, attr);
}

void GVNode_draw(GVNode* self, Graphviz* gv) {
	if(self->attrs.count == 0) {
		/* Is this necessary? */
		Graphviz_draw(gv, "<%s>;", self->node_id);
		return;
	}
	
	char* attrs = NULL;
	foreach(&self->attrs, attr) {
		const char* prefix = "";
		if(attrs != NULL) {
			prefix = ", ";
		}
		
		char* next = rsprintf_ff("%s%s%s=%s", attrs ?: "", prefix, attr->name, attr->value);
		destroy(&attrs);
		attrs = next;
	}
	
	Graphviz_draw(gv, "<%s> [%s];", self->node_id, attrs);
	destroy(&attrs);
}
