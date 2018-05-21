//
//  graphviz.h
//  PL/0
//
//  Created by Kevin Colley on 11/3/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_GRAPHVIZ_H
#define PL0_GRAPHVIZ_H

#include <stdarg.h>
#include <stdbool.h>

typedef struct Graphviz Graphviz;

#include "object.h"

struct Graphviz {
	OBJECT_BASE;
	
	/*! True if this is a subgraph of another graph */
	bool subgraph;
	
	union {
		FILE* fout;   /*!< Output file stream where the DOT data will be written */
		Graphviz* gv; /*!< Parent graph */
	} output;
};
DECL(Graphviz);


/*! Initializes the graphviz object to write to the specified file and names the output graph
 @param fout Output file stream where the DOT data will be written
 @param name Format string for the name of the main graph to output
 */
Graphviz* Graphviz_initWithFile(Graphviz* self, FILE* fout, const char* name, ...);
Graphviz* Graphviz_vInitWithFile(Graphviz* self, FILE* fout, const char* name, va_list ap);

/*! Initializes the graphviz object to write a subgraph within the specified writer
 @param parent Parent graph writer this graph is a subgraph of
 @param name Format string for the name of the subgraph to output
 */
Graphviz* Graphviz_initWithParentGraph(Graphviz* self, Graphviz* parent, const char* name, ...);
Graphviz* Graphviz_vInitWithParentGraph(Graphviz* self, Graphviz* parent, const char* name, va_list ap);

/*! Prints raw Graphviz DOT code to the output file */
void Graphviz_printf(Graphviz* self, const char* fmt, ...);
void Graphviz_vprintf(Graphviz* self, const char* fmt, va_list ap);

/*! Prints the indentation for the current graph level */
void Graphviz_indent(Graphviz* self);

/*! Draws a line of raw Graphviz DOT code to the output file
 @param fmt Formatted string for DOT code
 */
void Graphviz_draw(Graphviz* self, const char* fmt, ...);
void Graphviz_vDraw(Graphviz* self, const char* fmt, va_list ap);

/*! Draws a node with the specified id and formatted label
 @param id Node identifier
 @param fmt Label format string
 */
void Graphviz_drawNode(Graphviz* self, const char* id, const char* fmt, ...);
void Graphviz_vDrawNode(Graphviz* self, const char* id, const char* fmt, va_list ap);

/*! Draws a node using the object pointer as its node id, and with the given formatted label
 @param obj Object pointer to use as the node identifier
 @param fmt Label format string
 */
void Graphviz_drawPtrNode(Graphviz* self, void* obj, const char* fmt, ...);
void Graphviz_vDrawPtrNode(Graphviz* self, void* obj, const char* fmt, va_list ap);

/*! Draws an edge between two nodes referenced by their ids
 @param from Node identifier of the source node
 @param to Node identifier of the destination node
 */
void Graphviz_drawEdge(Graphviz* self, const char* from, const char* to);

/*! Draws an edge between two nodes using object pointer node ids
 @param from Object pointer used as the source node id
 @param to Object pointer used as the destination node id
 */
void Graphviz_drawPtrEdge(Graphviz* self, void* from, void* to);


/* Helpful functions */

/*! Convert a character into an HTML-escaped string */
static inline char* html_char(char c) {
	switch(c) {
		case '<': return strdup_ff("&lt;");
		case '>': return strdup_ff("&gt;");
		case '&': return strdup_ff("&amp;");
		case '"': return strdup_ff("&quot;");
		default: return strdup_ff(((const char[2]){c, '\0'}));
	}
}

/* Convert a string into an HTML-escaped string */
static inline char* html_str(const char* s) {
	if(s == NULL) {
		return NULL;
	}
	
	dynamic_string str = {};
	while(*s != '\0') {
		string_append(&str, html_char(*s++));
	}
	
	return string_cstr(&str);
}


#endif /* PL0_GRAPHVIZ_H */
