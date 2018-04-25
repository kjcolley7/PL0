//
//  pl0c.h
//  PL/0
//
//  Created by Kevin Colley on 11/30/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_PL0C_H
#define PL0_PL0C_H

typedef struct CompilerFiles CompilerFiles;

#include "object.h"
#include "compiler/parser/parser.h"
#include "compiler/codegen/codegen.h"

struct CompilerFiles {
	OBJECT_BASE;
	
	FILE* tokenlist;
	FILE* symtab;
	FILE* mcode;
	FILE* ast;
#if DEBUG
	FILE* unoptimized_cfg;
#endif
	FILE* cfg;
};
DECL(CompilerFiles);


/*! Run the lexer and compiler together to produce the PM/0 machine code
 @param files Open file streams used by the compiler
 @param parserType Which parser to use
 @param codegenType Which code generation engine to use
 @return Zero on success, nonzero on error
 */
int run_compiler(CompilerFiles* files, PARSER_TYPE parserType, CODEGEN_TYPE codegenType);


#endif /* PL0_PL0C_H */
