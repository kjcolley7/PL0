//
//  symbol.c
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "symbol.h"


static const char* Symbol_getType(Symbol* self);
static Word Symbol_getValue(Symbol* self);


Destroyer(Symbol) {
	destroy(&self->name);
}
DEF(Symbol);

void Symbol_write(Symbol* self, FILE* fp) {
	fprintf(fp, "%s\t%s\t%d\t%d\n", self->name, Symbol_getType(self), self->level, Symbol_getValue(self));
}

static const char* Symbol_getType(Symbol* self) {
	switch(self->type) {
		case SYM_CONST: return "const";
		case SYM_VAR:   return "var";
		case SYM_PROC:  return "proc";
		
		default:
			ASSERT(!"Unknown symbol type");
	}
}

static Word Symbol_getValue(Symbol* self) {
	switch(self->type) {
		case SYM_CONST:
			return self->value.number;
			
		case SYM_VAR:
			return self->value.frame_offset;
			
		case SYM_PROC:
			return Block_getAddress(self->value.procedure.body);
		
		default:
			ASSERT(!"Unknown symbol type");
	}
}
