//
//  block.c
//  PL/0
//
//  Created by Kevin Colley on 11/14/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "block.h"
#include "codegen.h"
#include "gvnode.h"


static Word Block_getCodeLength(Block* self);
static void Block_resolve(Block* self);
static Block* Block_getLast(Block* self);
static void Block_setNext(Block* self, Block* next);


Destroyer(Block) {
	release(&self->symtree);
	release(&self->code);
}
DEF(Block);

Block* Block_initWithScope(Block* self, SymTree* symtree) {
	if((self = Block_init(self))) {
		self->symtree = symtree;
	}
	
	return self;
}

bool Block_generate(Block* self, AST_Block* ast) {
	size_t i;
	for(i = 0; i < ast->procs->proc_count; i++) {
		/* Generate the code for all subprocedures of this procedure */
		AST_Proc* proc = ast->procs->procs[i];
		Symbol* sym = SymTree_findSymbol(self->symtree, proc->ident->name);
		if(!Block_generate(sym->value.procedure.body, proc->body)) {
			return false;
		}
	}
	
	/* Generate the code for this block */
	return Codegen_genBlock(self, ast);
}

void Block_optimize(Block* self) {
	/* Make sure we don't do this again due to recursive calls */
	if(self->optimized) {
		return;
	}
	self->optimized = true;
	
	/* Find last basic block */
	BasicBlock* cur = self->code;
	while(cur->next != NULL) {
		cur = cur->next;
	}
	
	/* Bypass all jumps to empty basic blocks */
	while(cur != NULL) {
		/* Save previous basic block because cur might be destroyed */
		BasicBlock* prev = cur->prev;
		
		/* Optimize referenced blocks (this might destroy cur!) */
		BasicBlock_optimize(cur);
		cur = prev;
	}
}

bool Block_isEmpty(Block* self) {
	return BasicBlock_isEmptyProc(self->code);
}

Word Block_getAddress(Block* self) {
	/* Produce the block if it doesn't yet have an address */
	return BasicBlock_getAddress(self->code);
}

void Block_setAddress(Block* self, Word addr) {
	/* Count instructions in this block */
	self->code_length = Block_getCodeLength(self);
	
	BasicBlock* cur = self->code;
	while(cur != NULL) {
		/* Set the address of each basic block in the code graph */
		BasicBlock_setAddress(cur, addr);
		addr += BasicBlock_getInstructionCount(cur);
		cur = cur->next;
	}
	
	/* Resolve all references in the code graph */
	Block_resolve(self);
}

static Word Block_getCodeLength(Block* self) {
	Word length = 0;
	BasicBlock* cur = self->code;
	while(cur != NULL) {
		length += BasicBlock_getInstructionCount(cur);
		cur = cur->next;
	}
	return length;
}

static void Block_resolve(Block* self) {
	BasicBlock* cur = self->code;
	while(cur != NULL) {
		/* Resolve each basic block's undefined symbols */
		BasicBlock_resolve(cur, self);
		cur = cur->next;
	}
}

static Block* Block_getLast(Block* self) {
	/* Recursively update the last pointers. Amortized O(1). Normally O(1) with O(n) worst case */
	if(self->last == NULL) {
		return self;
	}
	
	/* Use path compression for speed */
	return self->last = Block_getLast(self->last);
}

static void Block_setNext(Block* self, Block* next) {
	assert(self->next == NULL);
	Block_setAddress(next, Block_getAddress(self) + self->code_length);
	self->last = self->next = next;
}

void Block_append(Block* self, Block* last) {
	/* More path compression */
	Block* end = Block_getLast(self);
	Block_setNext(end, last);
	self->last = last;
}

void Block_emit(Block* self, FILE* fp) {
	Block* blk = self;
	while(blk != NULL) {
		/* Emit the code for each basic block in the code graph */
		BasicBlock* cur = blk->code;
		while(cur != NULL) {
			BasicBlock_emit(cur, fp, blk->symtree->level);
			cur = cur->next;
		}
		
		blk = blk->next;
	}
}

void Block_drawGraph(Block* self, Graphviz* gv) {
	/* Draw this block's CFG */
	BasicBlock* cur = self->code;
	while(cur != NULL) {
		BasicBlock_drawGraph(cur, gv, self->symtree->level);
		cur = cur->next;
	}
	
	/* Draw CFGs for subprocedures */
	SymTree_drawProcs(self->symtree, gv);
}
