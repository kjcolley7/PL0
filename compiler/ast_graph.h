//
//  ast_graph.h
//  PL/0
//
//  Created by Kevin Colley on 2/13/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#ifndef PL0_AST_GRAPH_H
#define PL0_AST_GRAPH_H

#include "ast_nodes.h"
#include "graphviz.h"

void AST_Block_drawGraph(AST_Block* self, Graphviz* gv);
void AST_ConstDecls_drawGraph(AST_ConstDecls* self, Graphviz* gv);
void AST_VarDecls_drawGraph(AST_VarDecls* self, Graphviz* gv);
void AST_ProcDecls_drawGraph(AST_ProcDecls* self, Graphviz* gv);
void AST_Proc_drawGraph(AST_Proc* self, Graphviz* gv);
void AST_ParamDecls_drawGraph(AST_ParamDecls* self, Graphviz* gv);
void AST_Stmt_drawGraph(AST_Stmt* self, Graphviz* gv);
void AST_Cond_drawGraph(AST_Cond* self, Graphviz* gv);
void AST_Expr_drawGraph(AST_Expr* self, Graphviz* gv);
void AST_Term_drawGraph(AST_Term* self, Graphviz* gv);
void AST_Factor_drawGraph(AST_Factor* self, Graphviz* gv);

#endif /* PL0_AST_GRAPH_H */
