//
//  symtree.c
//  PL/0
//
//  Created by Kevin Colley on 11/22/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "symtree.h"


/*! Add all parameters from a block's containing procedure
 @param params Node that holds all of the parameter declarations for a block
 */
static bool SymTree_addParams(SymTree* self, AST_ParamDecls* params);

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

/*! Compares a `const char* name` with the name of a `const Symbol** psym` */
static int compare_sym(const void* a, const void* b);


Destroyer(SymTree) {
	array_release(&self->children);
	array_release(&self->syms);
}
DEF(SymTree);

SymTree* SymTree_initWithAST(SymTree* self, SymTree* parent, AST_ParamDecls* params, AST_Block* block, uint16_t level) {
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
		if(!SymTree_addParams(self, params)        ||
		   !SymTree_addConsts(self, block->consts) ||
		   !SymTree_addVars(self, block->vars)     ||
		   !SymTree_addProcs(self, block->procs)) {
			release(&self);
			return NULL;
		}
	}
	
	return self;
}

static bool SymTree_addParams(SymTree* self, AST_ParamDecls* params) {
	bool success = true;
	
	/* Check if we were given parameters */
	if(params != NULL && params->params.count > 0) {
		/* Add variables for all the parameters */
		foreach(&params->params, pparam) {
			/* Create new parameter symbol for each param */
			Symbol* symParam = Symbol_new();
			symParam->type = SYM_VAR;
			symParam->name = strdup_ff(*pparam);
			symParam->level = self->level;
			symParam->value.frame_offset = self->frame_size++;
			
			/* Try to add the parameter into the Tree node */
			success = SymTree_addSymbol(self, symParam);
			
			/* The SymTree now holds a reference to symParam, so release our local reference */
			release(&symParam);
		}
	}
	
	return success;
}

static bool SymTree_addConsts(SymTree* self, AST_ConstDecls* decls) {
	/* Make sure the declarations exist */
	if(decls == NULL) {
		return true;
	}
	
	/* Iterate through each declaration and add a new symbol into the table */
	bool success = true;
	foreach(&decls->consts, pconst) {
		/* Create a new symbol for the constant */
		Symbol* sym = Symbol_new();
		sym->type = SYM_CONST;
		sym->name = strdup_ff(pconst->ident);
		sym->level = self->level;
		
		/* Set the constant's numeric value */
		sym->value.number = pconst->value;
		
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
	foreach(&decls->vars, pvar) {
		/* Create a new symbol for the variable */
		Symbol* sym = Symbol_new();
		sym->type = SYM_VAR;
		sym->name = strdup_ff(*pvar);
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
	foreach(&decls->procs, pproc) {
		/* Create a new symbol for the procedure */
		Symbol* symProc = Symbol_new();
		symProc->type = SYM_PROC;
		symProc->name = strdup_ff((*pproc)->ident);
		symProc->level = self->level;
		
		/* Create the child symtrees */
		SymTree* child = SymTree_initWithAST(SymTree_alloc(), self, (*pproc)->param_decls, (*pproc)->body, self->level + 1);
		if(!child) {
			release(&symProc);
			return false;
		}
		
		/* Set the parameter count */
		size_t param_count = (*pproc)->param_decls ? (*pproc)->param_decls->params.count : 0;
		symProc->value.procedure.param_count = param_count;
		
		/* Create the block */
		symProc->value.procedure.body = Block_initWithScope(Block_alloc(), child);
		
		/* Need to add the symbol before codegen-ing its block to handle recursion */
		success = SymTree_addSymbol(self, symProc);
		release(&symProc);
	}
	
	return success;
}

static int compare_sym(const void* key, const void* elem) {
	const char* name = key;
	const Symbol* sym = *(const Symbol**)elem;
	return strcmp(name, sym->name);
}

bool SymTree_addSymbol(SymTree* self, Symbol* sym) {
	/* Have to manually bsearch to find the place to insert the symbol */
	size_t lo = 0, hi = self->syms.count;
	while(lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		
		/* Determine where the symbol should go in relation to the middle element */
		int diff = compare_sym(sym->name, &self->syms.elems[mid]);
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
	retain(sym);
	array_insert(&self->syms, lo, &sym);
	return true;
}

void SymTree_addChild(SymTree* self, SymTree* child) {
	if(self == NULL) {
		return;
	}
	
	/* Add the child into the array of children */
	array_append(&self->children, child);
	
	/* Give the child a pointer to its parent */
	child->parent = self;
}

Symbol* SymTree_findSymbol(SymTree* self, const char* name) {
	/* Base case, this means that we never found a symbol when recursing up through the tree */
	if(self == NULL) {
		return NULL;
	}
	
	/* Binary search through the symbol array and see if it contains our symbol */
	Symbol** psym = bsearch(name,
		self->syms.elems, self->syms.count, element_size(self->syms), &compare_sym);
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
	foreach(&self->syms, psym) {
		Symbol_write(*psym, fp);
	}
	
	/* Write all the symbols in each of the child nodes */
	foreach(&self->children, pchild) {
		SymTree_write(*pchild, fp);
	}
}

void SymTree_drawProcs(SymTree* self, Graphviz* gv) {
	foreach(&self->syms, psym) {
		/* Only draw procedures */
		if((*psym)->type == SYM_PROC) {
			/* Create subgraph for this procedure */
			Graphviz* proc = Graphviz_initWithParentGraph(Graphviz_alloc(),
				gv, "cluster_%s", (*psym)->name);
			
			/* Graph's outline color is blue and the font for the procedure name is Courier */
			Graphviz_draw(proc, "color=blue;");
			Graphviz_draw(proc, "fontname=Courier;");
			
			/* Draw name of procedure and its param count */
			Graphviz_draw(proc,
				"label=<<font color=\"" CAL_COLOR "\">%s</font> (%zu parameters)>;",
				(*psym)->name, (*psym)->value.procedure.param_count);
			
			/* Draw the code graph */
			Block_drawGraph((*psym)->value.procedure.body, proc);
			release(&proc);
		}
	}
}
