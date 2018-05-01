//
//  genpm0.c
//  PL/0
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#include "genpm0.h"
#include "basicblock.h"


Destroyer(GenPM0) {
	release(&self->block);
}
DEF(GenPM0);


static void GenPM0_optimize(GenPM0* self);
static void GenPM0_layoutCode(GenPM0* self);
static void vSemanticError(const char* fmt, va_list ap);
static void semanticError(const char* fmt, ...);
static bool genStmt(SymTree* scope, BasicBlock** code, AST_Stmt* statement);
static bool genCond(SymTree* scope, BasicBlock** code, AST_Cond* condition);
static bool genExpr(SymTree* scope, BasicBlock** code, AST_Expr* expression);
static bool genNumber(SymTree* scope, BasicBlock** code, Word number);
static bool genCall(SymTree* scope, BasicBlock** code, char* ident, AST_ParamList* param_list);
static bool genParamList(SymTree* scope, BasicBlock** code, AST_ParamList* param_list);
static bool genLoadIdent(SymTree* scope, BasicBlock** code, char* ident);
static bool genStoreVar(SymTree* scope, BasicBlock** code, char* ident);


static void vSemanticError(const char* fmt, va_list ap) {
	printf("Semantic Error: ");
	vprintf(fmt, ap);
	printf("\n");
}

static void semanticError(const char* fmt, ...) {
	VARIADIC(fmt, ap, {
		vSemanticError(fmt, ap);
	});
}


GenPM0* GenPM0_initWithAST(GenPM0* self, AST_Block* prog) {
	if((self = GenPM0_init(self))) {
		/* Build the symbol tree */
		SymTree* scope = SymTree_initWithAST(SymTree_alloc(), NULL, NULL, prog, 0);
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
		if(!Block_generate(self->block, prog)) {
			/* Codegen error occurred, so destroy self and return NULL */
			release(&self);
			return NULL;
		}
		
		/* Optimize the block */
		GenPM0_optimize(self);
		
		/* Layout the code in the block */
		GenPM0_layoutCode(self);
	}
	
	return self;
}


static void GenPM0_optimize(GenPM0* self) {
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

static void GenPM0_layoutCode(GenPM0* self) {
	/* Recursively set the addresses of all referenced code and resolve undefined symbols */
	Block_setAddress(self->block, 0);
}

void GenPM0_drawGraph(GenPM0* self, FILE* fp) {
	Graphviz* gv = Graphviz_initWithFile(Graphviz_alloc(), fp, "program");
	Graphviz_draw(gv, "bgcolor=azure;");
	Block_drawGraph(self->block, gv);
	release(&gv);
}

void GenPM0_writeSymbolTable(GenPM0* self, FILE* fp) {
	fprintf(fp, "Name\tType\tLevel\tValue\n");
	SymTree_write(self->block->symtree, fp);
}

void GenPM0_emit(GenPM0* self, FILE* fp) {
	/* The top level code starts at address zero */
	Block_emit(self->block, fp);
}


bool GenPM0_genBlock(SymTree* scope, BasicBlock** code, AST_Block* ast) {
	/* Produce an INC if we need to modify the stack space */
	if(scope->frame_size != 0) {
		BasicBlock_addInsn(*code, MAKE_INC(scope->frame_size));
	}
	
	/* Generate code for the statements contained in the block */
	return genStmt(scope, code, ast->stmt);
}

static bool genStmt(SymTree* scope, BasicBlock** code, AST_Stmt* statement) {
	/* Empty statement, so do nothing and return success */
	if(!statement) {
		return true;
	}
	
	switch(statement->type) {
		case STMT_ASSIGN:
			/* Generate code to evaluate the expression */
			if(!genExpr(scope, code, statement->stmt.assign.value)) {
				return false;
			}
			
			/* Generate code to store the evaluation result in the variable */
			return genStoreVar(scope, code, statement->stmt.assign.ident);
			
		case STMT_CALL:
			/* Generate a call but don't increment the stack afterwards (ignore return value) */
			return genCall(scope, code, statement->stmt.call.ident, statement->stmt.call.param_list);
			
		case STMT_BEGIN: {
			foreach(&statement->stmt.begin.stmts, pstmt) {
				/* Generate the code for each statement */
				if(!genStmt(scope, code, *pstmt)) {
					return false;
				}
			}
			return true;
		}
			
		case STMT_IF: {
			/* Generate the code to compute the condition */
			if(!genCond(scope, code, statement->stmt.if_stmt.cond)) {
				return false;
			}
			
			/* Remember the basic block for the condition */
			BasicBlock* cond = *code;
			
			/* Create an empty basic block to hold the code when the condition is true */
			BasicBlock* true_branch_begin = BasicBlock_createNext(code);
			BasicBlock_setTarget(cond, true_branch_begin);
			
			/* Generate code for the then statement of the if statement */
			if(!genStmt(scope, code, statement->stmt.if_stmt.then_stmt)) {
				return false;
			}
			
			/* Remember the last basic block of the true branch */
			BasicBlock* true_branch_end = *code;
			
			/* Create an empty basic block to hold the code when the condition is false */
			BasicBlock* false_branch_begin = BasicBlock_createNext(code);
			BasicBlock_setFalseTarget(cond, false_branch_begin);
			
			/* Does this if statement have an else branch to it? */
			if(statement->stmt.if_stmt.else_stmt == NULL) {
				/* There is no else statement, so make the code from the true branch rejoin the false branch */
				BasicBlock_setTarget(true_branch_end, false_branch_begin);
			}
			else {
				/* Generate code for the else statement of the if statement */
				if(!genStmt(scope, code, statement->stmt.if_stmt.else_stmt)) {
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
			
		case STMT_WHILE: {
			/* Remember the basic block just before the condition to link it */
			BasicBlock* before_cond = *code;
			
			/* Create a new basic block for the condition */
			BasicBlock* cond = BasicBlock_createNext(code);
			BasicBlock_setTarget(before_cond, cond);
			
			/* Generate code for the condition of the while statement */
			if(!genCond(scope, code, statement->stmt.while_stmt.cond)) {
				return false;
			}
			
			/* Create a new basic block for the loop body */
			BasicBlock* loop_body_begin = BasicBlock_createNext(code);
			BasicBlock_setTarget(cond, loop_body_begin);
			
			/* Generate code for the body of the while statement */
			if(!genStmt(scope, code, statement->stmt.while_stmt.do_stmt)) {
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
			
		case STMT_READ:
			/* Generate code to read a value to the top of the stack */
			BasicBlock_addInsn(*code, MAKE_READ());
			
			/* Generate code to store the value that was read on top of the stack into the variable */
			return genStoreVar(scope, code, statement->stmt.read.ident);
			
		case STMT_WRITE:
			/* Generate code to load the value of an identifier to the top of the stack */
			if(!genLoadIdent(scope, code, statement->stmt.write.ident)) {
				return false;
			}
			
			/* Generate code to write out the value of the variable */
			BasicBlock_addInsn(*code, MAKE_WRITE());
			return true;
			
		default:
			ASSERT(!"Unknown statement type");
	}
}

static bool genCond(SymTree* scope, BasicBlock** code, AST_Cond* condition) {
	/* Store the start of the condition in the basic block for optimizations later */
	BasicBlock_markCondition(*code);
	
	/* Get instruction type for the condition */
	Insn cond_insn;
	switch(condition->type) {
			/* For unary operators, generate the code now */
		case COND_ODD:
			if(!genExpr(scope, code, condition->values.operand)) {
				return false;
			}
			BasicBlock_addInsn(*code, MAKE_ODD());
			return true;
			
			/* For binary operators, just select the comparison instruction */
		case COND_EQ: cond_insn = MAKE_EQL(); break;
		case COND_NE: cond_insn = MAKE_NEQ(); break;
		case COND_LT: cond_insn = MAKE_LSS(); break;
		case COND_LE: cond_insn = MAKE_LEQ(); break;
		case COND_GT: cond_insn = MAKE_GTR(); break;
		case COND_GE: cond_insn = MAKE_GEQ(); break;
			
		default:
			ASSERT(!"Unknown condition type");
	}
	
	/* Create code for the first operand of the conditional operator */
	if(!genExpr(scope, code, condition->values.binop.left)) {
		return false;
	}
	
	/* All other conditions have a second operand */
	if(!genExpr(scope, code, condition->values.binop.right)) {
		return false;
	}
	
	/* Output the comparison instruction */
	BasicBlock_addInsn(*code, cond_insn);
	return true;
}

static bool genExpr(SymTree* scope, BasicBlock** code, AST_Expr* expression) {
	Insn expr_insn;
	switch(expression->type) {
		case EXPR_VAR:
			return genLoadIdent(scope, code, expression->values.ident);
			
		case EXPR_NUM:
			return genNumber(scope, code, expression->values.num);
			
		case EXPR_NEG:
			if(!genExpr(scope, code, expression->values.operand)) {
				return false;
			}
			BasicBlock_addInsn(*code, MAKE_NEG());
			return true;
			
		case EXPR_ADD: expr_insn = MAKE_ADD(); break;
		case EXPR_SUB: expr_insn = MAKE_SUB(); break;
		case EXPR_MUL: expr_insn = MAKE_MUL(); break;
		case EXPR_DIV: expr_insn = MAKE_DIV(); break;
		case EXPR_MOD: expr_insn = MAKE_MOD(); break;
			
		case EXPR_CALL:
			if(!genCall(scope, code, expression->values.call.ident, expression->values.call.param_list)) {
				return false;
			}
			
			/* Adjust the stack pointer by one so the procedure's result is on the stack */
			BasicBlock_addInsn(*code, MAKE_INC(1));
			return true;
			
		default:
			ASSERT(!"Unknown expression type");
	}
	
	/* Only binary operators execute this code */
	
	/* Generate left operand */
	if(!genExpr(scope, code, expression->values.binop.left)) {
		return false;
	}
	
	/* Generate right operand */
	if(!genExpr(scope, code, expression->values.binop.right)) {
		return false;
	}
	
	/* Add binary operator instruction */
	BasicBlock_addInsn(*code, expr_insn);
	return true;
}

static bool genNumber(SymTree* scope, BasicBlock** code, Word number) {
	/* The scope variable is not necessary, so it is ignored */
	(void)scope;
	BasicBlock_addInsn(*code, MAKE_LIT(number));
	return true;
}

static bool genCall(SymTree* scope, BasicBlock** code, char* ident, AST_ParamList* param_list) {
	/* Lookup the procedure symbol by name */
	Symbol* sym = SymTree_findSymbol(scope, ident);
	if(sym == NULL) {
		semanticError("Tried to call procedure \"%s\" which isn't declared at this scope", ident);
		return false;
	}
	
	/* Are there parameters given? */
	if(param_list != NULL) {
		/* Generate code to evaluate the parameters and place them where they need to be */
		genParamList(scope, code, param_list);
	}
	
	/* Remember to resolve this reference later */
	BasicBlock_markSymbol(*code, sym);
	
	/* Generate the call instruction (target will be set during symbol resolution) */
	BasicBlock_addInsn(*code, MAKE_CAL(0, ADDR_UND));
	return true;
}

static bool genParamList(SymTree* scope, BasicBlock** code, AST_ParamList* param_list) {
	/* No need to adjust the stack at all if the parameter list is empty */
	if(param_list->params.count == 0) {
		return true;
	}
	
	/* Need to adjust stack pointer up 4 so the parameters go in the right place */
	BasicBlock_addInsn(*code, MAKE_INC(4));
	
	/* Generate the code to produce the value of all parameters */
	foreach(&param_list->params, pparam) {
		if(!genExpr(scope, code, *pparam)) {
			return false;
		}
	}
	
	/* Adjust the stack pointer back to where it was before we first adjusted it */
	BasicBlock_addInsn(*code, MAKE_INC(-(4 + (Word)param_list->params.count)));
	return true;
}

static bool genLoadIdent(SymTree* scope, BasicBlock** code, char* ident) {
	/* Lookup the symbol by its name */
	Symbol* sym = SymTree_findSymbol(scope, ident);
	if(sym == NULL) {
		semanticError("Symbol \"%s\" used but not declared", ident);
		return false;
	}
	
	/* Add the symbol to the basic block (for CFG annotations) */
	BasicBlock_markSymbol(*code, sym);
	
	/* Produce code to load the symbol based on its type */
	switch(sym->type) {
		case SYM_PROC:
			/* Can't use a procedure here */
			semanticError("Symbol \"%s\" was used like a variable but is a procedure", ident);
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
			ASSERT(!"Unknown symbol type");
	}
}

static bool genStoreVar(SymTree* scope, BasicBlock** code, char* ident) {
	/* Lookup the symbol for the variable in the assignment */
	Symbol* sym = SymTree_findSymbol(scope, ident);
	if(sym == NULL) {
		semanticError("Tried to modify variable \"%s\" before it was declared", ident);
		return false;
	}
	
	/* Add the symbol to the basic block (for CFG annotations) */
	BasicBlock_markSymbol(*code, sym);
	
	/* Only variables are allowed, but handle each type for better error messages */
	switch(sym->type) {
		case SYM_CONST:
			semanticError("Tried to modify \"%s\", but it is a constant", ident);
			return false;
			
		case SYM_PROC:
			semanticError("Tried to modify \"%s\", but it is a procedure", ident);
			return false;
			
		case SYM_VAR: {
			/* Store the value to the variable's stack offset (set during symbol resolution) */
			BasicBlock_addInsn(*code, MAKE_STO(0, 0));
			return true;
		}
			
		default:
			ASSERT(!"Unknown symbol type");
	}
}
