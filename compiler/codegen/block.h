//
//  block.h
//  PL/0
//
//  Created by Kevin Colley on 11/14/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_BLOCK_H
#define PL0_BLOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Block Block;

#include "object.h"
#include "config.h"
#include "basicblock.h"
#include "compiler/ast_nodes.h"
#include "symtree.h"
#include "graphviz.h"

struct Block {
	OBJECT_BASE;
	
	/*! Symbol table for this block */
	SymTree* symtree;
	
	/*! First basic block in the code graph for this procedure */
	BasicBlock* code;
	
	/*! Total number of instructions in the code graph including jumps and returns */
	Word code_length;
	
	/*! Next block in the linked list */
	Block* next;
	
	/*! Last block in the list */
	Block* last;
	
	/*! Whether this block has been optimized or not */
	bool optimized;
};
DECL(Block);


/*! Initialize a block object with its scope
 @param symtree SymTree holding the scope of this block
 */
Block* Block_initWithScope(Block* self, SymTree* symtree);

/*! Generate the code graph from the AST of a block
 @param ast Abstract syntax tree representation of the code comprising this block
 */
bool Block_generate(Block* self, AST_Block* ast);

/*! Performs basic optimizations on the block's code graph by removing empty basic blocks */
void Block_optimize(Block* self);

/*! Used to determine whether a block contains any code
 @return True if the block has no code instructions
 */
bool Block_isEmpty(Block* self);

/*! Gets the code address of the block
 @return Address of the block in the code segment
 */
Word Block_getAddress(Block* self);

/*! Sets the code address of the block
 @param addr Code address of the block
 */
void Block_setAddress(Block* self, Word addr);

/*! Append the given block to the end of the list, thereby giving it an address
 @param last Block to add to the end of the list of blocks
 */
void Block_append(Block* self, Block* last);

/*! Emits this block's machine code to the specified file
 @param fp File stream to output this function's machine code to
 */
void Block_emit(Block* self, FILE* fp);

/*! Draws the block's code graph */
void Block_drawGraph(Block* self, Graphviz* gv);


#endif /* PL0_BLOCK_H */
