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

typedef enum CODEGEN_TYPE {
	CODEGEN_UNINITIALIZED = 0,
	CODEGEN_PM0,
	
#if WITH_LLVM
	CODEGEN_LLVM,
#endif /* WITH_LLVM */
} CODEGEN_TYPE;

#include "object.h"
#include "compiler/ast_nodes.h"
#include "graphviz.h"
#include "pm0/genpm0.h"

#if WITH_LLVM
#include "llvm/genllvm.h"
#endif /* WITH_LLVM */

struct Codegen {
	OBJECT_BASE;
	
	/*! Codegen emitter type (PM/0 vs LLVM) */
	CODEGEN_TYPE cgType;
	
	/*! The specific codegen emitter */
	union {
		GenPM0* pm0;    /*!< PM/0 code generator */
		
#if WITH_LLVM
		GenLLVM* llvm;  /*!< LLVM code generator */
#endif
	} cg;
};
DECL(Codegen);


/*! Initialize a codegen object from the program's AST
 @param prog AST that makes up the entire program
 @param cgType Codegen emitter to use
 */
Codegen* Codegen_initWithAST(Codegen* self, AST_Block* prog, CODEGEN_TYPE cgType);

/*! Draw procedure CFGs to fp */
void Codegen_drawGraph(Codegen* self, FILE* fp);

/*! Output the program's symbol table according to the assignment description
 @param fp Output file stream to write the symbol table to
 */
void Codegen_writeSymbolTable(Codegen* self, FILE* fp);

/*! Emits the instructions for the program to the specified file stream
 @param fp Output file stream where instructions should be emitted
 */
void Codegen_emit(Codegen* self, FILE* fp);


#endif /* PL0_CODEGEN_H */
