//
//  symtree.c
//  PL/0
//
//  Created by Kevin Colley on 11/22/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "symtree.h"


/*! Add all const names from a given block's declarations
 @param decls Node that holds all of the constant declarations in a block
 */
static bool SymTree_addConsts(SymTree* self, AST_ConstDecls* decls);

/*! Add all var names from a given block's declarations
 @param decls Node that holds all of the variable declarations in a block
 */
static bool SymTree_addVars(SymTree* self, AST_VarDecls* decls);

/*! Add all proc names from a given block's declarations
 @param decls Node that holds all of the procedure declarations in a block
 */
static bool SymTree_addProcs(SymTree* self, AST_ProcDecls* decls);

/*! Add a new symbol into the current SymTree node. Holds a strong reference
 @param sym The Symbol that is being added into the tree
 @return True on success, or false if the symbol already exists at this level
 */
static bool SymTree_addSymbol(SymTree* self, Symbol* sym);

/*! Compares a `const char* name` with the name of a `const Symbol** psym` */
static int compare_sym(const void* a, const void* b);


Destroyer(SymTree) {
	size_t i;
	for(i = 0; i < self->child_count; i++) {
		release(&self->children[i]);
	}
	destroy(&self->children);
	
	for(i = 0; i < self->sym_count; i++) {
		release(&self->syms[i]);
	}
	destroy(&self->syms);
}
DEF(SymTree);

SymTree* SymTree_initWithAST(SymTree* self, SymTree* parent, AST_Block* block, uint16_t level) {
	if((self = SymTree_init(self))) {
		/* Set the hierarchy first */
		SymTree_addChild(parent, self);
		
		/* Set the node's level */
		self->level = level;
		
		/* Set the base stack frame size of the procedure */
		self->frame_size = level == 0 ? 0 : 4;
		
		/* Top-level doesn't have a "return" */
		if(level > 0) {
			/* Create a new symbol for the procedure's return value */
			Symbol* ret = Symbol_new();
			ret->type = SYM_VAR;
			ret->name = strdup_ff("return");
			ret->level = level;
			ret->value.frame_offset = 0;
			
			/* Add "return" before anything else, so redeclaration errors are correct */
			bool success = SymTree_addSymbol(self, ret);
			release(&ret);
			if(!success) {
				release(&self);
				return NULL;
			}
		}
		
		/* Add consts, vars, and procs into the symbol tree and sort them */
		if(!SymTree_addConsts(self, block->consts) ||
		   !SymTree_addVars(self, block->vars)     ||
		   !SymTree_addProcs(self, block->procs)) {
			release(&self);
			return NULL;
		}
	}
	
	return self;
}

static bool SymTree_addConsts(SymTree* self, AST_ConstDecls* decls) {
	/* Make sure the declarations exist */
	if(decls == NULL) {
		return true;
	}
	
	/* Iterate through each declaration and add a new symbol into the table */
	bool success = true;
	size_t i;
	for(i = 0; i < decls->const_count; i++) {
		/* Create a new symbol for the constant */
		Symbol* sym = Symbol_new();
		sym->type = SYM_CONST;
		sym->name = strdup_ff(decls->idents[i]);
		sym->level = self->level;
		
		/* Set the constant's numeric value */
		sym->value.number = decls->values[i];
		
		/* Add the constant symbol to the current level of the tree */
		success = SymTree_addSymbol(self, sym);
		release(&sym);
	}
	
	return success;
}

static bool SymTree_addVars(SymTree* self, AST_VarDecls* decls) {
	/* Make sure the declarations exist */
	if(decls == NULL) {
		return true;
	}
	
	/* Iterate through each declaration and add a new symbol into the table */
	bool success = true;
	size_t i;
	for(i = 0; i < decls->var_count; i++) {
		/* Create a new symbol for the variable */
		Symbol* sym = Symbol_new();
		sym->type = SYM_VAR;
		sym->name = strdup_ff(decls->vars[i]);
		sym->level = self->level;
		
		/* Set the variable's local stack offset and increment the offset */
		sym->value.frame_offset = self->frame_size++;
		
		/* Add the variable symbol to the current level of the tree */
		success = SymTree_addSymbol(self, sym);
		release(&sym);
	}
	
	return success;
}

static bool SymTree_addProcs(SymTree* self, AST_ProcDecls* decls) {
	/* Make sure the declarations exist */
	if(decls == NULL) {
		return true;
	}
	
	/* Iterate through each declaration and add a new symbol into the table */
	bool success = true;
	size_t i, j;
	for(i = 0; i < decls->proc_count; i++) {
		/* Create a new symbol for the procedure */
		Symbol* sym = Symbol_new();
		AST_Proc* proc = decls->procs[i];
		sym->type = SYM_PROC;
		sym->name = strdup_ff(proc->ident);
		sym->level = self->level;
		
		/* Create the child symtrees */
		SymTree* child = SymTree_initWithAST(SymTree_alloc(), self, proc->body, self->level + 1);
		
		/* Set the parameter count */
		size_t param_count = proc->param_decls ? proc->param_decls->param_count : 0;
		sym->value.procedure.param_count = param_count;
		
		/* Add variables for all the parameters */
		for(j = 0; j < param_count; j++) {
			/* Create new parameter symbol for each param */
			Symbol* param = Symbol_new();
			param->type = SYM_VAR;
			param->name = strdup_ff(proc->param_decls->params[j]);
			param->level = child->level;
			param->value.frame_offset = child->frame_size++;
			
			/* Try to add the parameter into the Tree node */
			success = SymTree_addSymbol(child, param);
			release(&param);
			if(!success) {
				release(&sym);
				return false;
			}
		}
		
		/* Create the block */
		sym->value.procedure.body = Block_initWithScope(Block_alloc(), child);
		
		/* Need to add the symbol before codegen-ing its block to handle recursion */
		success = SymTree_addSymbol(self, sym);
		release(&sym);
	}
	
	return success;
}

static int compare_sym(const void* a, const void* b) {
	const char* name = a;
	const Symbol* sym = *(const Symbol**)b;
	return strcmp(name, sym->name);
}

static bool SymTree_addSymbol(SymTree* self, Symbol* sym) {
	/* Check to see if the array has hit its capacity */
	if(self->sym_count == self->sym_cap) {
		expand(&self->syms, &self->sym_cap);
	}
	
	/* Have to manually bsearch to find the place to insert the symbol */
	size_t lo = 0, hi = self->sym_count;
	while(lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		
		/* Determine where the symbol should go in relation to the middle element */
		int diff = compare_sym(sym->name, &self->syms[mid]);
		if(diff < 0) {
			/* Not mid - 1 because we want to insert before hi */
			hi = mid;
		}
		else if(diff > 0) {
			lo = mid + 1;
		}
		else {
			/* Symbol already exists in this level, so this is a redefinition (illegal) */
			return false;
		}
	}
	
	/* Insert the symbol into its position */
	Symbol* ref = retain(sym);
	insert_element(self->syms, lo, &ref, self->sym_count++);
	return true;
}

void SymTree_addChild(SymTree* self, SymTree* child) {
	if(self == NULL) {
		return;
	}
	
	/* Check to see if the array has hit its capacity */
	if(self->child_count == self->child_cap) {
		expand(&self->children, &self->child_cap);
	}
	
	/* Add the child into the array of children */
	self->children[self->child_count++] = child;
	
	/* Give the child a pointer to its parent */
	child->parent = self;
}

Symbol* SymTree_findSymbol(SymTree* self, const char* name) {
	/* Base case, this means that we never found a symbol when recursing up through the tree */
	if(self == NULL) {
		return NULL;
	}
	
	/* Binary search through the symbol array and see if it contains our symbol */
	Symbol** psym = bsearch(name, self->syms, self->sym_count, sizeof(*self->syms), &compare_sym);
	if(psym != NULL) {
		return *psym;
	}
	
	/* If we never found a symbol, then check this node's parent */
	return SymTree_findSymbol(self->parent, name);
}

void SymTree_write(SymTree* self, FILE* fp) {
	if(self == NULL) {
		return;
	}
	
	/* Write all the symbols in the current level of the symbol tree */
	size_t i;
	for(i = 0; i < self->sym_count; i++) {
		Symbol_write(self->syms[i], fp);
	}
	
	/* Write all the symbols in each of the child nodes */
	for(i = 0; i < self->child_count; i++) {
		SymTree_write(self->children[i], fp);
	}
}

void SymTree_drawProcs(SymTree* self, Graphviz* gv) {
	size_t i;
	for(i = 0; i < self->sym_count; i++) {
		Symbol* sym = self->syms[i];
		
		/* Only draw procedures */
		if(sym->type == SYM_PROC) {
			/* Create subgraph for this procedure */
			Graphviz* proc = Graphviz_initWithParentGraph(
				Graphviz_alloc(), gv, "cluster_%s", sym->name);
			
			/* Graph's outline color is blue and the font for the procedure name is Courier */
			Graphviz_draw(proc, "color=blue;");
			Graphviz_draw(proc, "fontname=Courier;");
			
			/* Draw name of procedure and its param count */
			Graphviz_draw(proc,
				"label=<<font color=\"" CAL_COLOR "\">%s</font> (%zu parameters)>;",
				sym->name, sym->value.procedure.param_count);
			
			/* Draw the code graph */
			Block_drawGraph(self->syms[i]->value.procedure.body, proc);
			release(&proc);
		}
	}
}
