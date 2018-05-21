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
	array_clear(&self->attrs);
}
DEF(GVNode);

GVNode* GVNode_initWithIdentifier(GVNode* self, const char* identifier, ...) {
	VARIADIC(identifier, ap, {
		return GVNode_vInitWithIdentifier(self, identifier, ap);
	});
}
GVNode* GVNode_vInitWithIdentifier(GVNode* self, const char* identifier, va_list ap) {
	if((self = GVNode_init(self))) {
		char* unescaped = vrsprintf_ff(identifier, ap);
		self->node_id = html_str(unescaped);
		destroy(&unescaped);
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
	array_append(&self->attrs, attr);
}

void GVNode_draw(GVNode* self, Graphviz* gv) {
	if(self->attrs.count == 0) {
		/* Is this necessary? */
		Graphviz_draw(gv, "<%s>;", self->node_id);
		return;
	}
	
	dynamic_string attrs = {};
	foreach(&self->attrs, attr) {
		if(!string_empty(&attrs)) {
			string_append(&attrs, ", ");
		}
		
		string_append(&attrs, attr->name);
		string_appendChar(&attrs, '=');
		string_append(&attrs, attr->value);
	}
	
	Graphviz_draw(gv, "<%s> [%s];", self->node_id, string_cstr(&attrs));
	string_clear(&attrs);
}
