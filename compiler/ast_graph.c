//
//  ast_graph.c
//  PL/0
//
//  Created by Kevin Colley on 2/13/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#include "ast_graph.h"
#include "config.h"


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
	Graphviz_drawPtrNode(gv, self, "<font " FACE_NONTERMINAL ">block</font>");
	
	if(self->consts != NULL) {
		AST_ConstDecls_drawGraph(self->consts, gv);
		Graphviz_drawPtrEdge(gv, self, self->consts);
	}
	
	if(self->vars != NULL) {
		AST_VarDecls_drawGraph(self->vars, gv);
		Graphviz_drawPtrEdge(gv, self, self->vars);
	}
	
	if(self->procs != NULL && self->procs->procs.count > 0) {
		AST_ProcDecls_drawGraph(self->procs, gv);
		Graphviz_drawPtrEdge(gv, self, self->procs);
	}
	
	if(self->stmt != NULL) {
		AST_Stmt_drawGraph(self->stmt, gv);
		Graphviz_drawPtrEdge(gv, self, self->stmt);
	}
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
	Graphviz_drawNode(gv, node_id, "<font " FACE_NONTERMINAL ">const-decls</font>");
	
	foreach(&self->consts, pconst) {
		char* const_id = rsprintf_ff("%p", pconst);
		
		Graphviz_drawNode(gv, const_id, "<font " FACE_TERMINAL ">=</font>");
		Graphviz_drawEdge(gv, node_id, const_id);
		
		char* name_id = ptos(pconst->ident);
		Ident_drawGraph(pconst->ident, gv);
		
		Graphviz_drawEdge(gv, const_id, name_id);
		destroy(&name_id);
		
		char* value_id = ptos(&pconst->value);
		Number_drawGraph(&pconst->value, gv);
		
		Graphviz_drawEdge(gv, const_id, value_id);
		destroy(&value_id);
		destroy(&const_id);
	}
	
	destroy(&node_id);
}

void AST_ParamDecls_drawGraph(AST_ParamDecls* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<font " FACE_NONTERMINAL ">parameter-block</font>");
	
	foreach(&self->params, pparam) {
		Ident_drawGraph(*pparam, gv);
		Graphviz_drawPtrEdge(gv, self, *pparam);
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
	Graphviz_drawPtrNode(gv, self, "<font " FACE_NONTERMINAL ">var-decls</font>");
	
	foreach(&self->vars, pvar) {
		Ident_drawGraph(*pvar, gv);
		Graphviz_drawPtrEdge(gv, self, *pvar);
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
	Graphviz_drawPtrNode(gv, self, "<font " FACE_NONTERMINAL ">proc-decls</font>");
	
	foreach(&self->procs, pproc) {
		AST_Proc_drawGraph(*pproc, gv);
		Graphviz_drawPtrEdge(gv, self, *pproc);
	}
}

/*! Example:
 
 @code
 procedure A;
 block...;
 @endcode
 
 @code
   +-----------+
   |procedure A|
   +-----------+
         |
       block
 @endcode
 */
void AST_Proc_drawGraph(AST_Proc* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self,
		"<font " FACE_TERMINAL ">procedure <font " COLOR_PROC ">%s</font></font>",
		self->ident);
	
	/* param-decls */
	if(self->param_decls != NULL && self->param_decls->params.count > 0) {
		AST_ParamDecls_drawGraph(self->param_decls, gv);
		Graphviz_drawPtrEdge(gv, self, self->param_decls);
	}
	
	/* block */
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
			ASSERT(!"Invalid statement type");
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
	/* Get HTML-safe label for conditional operator */
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
			ASSERT(!"Invalid condition type");
	}
	
	/* Draw relational operator node */
	Graphviz_drawPtrNode(gv, self, "<font " FACE_TERMINAL ">%s</font>", op_str);
	
	switch(self->type) {
		case COND_ODD: {
			/* Draw expression */
			AST_Expr_drawGraph(self->values.operand, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.operand);
			break;
		}
		
		case COND_EQ:
		case COND_NE:
		case COND_LT:
		case COND_LE:
		case COND_GT:
		case COND_GE: {
			/* Draw expression a */
			AST_Expr_drawGraph(self->values.binop.left, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.binop.left);
			
			/* Draw expression b */
			AST_Expr_drawGraph(self->values.binop.right, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.binop.right);
			break;
		}
		
		default:
			ASSERT(!"Invalid condition type");
	}
}

void AST_Expr_drawGraph(AST_Expr* self, Graphviz* gv) {
	switch(self->type) {
		case EXPR_VAR:
			Graphviz_drawPtrNode(gv, self,
				"<font " FACE_TERMINAL " " COLOR_VAR ">%s</font>",
				self->values.ident);
			break;
		
		case EXPR_NUM:
			Graphviz_drawPtrNode(gv, self,
				"<font " FACE_TERMINAL " " COLOR_NUM ">%d</font>",
				self->values.num);
			break;
		
		case EXPR_NEG:
			/* Draw unary operator */
			Graphviz_drawPtrNode(gv, self, "<font " FACE_TERMINAL ">-</font>");
			
			/* Draw operand */
			AST_Expr_drawGraph(self->values.operand, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.operand);
			break;
		
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV: {
			/* Draw binary operator */
			const char* op_str;
			switch(self->type) {
				case EXPR_ADD: op_str = "+"; break;
				case EXPR_SUB: op_str = "-"; break;
				case EXPR_MUL: op_str = "*"; break;
				case EXPR_DIV: op_str = "/"; break;
				
				default:
					ASSERT(!"Invalid expression type");
			}
			Graphviz_drawPtrNode(gv, self,
				"<font " FACE_TERMINAL ">%s</font>", op_str);
			
			/* Draw expr a */
			AST_Expr_drawGraph(self->values.binop.left, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.binop.left);
			
			/* Draw expr b */
			AST_Expr_drawGraph(self->values.binop.right, gv);
			Graphviz_drawPtrEdge(gv, self, self->values.binop.right);
			break;
		}
		
		case EXPR_CALL: {
			/* Draw procedure name */
			Graphviz_drawPtrNode(gv, self,
				"<font " FACE_TERMINAL ">call <font " COLOR_PROC ">%s</font></font>",
				self->values.call.ident);
			
			/* Draw each parameter from the list as a direct child */
			AST_ParamList* param_list = self->values.call.param_list;
			if(param_list != NULL) {
				foreach(&param_list->params, pparam) {
					AST_Expr_drawGraph(*pparam, gv);
					Graphviz_drawPtrEdge(gv, self, *pparam);
				}
			}
			break;
		}
		
		default:
			ASSERT(!"Invalid expression type");
	}
}

static void Ident_drawGraph(char* ident, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, ident,
		"<font " FACE_TERMINAL " " COLOR_VAR ">%s</font>",
		ident);
}

static void Number_drawGraph(Word* pvalue, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, pvalue,
		"<font " FACE_TERMINAL " " COLOR_NUM ">%u</font>",
		*pvalue);
}

static void Stmt_Assign_drawGraph(AST_Stmt* self, Graphviz* gv) {
	/* Draw assignment node */
	Graphviz_drawPtrNode(gv, self, "<font " FACE_TERMINAL ">:=</font>");
	
	/* Draw identifier */
	Ident_drawGraph(self->stmt.assign.ident, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt.assign.ident);
	
	/* Draw expression */
	AST_Expr_drawGraph(self->stmt.assign.value, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt.assign.value);
}

static void Stmt_Call_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self,
		"<font " FACE_TERMINAL ">call <font " COLOR_PROC ">%s</font></font>",
		self->stmt.call.ident);
	
	/* Draw each parameter from the list as a direct child */
	AST_ParamList* param_list = self->stmt.call.param_list;
	if(param_list != NULL) {
		foreach(&param_list->params, pparam) {
			AST_Expr_drawGraph(*pparam, gv);
			Graphviz_drawPtrEdge(gv, self, *pparam);
		}
	}
}

static void Stmt_Begin_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<font " FACE_TERMINAL ">begin</font>");
	
	foreach(&self->stmt.begin.stmts, pstmt) {
		if(*pstmt != NULL) {
			AST_Stmt_drawGraph(*pstmt, gv);
			Graphviz_drawPtrEdge(gv, self, *pstmt);
		}
	}
}

static void Stmt_If_drawGraph(AST_Stmt* self, Graphviz* gv) {
	/* Draw if node */
	char* node_id = ptos(self);
	Graphviz_drawPtrNode(gv, self, "<font " FACE_TERMINAL ">if</font>");
	
	/* Draw cond */
	char* cond_id = ptos(self->stmt.if_stmt.cond);
	AST_Cond_drawGraph(self->stmt.if_stmt.cond, gv);
	Graphviz_drawEdge(gv, node_id, cond_id);
	destroy(&cond_id);
	
	/* Draw then node */
	char* then_id = rsprintf_ff("%p(then)", self);
	Graphviz_drawNode(gv, then_id, "<font " FACE_TERMINAL ">then</font>");
	Graphviz_drawEdge(gv, node_id, then_id);
	
	/* Draw then statement */
	AST_Stmt* then_stmt = self->stmt.if_stmt.then_stmt;
	if(then_stmt != NULL) {
		char* then_stmt_id = ptos(then_stmt);
		AST_Stmt_drawGraph(then_stmt, gv);
		Graphviz_drawEdge(gv, then_id, then_stmt_id);
		destroy(&then_stmt_id);
		destroy(&then_id);
	}
	
	/* Does this if statement have an else branch? */
	AST_Stmt* else_stmt = self->stmt.if_stmt.else_stmt;
	if(else_stmt != NULL) {
		/* Draw else node */
		char* else_id = rsprintf_ff("%p(else)", self);
		Graphviz_drawNode(gv, else_id, "<font " FACE_TERMINAL ">else</font>");
		Graphviz_drawEdge(gv, node_id, else_id);
		
		/* Draw else statement */
		char* else_stmt_id = ptos(else_stmt);
		AST_Stmt_drawGraph(else_stmt, gv);
		Graphviz_drawEdge(gv, else_id, else_stmt_id);
		destroy(&else_stmt_id);
		destroy(&else_id);
	}
	
	destroy(&node_id);
}

static void Stmt_While_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self, "<font " FACE_TERMINAL ">while</font>");
	
	/* Draw while condition */
	AST_Cond_drawGraph(self->stmt.while_stmt.cond, gv);
	Graphviz_drawPtrEdge(gv, self, self->stmt.while_stmt.cond);
	
	/* Does this while statement have a non-null do statement? */
	AST_Stmt* do_stmt = self->stmt.while_stmt.do_stmt;
	if(do_stmt != NULL) {
		AST_Stmt_drawGraph(do_stmt, gv);
		Graphviz_drawPtrEdge(gv, self, do_stmt);
	}
}

static void Stmt_Read_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self,
		"<font " FACE_TERMINAL ">read <font " COLOR_VAR ">%s</font></font>",
		self->stmt.read.ident);
}

static void Stmt_Write_drawGraph(AST_Stmt* self, Graphviz* gv) {
	Graphviz_drawPtrNode(gv, self,
		"<font " FACE_TERMINAL ">write <font " COLOR_VAR ">%s</font></font>",
		self->stmt.write.ident);
}
