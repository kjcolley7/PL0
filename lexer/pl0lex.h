//
//  pl0lex.h
//  PL/0
//
//  Created by Kevin Colley on 11/30/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_PL0LEX_H
#define PL0_PL0LEX_H

typedef struct LexerFiles LexerFiles;

#include "object.h"
#include "lexer.h"

struct LexerFiles {
	OBJECT_BASE;
	
	FILE* input;
	FILE* table;
	FILE* clean;
	FILE* tokenlist;
	FILE* graph;
};
DECL(LexerFiles);


/*! Runs the lexer on the PL/0 input file
 @param files Open file streams used by the lexer
 */
int run_lexer(LexerFiles* files);

#endif /* PL0_PL0LEX_H */
