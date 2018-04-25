//
//  genllvm.h
//  PL/0
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#ifndef PL0_GENLLVM_H
#define PL0_GENLLVM_H

#include <llvm-c/Core.h>

typedef struct GenLLVM GenLLVM;

#include "object.h"
#include "compiler/ast_nodes.h"


struct GenLLVM {
	OBJECT_BASE;
	
	/*! LLVM globals context */
	LLVMContextRef context;
	
	/*! LLVM module (holds all functions) */
	LLVMModuleRef module;
};
DECL(GenLLVM);


/*! Initialize the LLVM code generator using the program's full AST
 @param prog AST for the top-level block
 */
GenLLVM* GenLLVM_initWithAST(GenLLVM* self, AST_Block* prog);

/*! Draw a code flow graph and write the Graphviz code to a file
 @param fp Output file where the Graphviz code should be written
 */
void GenLLVM_drawGraph(GenPM0* self, FILE* fp);

/*! Write the program's symbol table to a file
 @param fp Output file where the symbol table should be written
 */
void GenLLVM_writeSymbolTable(GenPM0* self, FILE* fp);

/*! Emits the instructions for the program to the specified file stream
 @param fp Output file stream where instructions should be emitted
 */
void GenLLVM_emit(GenLLVM* self, FILE* fp);


#endif /* PL0_GENLLVM_H */
