//
//  symtree.h
//  PL/0
//
//  Created by Kevin Colley on 11/22/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_SYMTREE_H
#define PL0_SYMTREE_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SymTree SymTree;

#include "object.h"
#include "symbol.h"
#include "config.h"
#include "compiler/ast_nodes.h"

struct SymTree {
	OBJECT_BASE;
	
	/*! Parent node of the symbol tree (or NULL at the root node) */
	SymTree* parent;
	
	/*! Lexical level for this node in the symbol tree (same as this node's height) */
	uint16_t level;
	
	/*! Number of children this node has */
	size_t child_count;
	
	/*! Allocated capacity for children for this node */
	size_t child_cap;
	
	/*! Array of children to this node */
	SymTree** children;
	
	/*! Number of symbols in this part of the symbol table */
	size_t sym_count;
	
	/*! Allocated capacity for symbols in this part of the symbol table */
	size_t sym_cap;
	
	/*! Sorted array of symbols */
	Symbol** syms;
	
	/*! Current size of the stack frame */
	Word frame_size;
};
DECL(SymTree);


/*! Initialize the symbol tree given an AST of the block
 @param parent Parent node in the symbol tree to this one
 @param block AST node for the block used to recursively create the symbol tree
 @param level Current lexicographic level of the SymTree
 */
SymTree* SymTree_initWithAST(SymTree* self, SymTree* parent, AST_Block* block, uint16_t level);

/*! Add a new child into the current SymTree node
 @param child The child that is being added into the tree
 */
void SymTree_addChild(SymTree* self, SymTree* child);

/*! Lookup the symbol with the given name in this node or any parents
 @param name Name of symbol to lookup
 @return The symbol that was found, or NULL if it wasn't found
 */
Symbol* SymTree_findSymbol(SymTree* self, const char* name);

/*! Writes the symbol table output as specified in the assignment description
 @param fp File stream to write the table output to
 */
void SymTree_write(SymTree* self, FILE* fp);

/*! Draw all procedures in the symbol tree */
void SymTree_drawProcs(SymTree* self, Graphviz* gv);


#endif /* PL0_SYMTREE_H */
