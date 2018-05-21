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


Destroyer(AST_Block) {
	release(&self->consts);
	release(&self->vars);
	release(&self->procs);
	release(&self->stmt);
}
DEF(AST_Block);

Destroyer(AST_ConstDecls) {
	foreach(&self->consts, pconst) {
		destroy(&pconst->ident);
	}
	array_clear(&self->consts);
}
DEF(AST_ConstDecls);

Destroyer(AST_ParamDecls) {
	array_destroy(&self->params);
}
DEF(AST_ParamDecls);

Destroyer(AST_VarDecls) {
	foreach(&self->vars, pvar) {
		destroy(pvar);
	}
	array_clear(&self->vars);
}
DEF(AST_VarDecls);

Destroyer(AST_ProcDecls) {
	foreach(&self->procs, pproc) {
		release(pproc);
	}
	array_clear(&self->procs);
}
DEF(AST_ProcDecls);

Destroyer(AST_Proc) {
	destroy(&self->ident);
	release(&self->body);
}
DEF(AST_Proc);

Destroyer(AST_Stmt) {
	switch(self->type) {
		case STMT_UNINITIALIZED:
			break;
		
		case STMT_ASSIGN:
			destroy(&self->stmt.assign.ident);
			release(&self->stmt.assign.value);
			break;
		
		case STMT_CALL:
			destroy(&self->stmt.call.ident);
			release(&self->stmt.call.param_list);
			break;
		
		case STMT_BEGIN: {
			array_release(&self->stmt.begin.stmts);
			break;
		}
		
		case STMT_IF:
			release(&self->stmt.if_stmt.cond);
			release(&self->stmt.if_stmt.then_stmt);
			release(&self->stmt.if_stmt.else_stmt);
			break;
		
		case STMT_WHILE:
			release(&self->stmt.while_stmt.cond);
			release(&self->stmt.while_stmt.do_stmt);
			break;
		
		case STMT_READ:
			destroy(&self->stmt.read.ident);
			break;
		
		case STMT_WRITE:
			release(&self->stmt.write.value);
			break;
		
		default:
			ASSERT(!"Invalid statement type");
	}
}
DEF(AST_Stmt);

Destroyer(AST_Cond) {
	switch(self->type) {
		case COND_UNINITIALIZED:
			break;
		
		case COND_ODD:
			release(&self->values.operand);
			break;
		
		case COND_EQ:
		case COND_NE:
		case COND_LT:
		case COND_LE:
		case COND_GT:
		case COND_GE:
			release(&self->values.binop.left);
			release(&self->values.binop.right);
			break;
		
		default:
			ASSERT(!"Invalid condition type");
	}
}
DEF(AST_Cond);

Destroyer(AST_Expr) {
	switch(self->type) {
		case EXPR_UNINITIALIZED:
			break;
		
		case EXPR_VAR:
			destroy(&self->values.ident);
			break;
		
		case EXPR_NUM:
			break;
		
		case EXPR_NEG:
			release(&self->values.operand);
			break;
		
		case EXPR_ADD:
		case EXPR_SUB:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
			release(&self->values.binop.left);
			release(&self->values.binop.right);
			break;
		
		case EXPR_CALL:
			destroy(&self->values.call.ident);
			release(&self->values.call.param_list);
			break;
		
		default:
			ASSERT(!"Invalid expression type");
	}
}
DEF(AST_Expr);

Destroyer(AST_ParamList) {
	array_release(&self->params);
}
DEF(AST_ParamList);


/*! Create an AST node for a block with the provided child nodes
 @param consts List of constant declarations
 @param vars List of variable declarations
 @param procs List of procedure declarations
 @param stmt Body of the block
 @return AST node for a block
 */
AST_Block* AST_Block_create(AST_ConstDecls* consts, AST_VarDecls* vars, AST_ProcDecls* procs, AST_Stmt* stmt) {
	AST_Block* ret = AST_Block_new();
	ret->consts = consts;
	ret->vars = vars;
	ret->procs = procs;
	ret->stmt = stmt;
	return ret;
}

/*! Append a constant declaration to a list
 @param ident Name of constant
 @param value Value of constant
 @return self
 */
AST_ConstDecls* AST_ConstDecls_append(AST_ConstDecls* self, char* ident, Word value) {
	/* Allow self to be NULL by allocating a new instance */
	if(!self) {
		self = AST_ConstDecls_new();
	}
	
	/* Append value */
	element_type(self->consts) newConst = {
		.ident = ident,
		.value = value
	};
	array_append(&self->consts, newConst);
	return self;
}

/*! Append a variable declaration to a list
 @param ident Name of variable
 @return self
 */
AST_VarDecls* AST_VarDecls_append(AST_VarDecls* self, char* ident) {
	/* Allow self to be NULL by allocating a new instance */
	if(!self) {
		self = AST_VarDecls_new();
	}
	
	/* Append variable */
	array_append(&self->vars, ident);
	return self;
}

/*! Append a procedure declaration to a list
 @param ident Name of procedure
 @param param_decls List of parameter declarations
 @param body Block making up the procedure's body
 @return self
 */
AST_ProcDecls* AST_ProcDecls_append(AST_ProcDecls* self, char* ident, AST_ParamDecls* param_decls, AST_Block* body) {
	/* Allow self to be NULL by allocating a new instance */
	if(!self) {
		self = AST_ProcDecls_new();
	}
	
	/* Build proc object */
	AST_Proc* proc = AST_Proc_new();
	proc->ident = ident;
	proc->param_decls = param_decls;
	proc->body = body;
	
	/* Append procedure declaration object */
	array_append(&self->procs, proc);
	return self;
}

/*! Append a parameter declaration to a list
 @param ident Name of parameter
 @return self
 */
AST_ParamDecls* AST_ParamDecls_append(AST_ParamDecls* self, char* ident) {
	/* Allow self to be NULL by allocating a new instance */
	if(!self) {
		self = AST_ParamDecls_new();
	}
	
	/* Append parameter */
	array_append(&self->params, ident);
	return self;
}

/*! Create an AST node for a statement with the provided type and child nodes
 @param type Statement type
 @note Further parameters depend on the statement type
 @return AST node for a statement
 */
AST_Stmt* AST_Stmt_create(STMT_TYPE type, ...) {
	VARIADIC(type, ap, {
		AST_Stmt* ret = AST_Stmt_new();
		ret->type = type;
		
		switch(type) {
			case STMT_ASSIGN:
				VA_POP(ap, ret->stmt.assign.ident);
				VA_POP(ap, ret->stmt.assign.value);
				break;
			
			case STMT_CALL:
				VA_POP(ap, ret->stmt.call.ident);
				VA_POP(ap, ret->stmt.call.param_list);
				break;
			
			case STMT_BEGIN:
				/* Nothing to do */
				break;
			
			case STMT_IF:
				VA_POP(ap, ret->stmt.if_stmt.cond);
				VA_POP(ap, ret->stmt.if_stmt.then_stmt);
				VA_POP(ap, ret->stmt.if_stmt.else_stmt);
				break;
			
			case STMT_WHILE:
				VA_POP(ap, ret->stmt.while_stmt.cond);
				VA_POP(ap, ret->stmt.while_stmt.do_stmt);
				break;
			
			case STMT_READ:
				VA_POP(ap, ret->stmt.read.ident);
				break;
			
			case STMT_WRITE:
				VA_POP(ap, ret->stmt.write.value);
				break;
			
			default:
				ASSERT(!"Invalid statement type");
		}
		
		return ret;
	});
}

/*! Append a statement to a begin statement
 @param self Statement of type STMT_BEGIN (will be checked with an assertion in debug builds)
 @param stmt Statement to append to this begin statement
 @return self
 */
AST_Stmt* AST_Stmt_append(AST_Stmt* self, AST_Stmt* stmt) {
	/* Don't bother appending empty statements */
	if(!stmt) {
		return self;
	}
	
	/* Allow self to be NULL by allocating a new instance */
	if(!self) {
		self = AST_Stmt_new();
		self->type = STMT_BEGIN;
	}
	
	/* Must be a begin statement */
	ASSERT(self->type == STMT_BEGIN);
	
	/* Append statement to array */
	array_append(&self->stmt.begin.stmts, stmt);
	return self;
}

/*! Create an AST node for a condition with the provided type and child nodes
 @param type Condition type
 @note Further parameters depend on the condition type
 @return AST node for a condition
 */
AST_Cond* AST_Cond_create(COND_TYPE type, ...) {
	VARIADIC(type, ap, {
		AST_Cond* ret = AST_Cond_new();
		ret->type = type;
		
		switch(type) {
			case COND_ODD:
				VA_POP(ap, ret->values.operand);
				break;
			
			case COND_EQ:
			case COND_NE:
			case COND_LT:
			case COND_LE:
			case COND_GT:
			case COND_GE:
				VA_POP(ap, ret->values.binop.left);
				VA_POP(ap, ret->values.binop.right);
				break;
			
			default:
				ASSERT(!"Invalid condition type");
		}
		
		return ret;
	});
}

/*! Create an AST node for an expression with the provided type and child nodes
 @param type Expression type
 @note Further parameters depend on the expression type
 @return AST node for an expression
 */
AST_Expr* AST_Expr_create(EXPR_TYPE type, ...) {
	VARIADIC(type, ap, {
		AST_Expr* ret = AST_Expr_new();
		ret->type = type;
		
		switch(type) {
			case EXPR_VAR:
				VA_POP(ap, ret->values.ident);
				break;
			
			case EXPR_NUM:
				VA_POP(ap, ret->values.num);
				break;
			
			case EXPR_NEG:
				VA_POP(ap, ret->values.operand);
				break;
			
			case EXPR_ADD:
			case EXPR_SUB:
			case EXPR_MUL:
			case EXPR_DIV:
			case EXPR_MOD:
				VA_POP(ap, ret->values.binop.left);
				VA_POP(ap, ret->values.binop.right);
				break;
			
			case EXPR_CALL:
				VA_POP(ap, ret->values.call.ident);
				VA_POP(ap, ret->values.call.param_list);
				break;
			
			default:
				ASSERT(!"Invalid expression type");
		}
		
		return ret;
	});
}

/*! Applies a unary operator (+/-) to an expression
 @param unary_op Either EXPR_ADD (+) or EXPR_SUB (-)
 @return New EXPR_NEG object if unary_op was EXPR_SUB, otherwise just self
 */
AST_Expr* AST_Expr_applyUnaryOperator(AST_Expr* self, EXPR_TYPE unary_op) {
	switch(unary_op) {
		case EXPR_ADD:
			return self;
		
		case EXPR_SUB:
			return AST_Expr_create(EXPR_NEG, self);
		
		default:
			ASSERT(!"Unexpected expression type");
	}
}

/*! Append an expression to a parameter list
 @param expr Expression to compute the parameter value
 @return self
 */
AST_ParamList* AST_ParamList_append(AST_ParamList* self, AST_Expr* expr) {
	/* Allow self to be NULL by allocating a new instance */
	if(!self) {
		self = AST_ParamList_new();
	}
	
	/* Append expression to array */
	array_append(&self->params, expr);
	return self;
}
