//
//  ast_nodes.c
//  PL/0
//
//  Created by Kevin Colley on 10/26/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

/****************************************************************************************\
 * Just a bunch of boilerplate code to make AST node objects that manage memory nicely. *
 * Should be fairly straightforward, hence the lack of commenting.                      *
\****************************************************************************************/

#include "ast_nodes.h"
#include <assert.h>


Destroyer(AST_Program) {
	release(&self->block);
}
DEF(AST_Program);

/*! Example:
 
 @code
   +-------+
   |program|
   +-------+
       |
     block
 @endcode
 */
void AST_Program_drawGraph(AST_Program* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>program</i>");
	AST_Block_drawGraph(self->block, gv);
	Graphviz_drawPtrEdge(gv, self, self->block);
}

Destroyer(AST_Block) {
	release(&self->consts);
	release(&self->vars);
	release(&self->procs);
	release(&self->stmt);
}
DEF(AST_Block);

/*! Example:
 
 @code
          +-----+
          |block|
          +-----+
    _____/ /  \ \_____
   /      /    \      \
 consts vars  procs  stmt
 @endcode
 */
void AST_Block_drawGraph(AST_Block* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>block</i>");
	
	if(self->consts != NULL) {
		AST_ConstDecls_drawGraph(self->consts, gv);
		Graphviz_drawPtrEdge(gv, self, self->consts);
	}
	
	if(self->vars != NULL) {
		AST_VarDecls_drawGraph(self->vars, gv);
		Graphviz_drawPtrEdge(gv, self, self->vars);
	}
	
	if(self->procs != NULL && self->procs->proc_count > 0) {
		AST_ProcDecls_drawGraph(self->procs, gv);
		Graphviz_drawPtrEdge(gv, self, self->procs);
	}
	
	AST_Stmt_drawGraph(self->stmt, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt);
}

Destroyer(AST_ConstDecls) {
	size_t i;
	for(i = 0; i < self->const_count; i++) {
		release(&self->idents[i]);
		release(&self->values[i]);
	}
	
	destroy(&self->idents);
	destroy(&self->values);
}
DEF(AST_ConstDecls);

/*! Example: const a = 4, b = 9, c = 0;

 @code
    +-----------+
    |const-decls|
    +-----------+
    /     |     \
   =      =      =
  / \    / \    / \
 a   4  b   9  c   0
 @endcode
 */
void AST_ConstDecls_drawGraph(AST_ConstDecls* self, Graphviz* gv) {
	char* node_id = ptos(self);
	Graphviz_drawNode(gv, node_id, "<i>const-decls</i>");
	
	size_t i;
	for(i = 0; i < self->const_count; i++) {
		char* const_id;
		asprintf_ff(&const_id, "%p[%zu]", self, i);
		
		Graphviz_drawNode(gv, const_id, "=");
		Graphviz_drawEdge(gv, node_id, const_id);
		
		char* name_id = ptos(self->idents[i]);
		AST_Ident_drawGraph(self->idents[i], gv);
		
		Graphviz_drawEdge(gv, const_id, name_id);
		destroy(&name_id);
		
		char* value_id = ptos(self->values[i]);
		AST_Number_drawGraph(self->values[i], gv);
		
		Graphviz_drawEdge(gv, const_id, value_id);
		destroy(&value_id);
		destroy(&const_id);
	}
	
	destroy(&node_id);
}

Destroyer(AST_ParamDecls) {
	size_t i;
	for(i = 0; i < self->param_count;i++) {
		release(&self->params[i]);
	}
	
	destroy(&self->params);
}
DEF(AST_ParamDecls);

void AST_ParamDecls_drawGraph(AST_ParamDecls* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>parameter-block</i>");
	
	size_t i;
	for(i = 0; i < self->param_count; i++) {
		AST_Ident_drawGraph(self->params[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->params[i]);
	}
}

Destroyer(AST_VarDecls) {
	size_t i;
	for(i = 0; i < self->var_count; i++) {
		release(&self->vars[i]);
	}
	
	destroy(&self->vars);
}
DEF(AST_VarDecls);

/*! Example: var x, y;
 
 @code
  +---------+
  |var-decls|
  +---------+
     /   \
    x     y
 @endcode
 */
void AST_VarDecls_drawGraph(AST_VarDecls* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>var-decls</i>");
	
	size_t i;
	for(i = 0; i < self->var_count; i++) {
		AST_Ident_drawGraph(self->vars[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->vars[i]);
	}
}

Destroyer(AST_ProcDecls) {
	size_t i;
	for(i = 0; i < self->proc_count; i++) {
		release(&self->procs[i]);
	}

	destroy(&self->procs);
}
DEF(AST_ProcDecls);

/*! Example:
 
 procedure A;;
 procedure B;;
 
 @code
   +----------+
   |proc-decls|
   +----------+
     /     \
    A       B
 @endcode
 */
void AST_ProcDecls_drawGraph(AST_ProcDecls* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>proc-decls</i>");
	
	size_t i;
	for(i = 0; i < self->proc_count; i++) {
		AST_Proc_drawGraph(self->procs[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->procs[i]);
	}
}

Destroyer(AST_Proc) {
	release(&self->ident);
	release(&self->body);
}
DEF(AST_Proc);

/*! Example:
 
 @code
 procedure A;
 block...;
 @endcode
 
 @code
   +---------+
   |procedure|
   +---------+
     /    \
    A    block
 @endcode
 */
void AST_Proc_drawGraph(AST_Proc* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>procedure</i>");
	AST_Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
	AST_ParamDecls_drawGraph(self->param_decls, gv);
	Graphviz_drawPtrEdge(gv, self, self->param_decls);
	AST_Block_drawGraph(self->body, gv);
	Graphviz_drawPtrEdge(gv, self, self->body);
}

Destroyer(AST_Stmt) {
	switch(self->type) {
		case STMT_ASSIGN:
			release(&self->stmt.assign);
			break;
		
		case STMT_CALL:
			release(&self->stmt.call);
			break;
		
		case STMT_BEGIN:
			release(&self->stmt.begin);
			break;
		
		case STMT_IF:
			release(&self->stmt.if_stmt);
			break;
		
		case STMT_WHILE:
			release(&self->stmt.while_stmt);
			break;
		
		case STMT_READ:
			release(&self->stmt.read);
			break;
		
		case STMT_WRITE:
			release(&self->stmt.write);
			break;
		
		case STMT_EMPTY:
			break;
		
		default:
			assert(false);
	}
}
DEF(AST_Stmt);

void AST_Stmt_drawGraph(AST_Stmt* self, Graphviz* gv) {
	if(self->type == STMT_EMPTY) {
		/* Draw empty statements in red to be more easily seen */
		Graphviz_drawPtrNode(gv, self, "<font color=\"red\"><i>statement</i></font>");
		return;
	}
	
	Graphviz_drawPtrNode(gv, self, "<i>statement</i>");
	
	/* Draw the actual statement nodes and then add an edge to the subtree */
	switch(self->type) {
		case STMT_ASSIGN:
			Stmt_Assign_drawGraph(self->stmt.assign, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.assign);
			break;
			
		case STMT_CALL:
			Stmt_Call_drawGraph(self->stmt.call, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.call);
			break;
			
		case STMT_BEGIN:
			Stmt_Begin_drawGraph(self->stmt.begin, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.begin);
			break;
			
		case STMT_IF:
			Stmt_If_drawGraph(self->stmt.if_stmt, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.if_stmt);
			break;
			
		case STMT_WHILE:
			Stmt_While_drawGraph(self->stmt.while_stmt, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.while_stmt);
			break;
			
		case STMT_READ:
			Stmt_Read_drawGraph(self->stmt.read, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.read);
			break;
			
		case STMT_WRITE:
			Stmt_Write_drawGraph(self->stmt.write, gv);
			Graphviz_drawPtrEdge(gv, self, self->stmt.write);
			break;
			
		default:
			abort();
	}
}

Destroyer(AST_Cond) {
	release(&self->left);
	release(&self->right);
}
DEF(AST_Cond);

/*! Example: a < b + 4
 
 @code
   +---------+
   |condition|
   +---------+
        |
        <
       / \
      a   +
         / \
        b   4
 @endcode
 */
void AST_Cond_drawGraph(AST_Cond* self, Graphviz* gv) {
	char* node_id = ptos(self);
	Graphviz_drawNode(gv, node_id, "<i>condition</i>");
	
	const char* op_str = NULL;
	switch(self->type) {
		case COND_ODD: op_str = "odd"; break;
		case COND_EQ:  op_str = "=";   break;
		case COND_GE:  op_str = "&gt;=";  break;
		case COND_GT:  op_str = "&gt;";   break;
		case COND_LE:  op_str = "&lt;=";  break;
		case COND_LT:  op_str = "&lt;";   break;
		case COND_NEQ: op_str = "&lt;&gt;";  break;
		default:
			abort();
	}
	
	/* Draw relational operator node */
	char* op_id;
	asprintf_ff(&op_id, "%p(%s)", self, op_str);
	Graphviz_drawNode(gv, op_id, op_str);
	Graphviz_drawEdge(gv, node_id, op_id);
	
	/* Draw expression a */
	char* a_id = ptos(self->left);
	AST_Expr_drawGraph(self->left, gv);
	Graphviz_drawEdge(gv, op_id, a_id);
	destroy(&a_id);
	
	/* Draw expression b (if it exists) */
	if(self->type != COND_ODD) {
		char* b_id = ptos(self->right);
		AST_Expr_drawGraph(self->right, gv);
		Graphviz_drawEdge(gv, op_id, b_id);
		destroy(&b_id);
	}
	
	destroy(&op_id);
	destroy(&node_id);
}

Destroyer(AST_Expr) {
	release(&self->left);
	release(&self->right);
}
DEF(AST_Expr);

void AST_Expr_drawGraph(AST_Expr* self, Graphviz* gv) {
	char* node_id = ptos(self);
	Graphviz_drawNode(gv, node_id, "<i>expression</i>");
	
	/* Draw binop node (+/-) if there are two operands */
	const char* expr_parent_id = node_id;
	char* binop_id = NULL;
	if(self->right != NULL) {
		const char* op_str = self->subtract ? "-" : "+";
		asprintf_ff(&binop_id, "%p(%s)", self, op_str);
		expr_parent_id = binop_id;
		Graphviz_drawNode(gv, binop_id, op_str);
		Graphviz_drawEdge(gv, node_id, binop_id);
	}
	
	/* Draw a negation node above a if a is negated */
	const char* a_parent_id = expr_parent_id;
	char* neg_id = NULL;
	if(self->a_negative) {
		asprintf_ff(&neg_id, "%p(-)", self->left);
		a_parent_id = neg_id;
		Graphviz_drawNode(gv, neg_id, "-");
		Graphviz_drawEdge(gv, expr_parent_id, neg_id);
	}
	
	/* Draw term a */
	char* a_id = ptos(self->left);
	AST_Term_drawGraph(self->left, gv);
	Graphviz_drawEdge(gv, a_parent_id, a_id);
	destroy(&neg_id);
	destroy(&a_id);
	
	/* Draw expr b (if it exists) */
	if(self->right != NULL) {
		char* b_id = ptos(self->right);
		AST_Expr_drawGraph(self->right, gv);
		Graphviz_drawEdge(gv, expr_parent_id, b_id);
		destroy(&b_id);
	}
	
	destroy(&binop_id);
	destroy(&node_id);
}

Destroyer(AST_Term) {
	release(&self->left);
	release(&self->right);
}
DEF(AST_Term);

void AST_Term_drawGraph(AST_Term* self, Graphviz* gv) {
	char* node_id = ptos(self);
	Graphviz_drawNode(gv, node_id, "<i>term</i>");
	
	/* Draw the binop node if we have two operands */
	const char* a_parent_id = node_id;
	char* binop_id = NULL;
	if(self->right != NULL) {
		const char* binop_str = self->divide ? "/" : "*";
		asprintf_ff(&binop_id, "%p(%s)", self, binop_str);
		a_parent_id = binop_id;
		Graphviz_drawNode(gv, binop_id, binop_str);
		Graphviz_drawEdge(gv, node_id, binop_id);
	}
	
	/* Draw factor a */
	char* a_id = ptos(self->left);
	AST_Factor_drawGraph(self->left, gv);
	Graphviz_drawEdge(gv, a_parent_id, a_id);
	destroy(&a_id);
	
	/* Draw term b (if it exists) */
	if(self->right != NULL) {
		char* b_id = ptos(self->right);
		AST_Term_drawGraph(self->right, gv);
		Graphviz_drawEdge(gv, binop_id, b_id);
		destroy(&b_id);
	}
	
	destroy(&binop_id);
	destroy(&node_id);
}

Destroyer(AST_Factor) {
	switch(self->type) {
		case FACT_IDENT:
			release(&self->value.ident);
			break;
		
		case FACT_NUMBER:
			release(&self->value.number);
			break;
		
		case FACT_EXPR:
			release(&self->value.expr);
			break;
		
		case FACT_CALL:
			release(&self->value.call);
			break;
		
		default:
			abort();
	}
}
DEF(AST_Factor);

void AST_Factor_drawGraph(AST_Factor* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>factor</i>");
	
	switch(self->type) {
		case FACT_IDENT:
			AST_Ident_drawGraph(self->value.ident, gv);
			Graphviz_drawPtrEdge(gv, self, self->value.ident);
			break;
		
		case FACT_NUMBER:
			AST_Number_drawGraph(self->value.number, gv);
			Graphviz_drawPtrEdge(gv, self, self->value.number);
			break;
		
		case FACT_EXPR:
			AST_Expr_drawGraph(self->value.expr, gv);
			Graphviz_drawPtrEdge(gv, self, self->value.expr);
			break;
		
		case FACT_CALL:
			AST_Call_drawGraph(self->value.call, gv);
			Graphviz_drawPtrEdge(gv, self, self->value.call);
			break;
		
		default:
			abort();
	}
}

Destroyer(AST_Number) {
	/* Nothing to do */
}
DEF(AST_Number);

void AST_Number_drawGraph(AST_Number* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<b>%u</b>", self->num);
}

Destroyer(AST_Ident) {
	destroy(&self->name);
}
DEF(AST_Ident);

void AST_Ident_drawGraph(AST_Ident* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<b>%s</b>", self->name);
}

Destroyer(AST_Call) {
	release(&self->ident);
	release(&self->param_list);
}
DEF(AST_Call);

void AST_Call_drawGraph(AST_Call* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<b>call</b>");
	AST_Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
	AST_ParamList_drawGraph(self->param_list, gv);
	Graphviz_drawPtrEdge(gv, self, self->param_list);
}

Destroyer(AST_ParamList) {
	size_t i;
	for(i = 0; i < self->param_count; i++) {
		release(&self->params[i]);
	}
	destroy(&self->params);
}
DEF(AST_ParamList);

void AST_ParamList_drawGraph(AST_ParamList* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>param-list</i>");
	
	size_t i;
	for(i = 0; i < self->param_count; i++) {
		AST_Expr_drawGraph(self->params[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->params[i]);
	}
}

/* Statements */
Destroyer(Stmt_Assign) {
	release(&self->ident);
	release(&self->value);
}
DEF(Stmt_Assign);

void Stmt_Assign_drawGraph(Stmt_Assign* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, ":=");
	AST_Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
	AST_Expr_drawGraph(self->value, gv);
	Graphviz_drawPtrEdge(gv, self, self->value);
}

Destroyer(Stmt_Call) {
	release(&self->ident);
	release(&self->param_list);
}
DEF(Stmt_Call);

void Stmt_Call_drawGraph(Stmt_Call* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "call");
	AST_Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
	
	if(self->param_list != NULL) {
		AST_ParamList_drawGraph(self->param_list, gv);
		Graphviz_drawPtrEdge(gv, self, self->param_list);
	}
}

Destroyer(Stmt_Begin) {
	size_t i;
	for(i = 0; i < self->stmt_count; i++) {
		release(&self->stmts[i]);
	}
	
	destroy(&self->stmts);
}
DEF(Stmt_Begin);

void Stmt_Begin_drawGraph(Stmt_Begin* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "begin");
	
	size_t i;
	for(i = 0; i < self->stmt_count; i++) {
		AST_Stmt_drawGraph(self->stmts[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->stmts[i]);
	}
}

Destroyer(Stmt_If) {
	release(&self->cond);
	release(&self->then_stmt);
	release(&self->else_stmt);
}
DEF(Stmt_If);

void Stmt_If_drawGraph(Stmt_If* self, Graphviz* gv) {
	char* node_id = ptos(self);
	Graphviz_drawNode(gv, node_id, "if");
	
	/* Draw cond */
	char* cond_id = ptos(self->cond);
	AST_Cond_drawGraph(self->cond, gv);
	Graphviz_drawEdge(gv, node_id, cond_id);
	destroy(&cond_id);
	
	/* Draw then node */
	char* then_id;
	asprintf_ff(&then_id, "%p(then)", self);
	Graphviz_drawNode(gv, then_id, "then");
	Graphviz_drawEdge(gv, node_id, then_id);
	
	/* Draw then statement */
	char* then_stmt_id = ptos(self->then_stmt);
	AST_Stmt_drawGraph(self->then_stmt, gv);
	Graphviz_drawEdge(gv, then_id, then_stmt_id);
	destroy(&then_stmt_id);
	destroy(&then_id);
	
	/* Else is optional */
	if(self->else_stmt != NULL) {
		/* Draw else node */
		char* else_id;
		asprintf_ff(&else_id, "%p(else)", self);
		Graphviz_drawNode(gv, else_id, "else");
		Graphviz_drawEdge(gv, node_id, else_id);
		
		/* Draw else statement */
		char* else_stmt_id = ptos(self->else_stmt);
		AST_Stmt_drawGraph(self->else_stmt, gv);
		Graphviz_drawEdge(gv, else_id, else_stmt_id);
		destroy(&else_stmt_id);
		destroy(&else_id);
	}
	
	destroy(&node_id);
}

Destroyer(Stmt_While) {
	release(&self->cond);
	release(&self->do_stmt);
}
DEF(Stmt_While);

void Stmt_While_drawGraph(Stmt_While* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "while");
	AST_Cond_drawGraph(self->cond, gv);
	Graphviz_drawPtrEdge(gv, self, self->cond);
	AST_Stmt_drawGraph(self->do_stmt, gv);
	Graphviz_drawPtrEdge(gv, self, self->do_stmt);
}

Destroyer(Stmt_Read) {
	release(&self->ident);
}
DEF(Stmt_Read);

void Stmt_Read_drawGraph(Stmt_Read* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "read");
	AST_Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
}

Destroyer(Stmt_Write) {
	release(&self->ident);
}
DEF(Stmt_Write);

void Stmt_Write_drawGraph(Stmt_Write* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "write");
	AST_Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
}
