//
//  codegen.h
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_CODEGEN_H
#define PL0_CODEGEN_H

typedef struct Codegen Codegen;

#include "object.h"
#include "compiler/ast_nodes.h"
#include "block.h"
#include "graphviz.h"

struct Codegen {
	OBJECT_BASE;
	
	/*! The top-level code block of the program where execution begins */
	Block* block;
};
DECL(Codegen);


/*! Initialize a codegen object from the program's AST
 @param prog AST that makes up the entire program
 */
Codegen* Codegen_initWithAST(Codegen* self, AST_Program* prog);

/*! Sets addresses and resolves symbols */
void Codegen_layoutCode(Codegen* self);

/*! Performs basic optimizations on the code graphs */
void Codegen_optimize(Codegen* self);

/*! Draw procedure CFGs to fp */
void Codegen_drawGraph(Codegen* self, FILE* fp);

/*! Output the program's symbol table according to the assignment description
 @param fp Output file stream to write the symbol table to
 */
void Codegen_writeSymbolTable(Codegen* self, FILE* fp);

/*! Emits the instructions for the program to the specified file stream
 @param fp Output file stream to emit instructions to
 */
void Codegen_emit(Codegen* self, FILE* fp);

/*! Generate code for a block given it's AST
 @param scope Block scope for the current procedure
 @param ast AST for the block used to produce code
 @return True on success, or false on error
 */
bool Codegen_genBlock(Block* scope, AST_Block* ast);


#endif /* PL0_CODEGEN_H */
