//
//  codegen.c
//  PL/0
//
//  Created by Kevin Colley on 11/9/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "codegen.h"
#include <stdarg.h>


static bool genStmt(Block* scope, BasicBlock** code, AST_Stmt* statement);
static bool genCond(Block* scope, BasicBlock** code, AST_Cond* condition);
static bool genExpr(Block* scope, BasicBlock** code, AST_Expr* expression);
static bool genTerm(Block* scope, BasicBlock** code, AST_Term* term);
static bool genFactor(Block* scope, BasicBlock** code, AST_Factor* factor);
static bool genNumber(Block* scope, BasicBlock** code, AST_Number* number);
static bool genCall(Block* scope, BasicBlock** code, AST_Call* call);
static bool genParamList(Block* scope, BasicBlock** code, AST_ParamList* param_list);
static bool genStmtAssign(Block* scope, BasicBlock** code, Stmt_Assign* assign_statement);
static bool genStmtCall(Block* scope, BasicBlock** code, Stmt_Call* call_statement);
static bool genStmtBegin(Block* scope, BasicBlock** code, Stmt_Begin* begin_statement);
static bool genStmtIf(Block* scope, BasicBlock** code, Stmt_If* if_statement);
static bool genStmtWhile(Block* scope, BasicBlock** code, Stmt_While* while_statement);
static bool genStmtRead(Block* scope, BasicBlock** code, Stmt_Read* read_statement);
static bool genStmtWrite(Block* scope, BasicBlock** code, Stmt_Write* write_statement);
static bool genLoadIdent(Block* scope, BasicBlock** code, AST_Ident* ident);
static bool genStoreVar(Block* scope, BasicBlock** code, AST_Ident* ident);


static void vSemanticError(const char* fmt, va_list ap) {
	printf("Semantic Error: ");
	vprintf(fmt, ap);
	printf("\n");
}

static void semanticError(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vSemanticError(fmt, ap);
	va_end(ap);
}


Destroyer(Codegen) {
	release(&self->block);
}
DEF(Codegen);

Codegen* Codegen_initWithAST(Codegen* self, AST_Program* prog) {
	if((self = Codegen_init(self))) {
		/* Build the symbol tree */
		SymTree* scope = SymTree_initWithAST(SymTree_alloc(), NULL, prog->block, 0);
		if(scope == NULL) {
			/* Codegen error occurred, so destroy self and return NULL */
			release(&self);
			return NULL;
		}
		
		/* Initialize the block */
		self->block = Block_initWithScope(Block_alloc(), scope);
		if(self->block == NULL) {
			/* Codegen error occurred, so destroy self and return NULL */
			release(&self);
			return NULL;
		}
		
		/* Generate code for the block */
		if(!Block_generate(self->block, prog->block)) {
			/* Codegen error occurred, so destroy self and return NULL */
			release(&self);
			return NULL;
		}
	}
	
	return self;
}

void Codegen_layoutCode(Codegen* self) {
	/* Recursively set the addresses of all referenced code and resolve undefined symbols */
	Block_setAddress(self->block, 0);
}

void Codegen_optimize(Codegen* self) {
	/* Breaks apart the chain of blocks so it can be recreated later */
	Block* cur = self->block;
	while(cur != NULL) {
		Block* next = cur->next;
		
		/* Remove cur from the linked list */
		cur->next = cur->last = NULL;
		cur->code_length = 0;
		
		/* Set the address of every basic block to ADDR_UND */
		BasicBlock* bb = cur->code;
		while(bb != NULL) {
			bb->code_addr = ADDR_UND;
			bb = bb->next;
		}
		
		cur = next;
	}
	
	/* Recursively optimizes the code graphs */
	Block_optimize(self->block);
}

void Codegen_drawGraph(Codegen* self, FILE* fp) {
	Graphviz* gv = Graphviz_initWithFile(Graphviz_alloc(), fp, "program");
	Graphviz_draw(gv, "bgcolor=azure;");
	Block_drawGraph(self->block, gv);
	release(&gv);
}

void Codegen_writeSymbolTable(Codegen* self, FILE* fp) {
	fprintf(fp, "Name\tType\tLevel\tValue\n");
	SymTree_write(self->block->symtree, fp);
}

void Codegen_emit(Codegen* self, FILE* fp) {
	/* The top level code starts at address zero */
	Block_emit(self->block, fp);
}

bool Codegen_genBlock(Block* scope, AST_Block* ast) {
	BasicBlock* entrypoint = BasicBlock_new();
	BasicBlock* cur = entrypoint;
	
	/* Produce an INC if we need to modify the stack space */
	if(scope->symtree->frame_size != 0) {
		BasicBlock_addInsn(cur, MAKE_INC(scope->symtree->frame_size));
	}
	
	/* Generate code for the statements contained in the block */
	bool success = genStmt(scope, &cur, ast->stmt);
	if(!success) {
		release(&entrypoint);
		return false;
	}
	
	scope->code = entrypoint;
	return true;
}

static bool genStmt(Block* scope, BasicBlock** code, AST_Stmt* statement) {
	switch(statement->type) {
		case STMT_ASSIGN: return genStmtAssign(scope, code, statement->stmt.assign);
		case STMT_CALL:   return genStmtCall(scope, code, statement->stmt.call);
		case STMT_BEGIN:  return genStmtBegin(scope, code, statement->stmt.begin);
		case STMT_IF:     return genStmtIf(scope, code, statement->stmt.if_stmt);
		case STMT_WHILE:  return genStmtWhile(scope, code, statement->stmt.while_stmt);
		case STMT_READ:   return genStmtRead(scope, code, statement->stmt.read);
		case STMT_WRITE:  return genStmtWrite(scope, code, statement->stmt.write);
		case STMT_EMPTY:  return true;
		default: abort();
	}
}

static bool genCond(Block* scope, BasicBlock** code, AST_Cond* condition) {
	/* Store the start of the condition in the basic block for optimizations later */
	BasicBlock_markCondition(*code);
	
	/* Create code for the first operand of the conditional operator */
	bool success = genExpr(scope, code, condition->left);
	if(!success) {
		return false;
	}
	
	/* The odd condition only has one operand */
	if(condition->type == COND_ODD) {
		BasicBlock_addInsn(*code, MAKE_ODD());
		return true;
	}
	
	/* All other conditions have a second operand */
	success = genExpr(scope, code, condition->right);
	if(!success) {
		return false;
	}
	
	/* Output the comparison instruction */
	switch(condition->type) {
		case COND_EQ:  BasicBlock_addInsn(*code, MAKE_EQL()); break;
		case COND_NEQ: BasicBlock_addInsn(*code, MAKE_NEQ()); break;
		case COND_LT:  BasicBlock_addInsn(*code, MAKE_LSS()); break;
		case COND_LE:  BasicBlock_addInsn(*code, MAKE_LEQ()); break;
		case COND_GT:  BasicBlock_addInsn(*code, MAKE_GTR()); break;
		case COND_GE:  BasicBlock_addInsn(*code, MAKE_GEQ()); break;
		default: abort();
	}
	
	return true;
}

static bool genExpr(Block* scope, BasicBlock** code, AST_Expr* expression) {
	/* Generate code to produce the left operand of the expression */
	bool success = genTerm(scope, code, expression->left);
	if(!success) {
		return false;
	}
	
	/* Left operand can optionally be negated */
	if(expression->a_negative) {
		BasicBlock_addInsn(*code, MAKE_NEG());
	}
	
	/* Right operand is optional */
	if(expression->right != NULL) {
		/* Generate code to produce the right operand of the expression */
		success = genExpr(scope, code, expression->right);
		if(!success) {
			return false;
		}
		
		/* Create subtraction or addition instruction */
		if(expression->subtract) {
			BasicBlock_addInsn(*code, MAKE_SUB());
		}
		else {
			BasicBlock_addInsn(*code, MAKE_ADD());
		}
	}
	
	return true;
}

static bool genTerm(Block* scope, BasicBlock** code, AST_Term* term) {
	/* Generate code to produce the left operand of the term */
	bool success = genFactor(scope, code, term->left);
	if(!success) {
		return false;
	}
	
	/* Right operand is optional */
	if(term->right != NULL) {
		/* Generate code to produce the right operand of the term */
		success = genTerm(scope, code, term->right);
		if(!success) {
			return false;
		}
		
		/* Create division or multiplication instruction */
		if(term->divide) {
			BasicBlock_addInsn(*code, MAKE_DIV());
		}
		else {
			BasicBlock_addInsn(*code, MAKE_MUL());
		}
	}
	
	return true;
}

static bool genFactor(Block* scope, BasicBlock** code, AST_Factor* factor) {
	/* Generate code to produce the factor's value based on its type */
	switch(factor->type) {
		case FACT_NUMBER: return genNumber(scope, code, factor->value.number);
		case FACT_IDENT:  return genLoadIdent(scope, code, factor->value.ident);
		case FACT_EXPR:   return genExpr(scope, code, factor->value.expr);
		case FACT_CALL:   return genCall(scope, code, factor->value.call);
		default: abort();
	}
}

static bool genNumber(Block* scope, BasicBlock** code, AST_Number* number) {
	/* The scope variable is not necessary, so it is ignored */
	(void)scope;
	BasicBlock_addInsn(*code, MAKE_LIT(number->num));
	return true;
}

static bool genCall(Block* scope, BasicBlock** code, AST_Call* call) {
	/* Lookup the procedure symbol by name */
	const char* name = call->ident->name;
	Symbol* sym = SymTree_findSymbol(scope->symtree, name);
	if(sym == NULL) {
		semanticError("Tried to call procedure \"%s\" which isn't declared at this scope", name);
		return false;
	}
	
	/* Generate code to evaluate the parameters and place them where they need to be */
	genParamList(scope, code, call->param_list);
	
	/* Remember to resolve this reference later */
	BasicBlock_markSymbol(*code, sym);
	
	/* Generate the call instruction (target will be set during symbol resolution) */
	BasicBlock_addInsn(*code, MAKE_CAL(0, ADDR_UND));
	
	/* Adjust the stack pointer by one so the procedure's result is on the stack */
	BasicBlock_addInsn(*code, MAKE_INC(1));
	return true;
}

static bool genParamList(Block* scope, BasicBlock** code, AST_ParamList* param_list) {
	/* No need to adjust the stack at all if the parameter list is empty */
	if(param_list->param_count == 0) {
		return true;
	}
	
	/* Need to adjust stack pointer up 4 so the parameters go in the right place */
	BasicBlock_addInsn(*code, MAKE_INC(4));
	
	/* Generate the code to produce the value of all parameters */
	size_t i;
	for(i = 0; i < param_list->param_count; i++) {
		bool success = genExpr(scope, code, param_list->params[i]);
		if(!success) {
			return success;
		}
	}
	
	/* Adjust the stack pointer back to where it was before we first adjusted it */
	BasicBlock_addInsn(*code, MAKE_INC(-(4 + (Word)param_list->param_count)));
	return true;
}

static bool genStmtAssign(Block* scope, BasicBlock** code, Stmt_Assign* assign_statement) {
	/* Generate code to produce the value to store into the variable */
	bool success = genExpr(scope, code, assign_statement->value);
	if(!success) {
		return false;
	}
	
	/* Generate code to store the expression result into the variable */
	return genStoreVar(scope, code, assign_statement->ident);
}

static bool genStmtCall(Block* scope, BasicBlock** code, Stmt_Call* call_statement) {
	/* Lookup the procedure symbol by name */
	const char* name = call_statement->ident->name;
	Symbol* sym = SymTree_findSymbol(scope->symtree, name);
	if(sym == NULL) {
		semanticError("Tried to call procedure \"%s\" which isn't declared at this scope", name);
		return false;
	}
	
	/* Are there parameters given? */
	if(call_statement->param_list != NULL) {
		/* Generate code to evaluate the parameters and place them where they need to be */
		genParamList(scope, code, call_statement->param_list);
	}
	
	/* Remember to resolve this reference later */
	BasicBlock_markSymbol(*code, sym);
	
	/* Generate the call instruction (target will be set during symbol resolution) */
	BasicBlock_addInsn(*code, MAKE_CAL(0, ADDR_UND));
	return true;
}

static bool genStmtBegin(Block* scope, BasicBlock** code, Stmt_Begin* begin_statement) {
	size_t i;
	for(i = 0; i < begin_statement->stmt_count; i++) {
		/* Generate the code for each statement */
		bool success = genStmt(scope, code, begin_statement->stmts[i]);
		if(!success) {
			return false;
		}
	}
	
	return true;
}

static bool genStmtIf(Block* scope, BasicBlock** code, Stmt_If* if_statement) {
	/* Generate the code to compute the condition */
	bool success = genCond(scope, code, if_statement->cond);
	if(!success) {
		return false;
	}
	
	/* Remember the basic block for the condition */
	BasicBlock* cond = *code;
	
	/* Create an empty basic block to hold the code when the condition is true */
	BasicBlock* true_branch_begin = BasicBlock_createNext(code);
	BasicBlock_setTarget(cond, true_branch_begin);
	
	/* Generate code for the then statement of the if statement */
	success = genStmt(scope, code, if_statement->then_stmt);
	if(!success) {
		return false;
	}
	
	/* Remember the last basic block of the true branch */
	BasicBlock* true_branch_end = *code;
	
	/* Create an empty basic block to hold the code when the condition is false */
	BasicBlock* false_branch_begin = BasicBlock_createNext(code);
	BasicBlock_setFalseTarget(cond, false_branch_begin);
	
	/* Does this if statement have an else branch to it? */
	if(if_statement->else_stmt == NULL) {
		/* There is no else statement, so make the code from the true branch rejoin the false branch */
		BasicBlock_setTarget(true_branch_end, false_branch_begin);
	}
	else {
		/* Generate code for the else statement of the if statement */
		success = genStmt(scope, code, if_statement->else_stmt);
		if(!success) {
			return false;
		}
		
		/* Remember the end of the false branch */
		BasicBlock* false_branch_end = *code;
		
		/* Create an empty basic block for both branches to rejoin into */
		BasicBlock* endif = BasicBlock_createNext(code);
		BasicBlock_setTarget(true_branch_end, endif);
		BasicBlock_setTarget(false_branch_end, endif);
	}
	
	return true;
}

static bool genStmtWhile(Block* scope, BasicBlock** code, Stmt_While* while_statement) {
	BasicBlock* before_cond = *code;
	
	/* Create a new basic block for the condition */
	BasicBlock* cond = BasicBlock_createNext(code);
	BasicBlock_setTarget(before_cond, cond);
	
	/* Generate code for the condition of the while statement */
	bool success = genCond(scope, code, while_statement->cond);
	if(!success) {
		return false;
	}
	
	/* Create a new basic block for the loop body */
	BasicBlock* loop_body_begin = BasicBlock_createNext(code);
	BasicBlock_setTarget(cond, loop_body_begin);
	
	/* Generate code for the body of the while statement */
	success = genStmt(scope, code, while_statement->do_stmt);
	if(!success) {
		return false;
	}
	
	/* Remember the end of the loop body */
	BasicBlock* loop_body_end = *code;
	
	/* Connect the end of the loop body to the condition */
	BasicBlock_setTarget(loop_body_end, cond);
	
	/* Create an empty basic block to go to when the while condition is false */
	BasicBlock* endwhile = BasicBlock_createNext(code);
	BasicBlock_setFalseTarget(cond, endwhile);
	return true;
}

static bool genStmtRead(Block* scope, BasicBlock** code, Stmt_Read* read_statement) {
	/* Generate code to read a value to the top of the stack */
	BasicBlock_addInsn(*code, MAKE_READ());
	
	/* Generate code to store the value that was read on top of the stack into the variable */
	return genStoreVar(scope, code, read_statement->ident);
}

static bool genStmtWrite(Block* scope, BasicBlock** code, Stmt_Write* write_statement) {
	/* Generate code to load the value of an identifier to the top of the stack */
	bool success = genLoadIdent(scope, code, write_statement->ident);
	if(!success) {
		return false;
	}
	
	/* Generate code to write out the value of the variable */
	BasicBlock_addInsn(*code, MAKE_WRITE());
	return true;
}

static bool genLoadIdent(Block* scope, BasicBlock** code, AST_Ident* ident) {
	/* Lookup the symbol by its name */
	const char* name = ident->name;
	Symbol* sym = SymTree_findSymbol(scope->symtree, name);
	if(sym == NULL) {
		semanticError("Symbol \"%s\" used but not declared", name);
		return false;
	}
	
	/* Add the symbol to the basic block (for CFG annotations) */
	BasicBlock_markSymbol(*code, sym);
	
	/* Produce code to load the symbol based on its type */
	switch(sym->type) {
		case SYM_PROC:
			/* Can't use a procedure here */
			semanticError("Symbol \"%s\" was used like a variable but is a procedure", name);
			return false;
			
		case SYM_CONST:
			/* For a constant, just push its value (set during symbol resolution) */
			BasicBlock_addInsn(*code, MAKE_LIT(0));
			return true;
			
		case SYM_VAR: {
			/* For a variable, load it from its stack location (set during symbol resolution) */
			BasicBlock_addInsn(*code, MAKE_LOD(0, 0));
			return true;
		}
			
		default:
			abort();
	}
}

static bool genStoreVar(Block* scope, BasicBlock** code, AST_Ident* ident) {
	/* Lookup the symbol for the variable in the assignment */
	const char* name = ident->name;
	Symbol* sym = SymTree_findSymbol(scope->symtree, name);
	if(sym == NULL) {
		semanticError("Tried to modify variable \"%s\" before it was declared", name);
		return false;
	}
	
	/* Add the symbol to the basic block (for CFG annotations) */
	BasicBlock_markSymbol(*code, sym);
	
	/* Only variables are allowed, but handle each type for better error messages */
	switch(sym->type) {
		case SYM_CONST:
			semanticError("Tried to modify \"%s\", but it is a constant", name);
			return false;
			
		case SYM_PROC:
			semanticError("Tried to modify \"%s\", but it is a procedure", name);
			return false;
			
		case SYM_VAR: {
			/* Store the value to the variable's stack offset (set during symbol resolution) */
			BasicBlock_addInsn(*code, MAKE_STO(0, 0));
			return true;
		}
			
		default:
			abort();
	}
}

