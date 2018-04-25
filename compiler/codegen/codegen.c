//
//  codegen.c
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "codegen.h"
#include <stdarg.h>
#include <assert.h>


Destroyer(Codegen) {
	switch(self->cgType) {
		case CODEGEN_PM0:
			release(&self->cg.pm0);
			break;
			
#if WITH_LLVM
		case CODEGEN_LLVM:
			release(&self->cg.llvm);
			break;
#endif /* WITH_LLVM */
			
		case CODEGEN_UNINITIALIZED:
			break;
			
		default:
			assert(!"Invalid codegen type");
	}
}
DEF(Codegen);

Codegen* Codegen_initWithAST(Codegen* self, AST_Block* prog, CODEGEN_TYPE cgType) {
	if((self = Codegen_init(self))) {
		switch(cgType) {
			case CODEGEN_PM0:
				self->cg.pm0 = GenPM0_initWithAST(GenPM0_alloc(), prog);
				if(self->cg.pm0 == NULL) {
					release(&self);
					return NULL;
				}
				break;
				
#if WITH_LLVM
			case CODEGEN_LLVM:
				self->cg.llvm = GenLLVM_initWithAST(GenLLVM_alloc(), prog);
				if(self->cg.llvm == NULL) {
					release(&self);
					return NULL;
				}
				break;
#endif /* WITH_LLVM */
				
			default:
				assert(!"Unknown codegen type");
		}
		
		self->cgType = cgType;
	}
	
	return self;
}

void Codegen_drawGraph(Codegen* self, FILE* fp) {
	switch(self->cgType) {
		case CODEGEN_PM0:
			return GenPM0_drawGraph(self->cg.pm0, fp);
			
#if WITH_LLVM
		case CODEGEN_LLVM:
			return GenLLVM_drawGraph(self->cg.llvm, fp);
#endif /* WITH_LLVM */
			
		default:
			assert(!"Unknown codegen type");
	}
}

void Codegen_writeSymbolTable(Codegen* self, FILE* fp) {
	switch(self->cgType) {
		case CODEGEN_PM0:
			return GenPM0_writeSymbolTable(self->cg.pm0, fp);
			
#if WITH_LLVM
		case CODEGEN_LLVM:
			return GenLLVM_writeSymbolTable(self->cg.llvm, fp);
#endif /* WITH_LLVM */
			
		default:
			assert(!"Unknown codegen type");
	}
}

void Codegen_emit(Codegen* self, FILE* fp) {
	switch(self->cgType) {
		case CODEGEN_PM0:
			return GenPM0_emit(self->cg.pm0, fp);
			
#if WITH_LLVM
		case CODEGEN_LLVM:
			return GenLLVM_emit(self->cg.llvm, fp);
#endif /* WITH_LLVM */
			
		default:
			assert(!"Unknown codegen type");
	}
}
