//
//  basicblock.h
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_BASICBLOCK_H
#define PL0_BASICBLOCK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct BasicBlock BasicBlock;

#include "object.h"
#include "config.h"
#include "instruction.h"
#include "symbol.h"
#include "block.h"
#include "graphviz.h"

struct BasicBlock {
	OBJECT_BASE;
	
	/*! Code address of the basic block */
	Word code_addr;
	
	/*! Number of instructions in this basic block */
	Word insn_count;
	
	/*! Allocated capacity of instructions */
	Word insn_cap;
	
	/*! Array of instructions */
	Insn* insns;
	
	/*! Whether the basic block ends in a conditional branch */
	bool conditional;
	
	/*! Index of the first instruction of the condition code, or -1 if there is no condition */
	Word cond_index;
	
	/*! Target of the branch from this basic block when the condition is true (or unconditional) */
	BasicBlock* target;
	
	/*! Target of the branch from this basic block when the condition is false (may be null) */
	BasicBlock* ztarget;
	
	/*! Basic block that directly follows this one in code */
	BasicBlock* next;
	
	/*! Basic block that precedes this one in code */
	BasicBlock* prev;
	
	/*! Number of symbols referenced by this basic block */
	size_t sym_count;
	
	/*! Allocated capacity for symbols */
	size_t sym_cap;
	
	/*! Array of symbols referenced by this basic block */
	Symbol** syms;
	
	/*! Indices of the instructions which reference these symbols */
	Word* sym_indices;
	
	/*! Number of cross references to this basic block */
	size_t xref_count;
	
	/*! Allocated capacity for cross references */
	size_t xref_cap;
	
	/*! Array of cross references to this basic block */
	BasicBlock** xrefs;
	
	/*! Index where the control flow code begins, or -1 if the tail doesn't exist */
	Word tail_index;
	
	/*! Whether the condition instruction should be inverted */
	bool invert_cond;
};
DECL(BasicBlock);


/*! Create and return a basic block that follows this one linearly, and update the pointer
 @return A newly created basic block that follows this one linearly in the code segment
 */
BasicBlock* BasicBlock_createNext(BasicBlock** bb);

/*! Sets the start of the condition code for this basic block to the current position */
void BasicBlock_markCondition(BasicBlock* self);

/*! Adds a symbol entry to the basic block
 @param sym Symbol that must be resolved later
 */
void BasicBlock_markSymbol(BasicBlock* self, Symbol* sym);

/*! Used to determine whether a basic block is empty of code
 @return True if the basic block contains no code instructions
 */
bool BasicBlock_isEmpty(BasicBlock* self);


/*! Used to determine whether a basic block that starts a procedure is empty
 @return True if the basic block is empty or contains only an INC instruction
 */
bool BasicBlock_isEmptyProc(BasicBlock* self);

/*! Set the default target of this basic block
 @param target Basic block this one should execute when the condition is true or unconditional
 */
void BasicBlock_setTarget(BasicBlock* self, BasicBlock* target);

/*! Sets the false target of this basic block
 @param false_target Basic block this one should execute when the condition is false
 @note This will make the basic block conditional if it isn't already
 */
void BasicBlock_setFalseTarget(BasicBlock* self, BasicBlock* false_target);

/*! Adds a reference to this basic block
 @param from Basic block that references this one
 */
void BasicBlock_addXref(BasicBlock* self, BasicBlock* from);

/*! Removes a cross reference to this block
 @param from Basic block which used to reference this one but no longer does
 */
void BasicBlock_removeXref(BasicBlock* self, BasicBlock* from);

/*! Create and append the instruction to the basic block
 @param insn Instruction to append to the basic block
 */
void BasicBlock_addInsn(BasicBlock* self, Insn insn);

/*! Gets the code address of the current basic block
 @return Code address of this basic block
 */
Word BasicBlock_getAddress(BasicBlock* self);

/*! Sets the code address of the current basic block
 @param addr Code address of the basic block
 */
void BasicBlock_setAddress(BasicBlock* self, Word addr);

/*! Resolves undefined symbols in this basic block
 @param scope Scope of the code this basic block was generated from
 */
void BasicBlock_resolve(BasicBlock* self, Block* scope);

/*! Performs optimizations on a basic block */
void BasicBlock_optimize(BasicBlock* self);

/*! Emits the instructions in this basic block to the output file stream specified
 @param fp File stream to write PM/0 machine code to
 @param level Lexical level that this code belongs to
 */
void BasicBlock_emit(BasicBlock* self, FILE* fp, uint16_t level);

/*! Count the number of instructions in this basic block including trailing jumps */
Word BasicBlock_getInstructionCount(BasicBlock* self);

/*! Draws the basic block as a node */
void BasicBlock_drawGraph(BasicBlock* self, Graphviz* gv, uint16_t level);


#endif /* PL0_BASICBLOCK_H */
