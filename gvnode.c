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
	
	size_t i;
	for(i = 0; i < self->attr_count; i++) {
		destroy(&self->attr_names[i]);
		destroy(&self->attr_values[i]);
	}
	destroy(&self->attr_names);
	destroy(&self->attr_values);
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
	if(self->attr_count == self->attr_cap) {
		size_t old_cap = self->attr_cap;
		expand(&self->attr_names, &old_cap);
		expand(&self->attr_values, &self->attr_cap);
	}
	
	self->attr_names[self->attr_count] = html_str(attrib);
	vasprintf_ff(&self->attr_values[self->attr_count++], value, ap);
}

void GVNode_draw(GVNode* self, Graphviz* gv) {
	if(self->attr_count == 0) {
		/* Is this necessary? */
		Graphviz_draw(gv, "<%s>;", self->node_id);
		return;
	}
	
	char* attrs = NULL;
	size_t i;
	for(i = 0; i < self->attr_count; i++) {
		const char* prefix = "";
		if(attrs != NULL) {
			prefix = ", ";
		}
		
		char* next;
		asprintf_ff(&next, "%s%s%s=%s", attrs ?: "", prefix, self->attr_names[i], self->attr_values[i]);
		destroy(&attrs);
		attrs = next;
	}
	
	Graphviz_draw(gv, "<%s> [%s];", self->node_id, attrs);
	destroy(&attrs);
}
