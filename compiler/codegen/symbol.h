//
//  symbol.h
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_SYMBOL_H
#define PL0_SYMBOL_H

#include <stdint.h>
#include <stdio.h>

#if WITH_LLVM
#include <llvm-c/Core.h>
#endif /* WITH_LLVM */

typedef struct Symbol Symbol;
typedef enum SYM_TYPE SYM_TYPE;

#include "object.h"
#include "compiler/codegen/pm0/block.h"
#include "config.h"

enum SYM_TYPE {
	SYM_CONST = 1,
	SYM_VAR,
	SYM_PROC
};

struct Symbol {
	OBJECT_BASE;
	
	/*! The type of variable this symbol represents */
	SYM_TYPE type;
	
	/*! Name of the symbol */
	char* name;
	
	/*! Lexical level of the symbol, with 0 being the top level */
	uint16_t level;
	
	/*! The value this symbol holds */
	union {
		Word number;            /*!< The numeric value of a constant */
		uint32_t frame_offset;  /*!< The local stack frame offset of the variable */
		struct {
			size_t param_count; /*!< Number of parameters the procedure takes */
			Block* body;        /*!< Code graph for the procedure */
		} procedure;            /*!< The block object of the procedure */
	} value;
	
#if WITH_LLVM
	/*! The LLVM value of this symbol */
	LLVMValueRef llvm;
#endif /* WITH_LLVM */
};
DECL(Symbol);


/*! Writes the symbol to the output file in the format specified by the assignment description
 @param fp File stream to write the output line to
 */
void Symbol_write(Symbol* sym, FILE* fp);

#endif /* PL0_SYMBOL_H */
