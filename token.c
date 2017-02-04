//
//  token.c
//  PL/0
//
//  Created by Kevin Colley on 10/13/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "token.h"
#include <stdlib.h>
#include <string.h>


Destroyer(Token) {
	destroy(&self->lexeme);
}
DEF(Token);

Token* Token_initWithType(Token* self, token_type type, const char* lexeme, size_t line_number) {
	if((self = Token_init(self))) {
		self->type = type;
		self->lexeme = strdup_ff(lexeme);
		self->line_number = line_number;
	}
	
	return self;
}
