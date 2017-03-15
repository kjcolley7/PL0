//
//  ast_graph.c
//  PL/0
//
//  Created by Kevin Colley on 2/13/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#include "ast_graph.h"


/* Forward declarations */
static void Ident_drawGraph(char* ident, Graphviz* gv);
static void Number_drawGraph(Word* pvalue, Graphviz* gv);
static void Stmt_Assign_drawGraph(AST_Stmt* self, Graphviz* gv);
static void Stmt_Call_drawGraph(AST_Stmt* self, Graphviz* gv);
static void Stmt_Begin_drawGraph(AST_Stmt* self, Graphviz* gv);
static void Stmt_If_drawGraph(AST_Stmt* self, Graphviz* gv);
static void Stmt_While_drawGraph(AST_Stmt* self, Graphviz* gv);
static void Stmt_Read_drawGraph(AST_Stmt* self, Graphviz* gv);
static void Stmt_Write_drawGraph(AST_Stmt* self, Graphviz* gv);


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
		char* const_id = rsprintf_ff("%p[%zu]", self, i);
		
		Graphviz_drawNode(gv, const_id, "=");
		Graphviz_drawEdge(gv, node_id, const_id);
		
		char* name_id = ptos(self->idents[i]);
		Ident_drawGraph(self->idents[i], gv);
		
		Graphviz_drawEdge(gv, const_id, name_id);
		destroy(&name_id);
		
		char* value_id = ptos(&self->values[i]);
		Number_drawGraph(&self->values[i], gv);
		
		Graphviz_drawEdge(gv, const_id, value_id);
		destroy(&value_id);
		destroy(&const_id);
	}
	
	destroy(&node_id);
}

void AST_ParamDecls_drawGraph(AST_ParamDecls* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<i>parameter-block</i>");
	
	size_t i;
	for(i = 0; i < self->param_count; i++) {
		Ident_drawGraph(self->params[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->params[i]);
	}
}

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
		Ident_drawGraph(self->vars[i], gv);
		Graphviz_drawPtrEdge(gv, self, self->vars[i]);
	}
}

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
	Ident_drawGraph(self->ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->ident);
	AST_ParamDecls_drawGraph(self->param_decls, gv);
	Graphviz_drawPtrEdge(gv, self, self->param_decls);
	AST_Block_drawGraph(self->body, gv);
	Graphviz_drawPtrEdge(gv, self, self->body);
}

void AST_Stmt_drawGraph(AST_Stmt* self, Graphviz* gv) {
	if(!self) {
		return;
	}
	
	/* Draw the actual statement nodes and then add an edge to the subtree */
	switch(self->type) {
		case STMT_ASSIGN:
			Stmt_Assign_drawGraph(self, gv);
			break;
			
		case STMT_CALL:
			Stmt_Call_drawGraph(self, gv);
			break;
			
		case STMT_BEGIN:
			Stmt_Begin_drawGraph(self, gv);
			break;
			
		case STMT_IF:
			Stmt_If_drawGraph(self, gv);
			break;
			
		case STMT_WHILE:
			Stmt_While_drawGraph(self, gv);
			break;
			
		case STMT_READ:
			Stmt_Read_drawGraph(self, gv);
			break;
			
		case STMT_WRITE:
			Stmt_Write_drawGraph(self, gv);
			break;
			
		default:
			assert(false);
	}
}

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
		case COND_ODD: op_str = "odd";      break;
		case COND_EQ:  op_str = "=";        break;
		case COND_GE:  op_str = "&gt;=";    break;
		case COND_GT:  op_str = "&gt;";     break;
		case COND_LE:  op_str = "&lt;=";    break;
		case COND_LT:  op_str = "&lt;";     break;
		case COND_NE:  op_str = "&lt;&gt;"; break;
		default:
			assert(false);
	}
	
	/* Draw relational operator node */
	char* op_id = rsprintf_ff("%p(%s)", self, op_str);
	Graphviz_drawNode(gv, op_id, op_str);
	Graphviz_drawEdge(gv, node_id, op_id);
	
	switch(self->type) {
		case COND_ODD: {
			/* Draw expression */
			char* operand_id = ptos(self->values.operand);
			AST_Expr_drawGraph(self->values.operand, gv);
			Graphviz_drawEdge(gv, op_id, operand_id);
			destroy(&operand_id);
			break;
		}
		
		case COND_EQ:
		case COND_NE:
		case COND_LT:
		case COND_LE:
		case COND_GT:
		case COND_GE: {
			/* Draw expression a */
			char* a_id = ptos(self->values.binop.left);
			AST_Expr_drawGraph(self->values.binop.left, gv);
			Graphviz_drawEdge(gv, op_id, a_id);
			destroy(&a_id);
			
			/* Draw expression b */
			char* b_id = ptos(self->values.binop.right);
			AST_Expr_drawGraph(self->values.binop.right, gv);
			Graphviz_drawEdge(gv, op_id, b_id);
			destroy(&b_id);
			break;
		}
		
		default:
			assert(false);
	}
	
	destroy(&op_id);
	destroy(&node_id);
}

void AST_Expr_drawGraph(AST_Expr* self, Graphviz* gv) {
	switch(self->type) {
		case EXPR_VAR:
			Graphviz_drawPtrNode(gv, self, "<b>%s</b>", self->values.ident);
			break;
		
		case EXPR_NUM:
			Graphviz_drawPtrNode(gv, self, "<b>%d</b>", self->values.num);
			break;
		
		case EXPR_NEG:
			/* Draw unary operator */
			Graphviz_drawPtrNode(gv, self, "-");
			
			/* Draw operand */
			AST_Expr_drawGraph(self->values.operand, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.operand);
			break;
		
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
			/* Draw binary operator */
			switch(self->type) {
				case EXPR_ADD: Graphviz_drawPtrNode(gv, self, "+"); break;
				case EXPR_SUB: Graphviz_drawPtrNode(gv, self, "-"); break;
				case EXPR_MUL: Graphviz_drawPtrNode(gv, self, "*"); break;
				case EXPR_DIV: Graphviz_drawPtrNode(gv, self, "/"); break;
				default: assert(false);
			}
			
			/* Draw expr a */
			AST_Expr_drawGraph(self->values.binop.left, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.binop.left);
			
			/* Draw expr b */
			AST_Expr_drawGraph(self->values.binop.right, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.binop.right);
			break;
		
		case EXPR_CALL: {
			/* Draw procedure name */
			Graphviz_drawPtrNode(gv, self, "call <b>%s</b>", self->values.call.ident);
			
			/* Draw each parameter from the list as a direct child */
			AST_ParamList* param_list = self->values.call.param_list;
			if(param_list != NULL) {
				size_t i;
				for(i = 0; i < param_list->param_count; i++) {
					AST_Expr_drawGraph(param_list->params[i], gv);
					Graphviz_drawPtrEdge(gv, self, param_list->params[i]);
				}
			}
			break;
		}
		
		default:
			assert(false);
	}
}

static void Ident_drawGraph(char* ident, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, ident, "<b>%s</b>", ident);
}

static void Number_drawGraph(Word* pvalue, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, pvalue, "<b>%u</b>", *pvalue);
}

static void Stmt_Assign_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<b>%s</b> :=", self->stmt.assign.ident);
	AST_Expr_drawGraph(self->stmt.assign.value, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt.assign.value);
}

static void Stmt_Call_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "call <b>%s</b>", self->stmt.call.ident);
	
	/* Draw each parameter from the list as a direct child */
	AST_ParamList* param_list = self->stmt.call.param_list;
	if(param_list != NULL) {
		size_t i;
		for(i = 0; i < param_list->param_count; i++) {
			AST_Expr_drawGraph(param_list->params[i], gv);
			Graphviz_drawPtrEdge(gv, self, param_list->params[i]);
		}
	}
}

static void Stmt_Begin_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "begin");
	
	size_t i;
	for(i = 0; i < self->stmt.begin.stmt_count; i++) {
		AST_Stmt* stmt = self->stmt.begin.stmts[i];
		if(stmt != NULL) {
			AST_Stmt_drawGraph(stmt, gv);
			Graphviz_drawPtrEdge(gv, self, stmt);
		}
	}
}

static void Stmt_If_drawGraph(AST_Stmt* self, Graphviz* gv) {
	char* node_id = ptos(self);
	Graphviz_drawPtrNode(gv, self, "if");
	
	/* Draw cond */
	char* cond_id = ptos(self->stmt.if_stmt.cond);
	AST_Cond_drawGraph(self->stmt.if_stmt.cond, gv);
	Graphviz_drawEdge(gv, node_id, cond_id);
	destroy(&cond_id);
	
	/* Draw then node */
	char* then_id = rsprintf_ff("%p(then)", self);
	Graphviz_drawNode(gv, then_id, "then");
	Graphviz_drawEdge(gv, node_id, then_id);
	
	/* Draw then statement */
	char* then_stmt_id = ptos(self->stmt.if_stmt.body);
	AST_Stmt_drawGraph(self->stmt.if_stmt.body, gv);
	Graphviz_drawEdge(gv, then_id, then_stmt_id);
	destroy(&then_stmt_id);
	destroy(&then_id);
	destroy(&node_id);
}

static void Stmt_While_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "while");
	AST_Cond_drawGraph(self->stmt.while_stmt.cond, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt.while_stmt.cond);
	AST_Stmt_drawGraph(self->stmt.while_stmt.body, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt.while_stmt.body);
}

static void Stmt_Read_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "read <b>%s</b>", self->stmt.read.ident);
}

static void Stmt_Write_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "write <b>%s</b>", self->stmt.write.ident);
}
