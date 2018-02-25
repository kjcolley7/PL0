//
//  genpm0.h
//  PL/0
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#ifndef PL0_GENPM0_H
#define PL0_GENPM0_H

typedef struct GenPM0 GenPM0;

#include "object.h"
#include "block.h"
#include "basicblock.h"
#include "compiler/codegen/symtree.h"
#include "compiler/ast_nodes.h"

struct GenPM0 {
	OBJECT_BASE;
	
	/*! The top-level block which contains the entire program */
	Block* block;
};
DECL(GenPM0);


/*! Initialize the PM/0 code generator using the program's full AST
 @param prog AST for the top-level block
 */
GenPM0* GenPM0_initWithAST(GenPM0* self, AST_Block* prog);

/*! Draw a code flow graph and write the Graphviz code to a file
 @param fp Output file where the Graphviz code should be written
 */
void GenPM0_drawGraph(GenPM0* self, FILE* fp);

/*! Write the program's symbol table to a file
 @param fp Output file where the symbol table should be written
 */
void GenPM0_writeSymbolTable(GenPM0* self, FILE* fp);

/*! Emits the instructions for the program to the specified file stream
 @param fp Output file stream where instructions should be emitted
 */
void GenPM0_emit(GenPM0* self, FILE* fp);

/*! Generate code for the block given its AST node
 @param scope SymTree node for the block being codegenned
 @param code Active basic block where code should be generated
 @param ast AST node for the block to codegen
 @return True on success, false on failure
 */
bool GenPM0_genBlock(SymTree* scope, BasicBlock** code, AST_Block* ast);


#endif /* PL0_GENPM0_H */
