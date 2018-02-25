//
//  graphviz.c
//  PL/0
//
//  Created by Kevin Colley on 11/3/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "graphviz.h"


Destroyer(Graphviz) {
	if(self->subgraph) {
		Graphviz_draw(self->output.gv, "}");
	}
	else {
		Graphviz_printf(self, "}");
	}
}
DEF(Graphviz);

Graphviz* Graphviz_initWithFile(Graphviz* self, FILE* fout, const char* name, ...) {
	VARIADIC(name, ap, {
		return Graphviz_vInitWithFile(self, fout, name, ap);
	});
}
Graphviz* Graphviz_vInitWithFile(Graphviz* self, FILE* fout, const char* name, va_list ap) {
	if((self = Graphviz_init(self))) {
		self->output.fout = fout;
		char* graphname = vrsprintf_ff(name, ap);
		Graphviz_printf(self, "digraph %s {\n", graphname);
		destroy(&graphname);
	}
	
	return self;
}

Graphviz* Graphviz_initWithParentGraph(Graphviz* self, Graphviz* parent, const char* name, ...) {
	VARIADIC(name, ap, {
		return Graphviz_vInitWithParentGraph(self, parent, name, ap);
	});
}
Graphviz* Graphviz_vInitWithParentGraph(Graphviz* self, Graphviz* parent, const char* name, va_list ap) {
	if((self = Graphviz_init(self))) {
		self->subgraph = true;
		self->output.gv = parent;
		char* graphname = vrsprintf_ff(name, ap);
		Graphviz_draw(parent, "subgraph %s {", graphname);
		destroy(&graphname);
	}
	
	return self;
}

void Graphviz_printf(Graphviz* self, const char* fmt, ...) {
	VARIADIC(fmt, ap, {
		Graphviz_vprintf(self, fmt, ap);
	});
}
void Graphviz_vprintf(Graphviz* self, const char* fmt, va_list ap) {
	if(self->subgraph) {
		Graphviz_vprintf(self->output.gv, fmt, ap);
	}
	else {
		vfprintf(self->output.fout, fmt, ap);
	}
}

void Graphviz_indent(Graphviz* self) {
	if(self->subgraph) {
		Graphviz_indent(self->output.gv);
	}
	Graphviz_printf(self, "\t");
}

void Graphviz_draw(Graphviz* self, const char* fmt, ...) {
	VARIADIC(fmt, ap, {
		Graphviz_vDraw(self, fmt, ap);
	});
}
void Graphviz_vDraw(Graphviz* self, const char* fmt, va_list ap) {
	Graphviz_indent(self);
	Graphviz_vprintf(self, fmt, ap);
	Graphviz_printf(self, "\n");
}

void Graphviz_drawNode(Graphviz* self, const char* id, const char* fmt, ...) {
	VARIADIC(fmt, ap, {
		Graphviz_vDrawNode(self, id, fmt, ap);
	});
}
void Graphviz_vDrawNode(Graphviz* self, const char* id, const char* fmt, va_list ap) {
	char* label = vrsprintf_ff(fmt, ap);
	Graphviz_draw(self, "<%s> [label = <%s>];", id, label);
	destroy(&label);
}

void Graphviz_drawPtrNode(Graphviz* self, void* obj, const char* fmt, ...) {
	VARIADIC(fmt, ap, {
		Graphviz_vDrawPtrNode(self, obj, fmt, ap);
	});
}
void Graphviz_vDrawPtrNode(Graphviz* self, void* obj, const char* fmt, va_list ap) {
	char* label = vrsprintf_ff(fmt, ap);
	Graphviz_draw(self, "<%p> [label = <%s>];", obj, label);
	destroy(&label);
}

void Graphviz_drawEdge(Graphviz* self, const char* from, const char* to) {
	Graphviz_draw(self, "<%s> -> <%s>;", from, to);
}

void Graphviz_drawPtrEdge(Graphviz* self, void* from, void* to) {
	Graphviz_draw(self, "<%p> -> <%p>;", from, to);
}
