//
//  basicblock.c
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "basicblock.h"
#include "gvnode.h"


static void BasicBlock_invalidateTail(BasicBlock* self);
static void BasicBlock_removeInsn(BasicBlock* self, size_t index);
static void BasicBlock_genTail(BasicBlock* self, uint16_t level);
static void BasicBlock_tailCallOptimize(BasicBlock* self, Block* scope);
static void BasicBlock_appendLine(char** label, const char* line);


Destroyer(BasicBlock) {
	release(&self->next);
	array_clear(&self->insns);
	array_clear(&self->symrefs);
	array_clear(&self->coderefs);
}

DEF_CUSTOM(BasicBlock) {
	self->code_addr = ADDR_UND;
	return self;
}

BasicBlock* BasicBlock_createNext(BasicBlock** bb) {
	/* Make sure not to replace the pointer to an existing basic block */
	ASSERT((*bb)->next == NULL);
	BasicBlock* next = BasicBlock_new();
	next->prev = *bb;
	(*bb)->next = next;
	
	/* Invalidate tail index */
	BasicBlock_invalidateTail(*bb);
	
#if DEBUG
	foreach(&(*bb)->insns, pinsn) {
		ASSERT(pinsn->op != OP_BREAK);
	}
#endif
	
	/* Update pointer and return it */
	return *bb = next;
}

void BasicBlock_markCondition(BasicBlock* self) {
	self->cond_index = (Word)self->insns.count;
}

void BasicBlock_markSymbol(BasicBlock* self, Symbol* sym) {
	element_type(self->symrefs) symref = {
		.sym = sym,
		.index = (Word)self->insns.count
	};
	array_append(&self->symrefs, symref);
}

static void BasicBlock_invalidateTail(BasicBlock* self) {
	/* If this basic block doesn't have a tail, there's nothing to do */
	if(!(self->flags & BB_HAS_TAIL)) {
		return;
	}
	
	/* Clear the inverted condition flag, as that is set by genTail */
	self->flags &= ~BB_INVERT_CONDITION;
	
	/* Keep chopping off the tail one instruction at a time until there's nothing left */
	while(self->insns.count > self->tail_index) {
		BasicBlock_removeInsn(self, self->insns.count - 1);
	}
	
	/* This basic block no longer has a tail */
	self->flags &= ~BB_HAS_TAIL;
}

bool BasicBlock_isEmpty(BasicBlock* self) {
	/* Empty and is NOT the first basic block of a procedure */
	return self == NULL || (self->insns.count == 0 && self->prev != NULL);
}

bool BasicBlock_isEmptyProc(BasicBlock* self) {
	/* Empty procedure if there's at most one INC instruction and no other basic blocks */
	return self == NULL ||
	       (self->prev == NULL && self->next == NULL &&
	        (self->insns.count == 0 ||
	         (self->insns.count == 1 && self->insns.elems[0].op == OP_INC)));
}

void BasicBlock_setTarget(BasicBlock* self, BasicBlock* target) {
	/* Remove the reference to the previous target */
	BasicBlock_removeXref(self->target, self);
	
	/* Let the new target know that we're now referencing it */
	self->target = target;
	BasicBlock_addXref(target, self);
	
	/* Invalidate tail */
	BasicBlock_invalidateTail(self);
}

void BasicBlock_setFalseTarget(BasicBlock* self, BasicBlock* false_target) {
	self->flags |= BB_HAS_CONDITION;
	
	/* Remove the reference to the previous target */
	BasicBlock_removeXref(self->ztarget, self);
	
	/* Let the new target know that we're now referencing it */
	self->ztarget = false_target;
	BasicBlock_addXref(false_target, self);
	
	/* Invalidate tail */
	BasicBlock_invalidateTail(self);
}

void BasicBlock_addXref(BasicBlock* self, BasicBlock* from) {
	if(self == NULL) {
		return;
	}
	
	/* Append xref */
	array_append(&self->coderefs, from);
}

void BasicBlock_removeXref(BasicBlock* self, BasicBlock* from) {
	if(self == NULL) {
		return;
	}
	
	/* Number of xrefs will almost always be small, so O(n) is fine */
	foreach(&self->coderefs, pxref) {
		if(*pxref == from) {
			/* Remove the reference */
			array_removeElement(&self->coderefs, pxref);
			break;
		}
	}
	
	/* No xrefs and not the first basic block means this can be removed */
	if(self->prev != NULL && self->coderefs.count == 0) {
		BasicBlock* prev = self->prev;
		BasicBlock* next = self->next;
		
		/* Skip over this node in the doubly linked list */
		prev->next = next;
		if(next != NULL) {
			next->prev = prev;
		}
		
		/* Remove this node's references to other nodes */
		BasicBlock_removeXref(self->target, self);
		BasicBlock_removeXref(self->ztarget, self);
		
		/* Make sure not to release more basic blocks than this one */
		self->next = NULL;
		release(&self);
	}
}

void BasicBlock_addInsn(BasicBlock* self, Insn insn) {
	/* Add the instruction and invalidate the tail */
	array_append(&self->insns, insn);
	BasicBlock_invalidateTail(self);
}

Word BasicBlock_getAddress(BasicBlock* self) {
	return self->code_addr;
}

void BasicBlock_setAddress(BasicBlock* self, Word addr) {
	self->code_addr = addr;
}

void BasicBlock_resolve(BasicBlock* self, Block* scope) {
	/* Resolve all the undefined symbols in this basic block */
	foreach(&self->symrefs, symref) {
		Symbol* sym = symref->sym;
		Insn* pinsn = &self->insns.elems[symref->index];
		
		switch(sym->type) {
			case SYM_CONST:
				/* Set the immediate value of this LIT instruction to the value of the constant */
				pinsn->imm = sym->value.number;
				break;
			
			case SYM_VAR:
				/* Set the level and offset of the variable for this LOD/STO instruction */
				pinsn->lvl = scope->symtree->level - sym->level;
				pinsn->imm = sym->value.frame_offset;
				break;
				
			case SYM_PROC: {
				/* Find the address of the procedure being called */
				Block* blk = sym->value.procedure.body;
				Word addr = Block_getAddress(blk);
				if(addr == ADDR_UND) {
					/* Append the block to the block chain (lol) to give it an address */
					Block_append(scope, blk);
					addr = Block_getAddress(blk);
				}
				
				/* If this is a CAL instruction, set the relative level */
				if(pinsn->op == OP_CAL) {
					pinsn->lvl = scope->symtree->level - sym->level;
				}
				
				/* Set the target address of this CAL/JMP instruction */
				pinsn->imm = addr;
				break;
			}
			
			default:
				ASSERT(!"Invalid symbol type");
		}
	}
}

static void BasicBlock_removeInsn(BasicBlock* self, size_t index) {
	/* Remove the instruction from the instructions array */
	array_removeIndex(&self->insns, index);
	
	/* Look for a symbol that references the deleted instruction */
	enumerate(&self->symrefs, i, symref) {
		if(symref->index == index) {
			/* Remove symbol from symbol array */
			array_removeIndex(&self->symrefs, i--);
		}
		else if(symref->index > index) {
			/* Decrement each index after the removed element */
			--symref->index;
		}
	}
	
	/* Bump the condition index forward one if it was past the deleted instruction */
	if(self->cond_index > index) {
		--self->cond_index;
	}
	
	/* Bump the tail index forward one if it was past the deleted instruction */
	if(self->tail_index > index) {
		--self->tail_index;
	}
}

void BasicBlock_optimize(BasicBlock* self, Block* scope) {
	/* Need to rebuild the tail after any optimizations */
	BasicBlock_invalidateTail(self);
	
	/* Optimize any referenced procedures */
	enumerate(&self->symrefs, i, symref) {
		if(symref->sym->type == SYM_PROC) {
			Block* blk = symref->sym->value.procedure.body;
			
			/* Optimize the procedure */
			Block_optimize(blk);
			
			/* Is the call even required any more? */
			if(Block_isEmpty(blk)) {
				BasicBlock_removeInsn(self, symref->index);
				--i;
			}
		}
	}
	
	/* Bypass true and direct jumps */
	while(self->target != NULL && BasicBlock_isEmpty(self->target)) {
		BasicBlock_setTarget(self, self->target->target);
	}
	
	if(self->flags & BB_HAS_CONDITION) {
		/* Bypass false jumps in conditionals */
		while(self->ztarget != NULL && BasicBlock_isEmpty(self->ztarget)) {
			BasicBlock_setFalseTarget(self, self->ztarget->target);
		}
		
#if 0
		/* Is the condition even required any more? */
		if(self->target == self->ztarget) {
			/* Both branches have the same target, so remove the condition code */
			BasicBlock_setFalseTarget(self, NULL);
			self->conditional = false;
			
			/* Keep chopping off the condition one instruction at a time until there's nothing left */
			while(self->insn_count > self->cond_index) {
				BasicBlock_removeInsn(self, self->insn_count - 1);
			}
			self->cond_index = -1;
		}
#endif
	}
	
	/* Try to perform tail call optimization */
	BasicBlock_tailCallOptimize(self, scope);
}

#define ADD_INSN(insn) array_append(&self->insns, insn)
static void BasicBlock_genTail(BasicBlock* self, uint16_t level) {
	/* If this basic block already has a tail or has been tail call optimized, do nothing */
	if(HAS_ANY_FLAGS(self->flags, BB_HAS_TAIL | BB_TAIL_CALL_OPTIMIZED)) {
		return;
	}
	
	/* Starting the tail, so mark the position */
	self->flags |= BB_HAS_TAIL;
	self->tail_index = self->insns.count;
	
	/* Instruction used to end execution in this procedure */
	Insn ret = level == 0 ? MAKE_HALT() : MAKE_RET();
	
	/* Produce the jumps necessary to branch to the next basic block(s) */
	if((self->flags & BB_HAS_CONDITION) && self->target != self->ztarget) {
		/* Get last instruction (should be the condition ALU instruction) */
		ASSERT(self->tail_index >= 1);
		Insn last = self->insns.elems[self->tail_index - 1];
		
		/* Conditionally execute either target or ztarget */
		if(self->target == NULL) {
			/* Case 1: True branch is a RET */
#if FIXME
			if(self->ztarget == self->next) {
				/* Case 1a: Produce a jmp-true to a RET */
			}
			else /* ztarget != next */ {
#endif
				/* Case 1b: Produce a jmp-false to ztarget, then add RET */
				ADD_INSN(MAKE_JPC(self->ztarget->code_addr));
				ADD_INSN(ret);
#if FIXME
			}
#endif
		}
		else if(self->ztarget == NULL) {
			/* Case 2a: False branch is a RET */
#if FIXME
			if(self->target == self->next) {
				/* Case 2a.1: Produce a jmp-false to a RET */
			}
			else /* target != next */ {
				/* Case 2a.2: Produce a jmp-false to a RET followed by a JMP to target */
			}
#else
			/* Temporary solution: Produce a jmp-true to target followed by a RET */
			if(last.imm == ALU_ODD) {
				/* Invert ODD to EVEN by comparing its result to zero (ODD == 0) */
				ADD_INSN(MAKE_LIT(0));
				ADD_INSN(MAKE_EQL());
			}
			else {
				self->flags |= BB_INVERT_CONDITION;
			}
			ADD_INSN(MAKE_JPC(self->target->code_addr));
			ADD_INSN(ret);
#endif
		}
		else /* target != NULL && ztarget != NULL */ {
			/* Case 2b: Neither branch is a RET */
			if(self->target == self->next) {
				/* Case 2b.1: Produce a jmp-false to ztarget */
				ADD_INSN(MAKE_JPC(self->ztarget->code_addr));
			}
			else if(self->ztarget == self->next) {
				/* Case 2b.2: Produce a jmp-true to target */
				if(last.imm == ALU_ODD) {
					/* Invert ODD to EVEN by comparing its result to zero (ODD == 0) */
					ADD_INSN(MAKE_LIT(0));
					ADD_INSN(MAKE_EQL());
				}
				else {
					self->flags |= BB_INVERT_CONDITION;
				}
				ADD_INSN(MAKE_JPC(self->target->code_addr));
			}
			else /* target != next && ztarget != next */ {
				/* Case 2b.3: Produce a jmp-false to ztarget followed by a JMP to target */
				ADD_INSN(MAKE_JPC(self->ztarget->code_addr));
				ADD_INSN(MAKE_JMP(self->target->code_addr));
			}
		}
	}
	else {
		/* Unconditionally execute target */
		if(self->target == NULL) {
			/* Case 1: Produce a RET */
			ADD_INSN(ret);
		}
		else if(self->target != self->next) {
			/* Case 2: Produce a JMP to target */
			ADD_INSN(MAKE_JMP(self->target->code_addr));
		}
		/* Case 3 (target != NULL && target != next): Produce nothing */
	}
}
#undef ADD_INSN

static void BasicBlock_tailCallOptimize(BasicBlock* self, Block* scope) {
	/* Check whether this basic block has already been tail call optimized */
	if(self->flags & BB_TAIL_CALL_OPTIMIZED) {
		return;
	}
	
	/* Only operate on terminating basic blocks */
	if(self->insns.count < 1) {
		return;
	}
	
	bool isTailCall = false;
	Insn callInsn = MAKE_BREAK(-1);
	size_t callIndex = 0;
	
	/* Two cases of tail calls to consider */
	Insn lastInsn = self->insns.elems[self->insns.count - 1];
	if(lastInsn.op == OP_CAL) {
		/* Case 1: Tail call without assigning result to `return`
		 *
		 * Example assembly listing:
		 *
		 * CAL someProc
		 * RET
		 */
		isTailCall = true;
		callInsn = lastInsn;
		callIndex = self->insns.count - 1;
	}
	else if(self->insns.count >= 3) {
		/* Case 2: Tail call that assigns result to `return`.
		 *
		 * Example assembly listing:
		 *
		 * CAL someProc
		 * INC 1
		 * STO return
		 * RET
		 */
		
		Insn stoReturn = MAKE_STO(0, 0);
		Insn inc1 = MAKE_INC(1);
		Insn secondToLastInsn = self->insns.elems[self->insns.count - 2];
		Insn thirdFromLastInsn = self->insns.elems[self->insns.count - 3];
		
		bool lastInsnIsStoReturn = memcmp(&lastInsn, &stoReturn, sizeof(stoReturn)) == 0;
		bool secondToLastInsnIsInc1 = memcmp(&secondToLastInsn, &inc1, sizeof(inc1)) == 0;
		bool thirdFromLastInsnIsCall = thirdFromLastInsn.op == OP_CAL;
		if(lastInsnIsStoReturn && secondToLastInsnIsInc1 && thirdFromLastInsnIsCall) {
			isTailCall = true;
			callInsn = thirdFromLastInsn;
			callIndex = self->insns.count - 3;
		}
	}
	
	if(!isTailCall) {
		return;
	}
	
	/* STACK FRAME:
	 *
	 * BP+?: SCRATCH SPACE
	 * BP+N: VARS
	 * BP+4: PARAMS
	 * BP+3: RETURN ADDRESS
	 * BP+2: DYNAMIC LINK
	 * BP+1: STATIC LINK
	 * BP+0: RETURN VALUE
	 *
	 * Before TCO:
	 *
	 * INC 4
	 * LIT 1
	 * LIT ...
	 * LIT someProc.paramCount
	 * INC -(4 + someProc.paramCount)
	 * CAL someProc
	 * INC 1
	 * STO return
	 * RET
	 *
	 * After TCO:
	 *
	 * LIT 1
	 * LIT ...
	 * LIT someProc.paramCount
	 * INC -(thisProc.frameSize + someProc.paramCount) + 1 #adjust stack to static link location
	 * LOD (origCal.L-1) 1 #load static link
	 * INC 2 #leave dynamic link and return address unchanged
	 * LOD 0 (thisProc.frameSize + 0)
	 * LOD 0 (thisProc.frameSize + ...)
	 * LOD 0 (thisProc.frameSize + someProc.paramCount - 1)
	 * INC -(4 + someProc.paramCount)
	 * JMP someProc
	 */
	
	/* Compute frame size at the point of the tail call */
	Word paramCount = -1;
	Symbol* callee = NULL;
	foreach(&self->symrefs, xref) {
		if(xref->index == callIndex && xref->sym->type == SYM_PROC) {
			paramCount = (Word)xref->sym->value.procedure.param_count;
			callee = xref->sym;
		}
	}
	
	/* Couldn't find a symbol reference? */
	if(callee == NULL) {
		return;
	}
	
	/* Cannot perform TCO when calling a child procedure */
	Word callLevel = scope->symtree->level - callee->value.procedure.body->symtree->level;
	if(callLevel < 0) {
		return;
	}
	
	/* Load containing procedure's frame size */
	Word frameSize = scope->symtree->frame_size;
	
	/* Trim call instruction (and store to return if exists) */
	while(self->insns.count > callIndex) {
		BasicBlock_removeInsn(self, self->insns.count - 1);
	}
	
	/* Trim additional INC -(4 + someProc.paramCount) if exists */
	Insn incBeforeCall = MAKE_INC(-(4 + paramCount));
	Word stackOffset = 0;
	if(memcmp(&self->insns.elems[self->insns.count - 1], &incBeforeCall, sizeof(incBeforeCall)) == 0) {
		BasicBlock_removeInsn(self, self->insns.count - 1);
		stackOffset = 4 + paramCount;
	}
	
	/* INC 1 - thisProc.frameSize - stackOffset #adjust stack to static link location */
	Insn destroyCurrentFrame = MAKE_INC(1 - (frameSize + stackOffset));
	BasicBlock_addInsn(self, destroyCurrentFrame);
	
	/* LOD (origCal.L-1) 1 #load static link */
	Insn loadStaticLink = MAKE_LOD(callLevel, 1);
	BasicBlock_addInsn(self, loadStaticLink);
	
	if(paramCount > 0) {
		/* INC 2 #leave dynamic link and return address unchanged */
		Insn inc2 = MAKE_INC(2);
		BasicBlock_addInsn(self, inc2);
		
		Word i;
		for(i = 0; i < paramCount; i++) {
			/* LOD 0 (thisProc.frameSize + 4 + i) */
			Insn loadParam = MAKE_LOD(0, frameSize + 4 + i);
			BasicBlock_addInsn(self, loadParam);
		}
		
		/* INC -(4 + someProc.paramCount) */
		Insn leaveFrame = MAKE_INC(-(4 + paramCount));
		BasicBlock_addInsn(self, leaveFrame);
	}
	else {
		/* INC -2 */
		Insn incNeg2 = MAKE_INC(-2);
		BasicBlock_addInsn(self, incNeg2);
	}
	
	/* JMP someProc */
	BasicBlock_markSymbol(self, callee);
	Insn jmpProc = MAKE_JMP(callInsn.imm);
	BasicBlock_addInsn(self, jmpProc);
	
	/* Mark this basic block as having been tail call optimized */
	self->flags |= BB_TAIL_CALL_OPTIMIZED;
}

void BasicBlock_emit(BasicBlock* self, FILE* fp, uint16_t level) {
	ASSERT(self->code_addr != ADDR_UND);
	
	/* Generate the code for the tail of this basic block to handle control flow */
	BasicBlock_genTail(self, level);
	
	/* Output all instructions in this basic block */
	enumerate(&self->insns, i, pinsn) {
		/* Invert the condition instruction directly before the tail if the flag is set */
		if(HAS_ALL_FLAGS(self->flags, BB_INVERT_CONDITION | BB_HAS_TAIL) && i == self->tail_index - 1) {
			Insn_emit(MAKE_INV(*pinsn), fp);
		}
		else {
			Insn_emit(*pinsn, fp);
		}
	}
}

size_t BasicBlock_getInstructionCount(BasicBlock* self) {
	/* Build the tail to count the instructions, then invalidate it because the level is unknown */
	BasicBlock_genTail(self, 0);
	size_t ret = self->insns.count;
	BasicBlock_invalidateTail(self);
	return ret;
}

void BasicBlock_drawGraph(BasicBlock* self, Graphviz* gv, uint16_t level) {
	GVNode* node = GVNode_initWithIdentifier(GVNode_alloc(), "%p", self);
	if(BasicBlock_isEmpty(self)) {
		/* Empty node that should be removed in the simplify step */
		GVNode_addAttribute(node, "shape", "point");
	}
	else {
		/* The node should be a rectangular box and use a monospaced font for the disassembly */
		GVNode_addAttribute(node, "shape", "box");
		GVNode_addAttribute(node, "fontname", "Courier");
		GVNode_addAttribute(node, "style", "filled");
		
		/* Check first if it starts a procedure because it can be both first and last */
		const char* color;
		if(self->prev == NULL) {
			/* First basic block in a procedure */
			color = "palegreen";
		}
		else if(self->target == NULL) {
			/* Some terminating basic block of the procedure */
			color = "mistyrose";
		}
		else {
			/* Some random basic block in the middle */
			color = "white";
		}
		GVNode_addAttribute(node, "fillcolor", color);
		
		/* Generate the tail */
		BasicBlock_genTail(self, level);
		
		char* label = NULL;
		size_t sym_idx = 0;
		enumerate(&self->insns, i, pinsn) {
			/* Disassemble the instruction and invert the condition if necessary */
			char* dis;
			if(HAS_ALL_FLAGS(self->flags, BB_INVERT_CONDITION | BB_HAS_TAIL)
				&& i == self->tail_index - 1
			) {
				dis = Insn_prettyDis(MAKE_INV(*pinsn));
			}
			else {
				dis = Insn_prettyDis(*pinsn);
			}
			
			/* Add the address */
			char* addr;
			if(self->code_addr == ADDR_UND) {
				asprintf_ff(&addr, "UND:  %s", dis);
			}
			else {
				asprintf_ff(&addr, "%3d:  %s", self->code_addr + (Word)i, dis);
			}
			destroy(&dis);
			dis = addr;
			
			/* Check if any relocation entries apply to this instruction */
			if(sym_idx < self->symrefs.count) {
				element_type(self->symrefs)* symref = &self->symrefs.elems[sym_idx];
				if(i == symref->index) {
					/* There is a relocation entry for this instruction, so we can show more info */
					const char* procname = symref->sym->name;
					++sym_idx;
					
					/* Annotate the CAL instruction with the name of the procedure being called */
					char* extra_dis = rsprintf_ff(
						"%s (<font color=\"" CAL_COLOR "\">%s</font>)", dis, procname);
					destroy(&dis);
					dis = extra_dis;
				}
			}
			
			/* Add the disassembled instruction to the disassembly listing */
			BasicBlock_appendLine(&label, dis);
			destroy(&dis);
		}
		
		/* Add the disassembly listing as the label of the basic block node */
		GVNode_setLabel(node, "<%s>", label);
	}
	
	/* Draw the node for the basic block */
	GVNode_draw(node, gv);
	release(&node);
	
	/* Draw edges from this basic block to the targets */
	if(self->flags & BB_HAS_CONDITION) {
		if(self->target != NULL) {
			Graphviz_draw(gv, "<%p>:s -> <%p>:n [color=green];", self, self->target);
		}
		
		if(self->ztarget != NULL) {
			Graphviz_draw(gv, "<%p>:s -> <%p>:n [color=red];", self, self->ztarget);
		}
	}
	else if(self->target != NULL) {
		Graphviz_draw(gv, "<%p>:s -> <%p>:n [color=blue];", self, self->target);
	}
}

static void BasicBlock_appendLine(char** label, const char* line) {
	char* tmp = rsprintf_ff("%s%s<br align=\"left\"/>", *label ?: "", line ?: "");
	destroy(label);
	*label = tmp;
}
