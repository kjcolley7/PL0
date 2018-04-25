//
//  genllvm.c
//  PL/0
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#include "genllvm.h"

static inline LLVMTypeRef getWordTy(void);


Destroyer(GenLLVM) {
	//TODO
}
DEF(GenLLVM);


static inline LLVMTypeRef getWordTy(void) {
	/* Ensure Word is a 16-bit integer */
	assert(sizeof(Word) == sizeof(int16_t));
	
	/* Ensure Word is signed */
	assert((Word)(-1) < 0);
	
	/* Word is an i16 */
	return LLVMInt16Type();
}


GenLLVM* GenLLVM_initWithAST(GenLLVM* self, AST_Block* prog) {
	if((self = GenLLVM_init(self))) {
		
	}
	
	return self;
}

