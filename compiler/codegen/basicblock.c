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
static void BasicBlock_addInsnRaw(BasicBlock* self, Insn insn);
static void BasicBlock_removeInsn(BasicBlock* self, Word index);
static void BasicBlock_genTail(BasicBlock* self, uint16_t level);
static void BasicBlock_appendLine(char** label, const char* line);


Destroyer(BasicBlock) {
	release(&self->next);
	destroy(&self->insns);
	destroy(&self->syms);
	destroy(&self->sym_indices);
	destroy(&self->xrefs);
}

DEF_CUSTOM(BasicBlock) {
	self->code_addr = ADDR_UND;
	self->cond_index = -1;
	self->tail_index = -1;
	return self;
}

BasicBlock* BasicBlock_createNext(BasicBlock** bb) {
	/* Make sure not to replace the pointer to an existing basic block */
	assert((*bb)->next == NULL);
	BasicBlock* next = BasicBlock_new();
	next->prev = *bb;
	(*bb)->next = next;
	
	/* Invalidate tail index */
	BasicBlock_invalidateTail(*bb);
	
#if DEBUG
	Word i;
	for(i = 0; i < (*bb)->insn_count; i++) {
		assert((*bb)->insns[i].op != OP_BREAK);
	}
#endif
	
	/* Update pointer and return it */
	return *bb = next;
}

void BasicBlock_markCondition(BasicBlock* self) {
	self->cond_index = self->insn_count;
}

void BasicBlock_markSymbol(BasicBlock* self, Symbol* sym) {
	if(self->sym_count == self->sym_cap) {
		size_t old_cap = self->sym_cap;
		expand(&self->syms, &old_cap);
		expand(&self->sym_indices, &self->sym_cap);
	}
	
	self->syms[self->sym_count] = sym;
	self->sym_indices[self->sym_count++] = self->insn_count;
}

static void BasicBlock_invalidateTail(BasicBlock* self) {
	if(self->tail_index < 0) {
		return;
	}
	
	/* Clear the inverted condition flag, as that is set by genTail */
	self->invert_cond = false;
	
	/* Keep chopping off the tail one instruction at a time until there's nothing left */
	while(self->insn_count > self->tail_index) {
		BasicBlock_removeInsn(self, self->insn_count - 1);
	}
	self->tail_index = -1;
}

bool BasicBlock_isEmpty(BasicBlock* self) {
	/* Empty and is NOT the first basic block of a procedure */
	return self == NULL || (self->insn_count == 0 && self->prev != NULL);
}

bool BasicBlock_isEmptyProc(BasicBlock* self) {
	/* Empty procedure if there's at most one INC instruction and no other basic blocks */
	return self == NULL ||
	       (self->prev == NULL && self->next == NULL &&
	        (self->insn_count == 0 ||
	         (self->insn_count == 1 && self->insns[0].op == OP_INC)));
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
	self->conditional = true;
	
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
	
	/* Expand array if necessary */
	if(self->xref_count == self->xref_cap) {
		expand(&self->xrefs, &self->xref_cap);
	}
	
	/* Add xref */
	self->xrefs[self->xref_count++] = from;
}

void BasicBlock_removeXref(BasicBlock* self, BasicBlock* from) {
	if(self == NULL) {
		return;
	}
	
	/* Number of xrefs will almost always be small, so O(n) is fine */
	size_t i;
	for(i = 0; i < self->xref_count; i++) {
		if(self->xrefs[i] == from) {
			/* Remove the reference */
			delete_element(self->xrefs, i, self->xref_count--);
			break;
		}
	}
	
	/* No xrefs and not the first basic block means this can be removed */
	if(self->prev != NULL && self->xref_count == 0) {
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

static void BasicBlock_addInsnRaw(BasicBlock* self, Insn insn) {
	/* Expand the array if necessary */
	if(self->insn_count == self->insn_cap) {
		expand(&self->insns, &self->insn_cap);
	}
	
	/* Add the instruction */
	self->insns[self->insn_count++] = insn;
}

void BasicBlock_addInsn(BasicBlock* self, Insn insn) {
	/* Add the instruction and invalidate the tail */
	BasicBlock_addInsnRaw(self, insn);
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
	size_t i;
	for(i = 0; i < self->sym_count; i++) {
		Symbol* sym = self->syms[i];
		Insn* insn = &self->insns[self->sym_indices[i]];
		
		switch(sym->type) {
			case SYM_CONST:
				/* Set the immediate value of this LIT instruction to the value of the constant */
				insn->imm = sym->value.number;
				break;
			
			case SYM_VAR:
				/* Set the level and offset of the variable for this LOD/STO instruction */
				insn->lvl = scope->symtree->level - sym->level;
				insn->imm = sym->value.frame_offset;
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
				
				/* Set the target address and relative level of this CAL instruction */
				insn->lvl = scope->symtree->level - sym->level;
				insn->imm = addr;
				break;
			}
			
			default:
				abort();
		}
	}
}

static void BasicBlock_removeInsn(BasicBlock* self, Word index) {
	/* Remove the instruction from the instructions array */
	delete_element(self->insns, index, self->insn_count--);
	
	/* Look for a symbol that references the deleted instruction */
	size_t i;
	for(i = 0; i < self->sym_count; i++) {
		if(self->sym_indices[i] == index) {
			/* Remove symbol from symbol array */
			delete_element(self->sym_indices, i, self->sym_count);
			delete_element(self->syms, i, self->sym_count--);
			break;
		}
	}
	
	/* Continue looping through the symbols and decrement each index */
	for(; i < self->sym_count; i++) {
		--self->sym_indices[i];
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

void BasicBlock_optimize(BasicBlock* self) {
	/* Need to rebuild the tail after any optimizations */
	BasicBlock_invalidateTail(self);
	
	/* Optimize any referenced procedures */
	size_t i;
	for(i = 0; i < self->sym_count; i++) {
		if(self->syms[i]->type == SYM_PROC) {
			Block* blk = self->syms[i]->value.procedure.body;
			
			/* Optimize the procedure */
			Block_optimize(blk);
			
			/* Is the call even required any more? */
			if(Block_isEmpty(blk)) {
				BasicBlock_removeInsn(self, self->sym_indices[i--]);
			}
		}
	}
	
	/* Bypass true and direct jumps */
	while(self->target != NULL && BasicBlock_isEmpty(self->target)) {
		BasicBlock_setTarget(self, self->target->target);
	}
	
	if(self->conditional) {
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
}

#define ADD_INSN(insn) BasicBlock_addInsnRaw(self, insn)
static void BasicBlock_genTail(BasicBlock* self, uint16_t level) {
	if(self->tail_index >= 0) {
		return;
	}
	
	/* Starting the tail, so mark the position */
	self->tail_index = self->insn_count;
	
	/* Instruction used to end execution in this procedure */
	Insn ret = level == 0 ? MAKE_HALT() : MAKE_RET();
	
	/* Produce the jumps necessary to branch to the next basic block(s) */
	if(self->conditional && self->target != self->ztarget) {
		/* Get last instruction (should be the condition ALU instruction) */
		assert(self->tail_index >= 1);
		Insn last = self->insns[self->tail_index - 1];
		
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
				self->invert_cond = true;
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
					self->invert_cond = true;
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

void BasicBlock_emit(BasicBlock* self, FILE* fp, uint16_t level) {
	assert(self->code_addr != ADDR_UND);
	
	/* Generate the code for the tail of this basic block to handle control flow */
	BasicBlock_genTail(self, level);
	
	/* Output all instructions in this basic block */
	Word i;
	for(i = 0; i < self->insn_count; i++) {
		/* Invert the condition instruction directly before the tail if invert_cond is set */
		if(i >= 1 && i == self->tail_index - 1 && self->invert_cond) {
			Insn_emit(MAKE_INV(self->insns[i]), fp);
		}
		else {
			Insn_emit(self->insns[i], fp);
		}
	}
}

Word BasicBlock_getInstructionCount(BasicBlock* self) {
	/* Build the tail to count the instructions, then invalidate it because the level is unknown */
	BasicBlock_genTail(self, 0);
	Word ret = (Word)self->insn_count;
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
		Word i;
		size_t sym_idx = 0;
		for(i = 0; i < self->insn_count; i++) {
			/* Disassemble the instruction and invert the condition if necessary */
			char* dis;
			if(i >= 1 && i == self->tail_index - 1 && self->invert_cond) {
				dis = Insn_prettyDis(MAKE_INV(self->insns[i]));
			}
			else {
				dis = Insn_prettyDis(self->insns[i]);
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
			if(sym_idx < self->sym_count) {
				if(i == self->sym_indices[sym_idx]) {
					/* There is a relocation entry for this instruction, so we can show more info */
					const char* procname = self->syms[sym_idx++]->name;
					
					/* Annotate the CAL instruction with the name of the procedure being called */
					char* extra_dis;
					asprintf_ff(&extra_dis, "%s (<font color=\"" CAL_COLOR "\">%s</font>)", dis, procname);
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
	if(self->conditional) {
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
	char* tmp;
	asprintf_ff(&tmp, "%s%s<br align=\"left\"/>", *label ?: "", line ?: "");
	destroy(label);
	*label = tmp;
}
