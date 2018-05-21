//
//  genllvm.c
//  PL/0
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#include "genllvm.h"
#include <stdbool.h>
#include "compiler/codegen/symtree.h"


#define ENUM_TO_BIT(val) (1 << ((val) - 1))
typedef uint32_t STMT_TYPE_MASK;

typedef int stmt_callback(AST_Stmt* stmt, void* cookie);

static LLVMTypeRef getWordTy(void);
static int foreachStmt(AST_Stmt* stmt, STMT_TYPE_MASK filter_types, stmt_callback* stmt_cb, void* cookie);
static int checkAssignsToReturn(AST_Stmt* stmt, void* cookie);
static bool genProg(AST_Block* prog, LLVMModuleRef* pModule);
static bool genProc(SymTree* scope, LLVMModuleRef module, AST_Proc* proc);
static bool genStmt(SymTree* scope, LLVMBuilderRef builder, AST_Stmt* stmt);


Destroyer(GenLLVM) {
	/* Nothing to destroy */
}
DEF(GenLLVM);


GenLLVM* GenLLVM_initWithAST(GenLLVM* self, AST_Block* prog) {
	if((self = GenLLVM_init(self))) {
		/* Build the symbol tree */
		SymTree* scope = SymTree_initWithAST(SymTree_alloc(), NULL, prog, 0);
		if(scope == NULL) {
			/* Codegen error occurred, so destroy self and return NULL */
			release(&self);
			return NULL;
		}
		
		/* Create the LLVM module and generate code for the entire program */
		if(!genProg(prog, &self->module)) {
			release(&self);
			return NULL;
		}
	}
	
	return self;
}

void GenLLVM_drawGraph(GenLLVM* self, FILE* fp) {
	(void)self;
	(void)fp;
	//TODO
}

void GenLLVM_writeSymbolTable(GenLLVM* self, FILE* fp) {
	(void)self;
	(void)fp;
	//TODO
}

void GenLLVM_emit(GenLLVM* self, FILE* fp) {
	(void)self;
	(void)fp;
	//TODO
}


static LLVMTypeRef getWordTy(void) {
	/* Ensure Word is a 16-bit integer */
	ASSERT(sizeof(Word) == sizeof(int32_t));
	
	/* Ensure Word is signed */
	ASSERT((Word)(-1) < 0);
	
	/* Word is an i32 */
	return LLVMInt32Type();
}

static bool genProg(AST_Block* prog, LLVMModuleRef* pModule) {
	*pModule = LLVMModuleCreateWithName("pl0");
	LLVMTypeRef wordType = getWordTy();
	
	/* Generate constants */
	foreach(&prog->consts->consts, pconst) {
		LLVMValueRef constValue = LLVMConstInt(wordType, pconst->value, true);
		
	}
	
	/* Generate global variables */
	foreach(&prog->vars->vars, pvar) {
		LLVMAddGlobal(*pModule, wordType, *pvar);
	}
	
	/* Generate procedures */
	foreach(&prog->procs->procs, pproc) {
		if(!genProc(*pModule, *pproc) {
			return false;
		}
	}
	
	/* Generate body */
	
}

static int foreachStmt(AST_Stmt* stmt, STMT_TYPE_MASK filter_types, stmt_callback* stmt_cb, void* cookie) {
	int ret = 0;
	if(stmt == NULL) {
		return ret;
	}
	
	/* Is this statment type one that the caller is interested in? */
	if(filter_types & ENUM_TO_BIT(stmt->type)) {
		ret = stmt_cb(stmt, cookie);
		if(ret != 0) {
			return ret;
		}
	}
	
	switch(stmt->type) {
		case STMT_BEGIN: {
			/* Recurse for each statement in the list */
			foreach(&stmt->stmt.begin.stmts, pstmt) {
				ret = foreachStmt(*pstmt, filter_types, stmt_cb, cookie);
				if(ret != 0) {
					return ret;
				}
			}
			break;
		}
			
		case STMT_IF:
			/* Recurse for the then and else branches */
			ret = foreachStmt(stmt->stmt.if_stmt.then_stmt, filter_types, stmt_cb, cookie);
			if(ret != 0) {
				return ret;
			}
			ret = foreachStmt(stmt->stmt.if_stmt.else_stmt, filter_types, stmt_cb, cookie);
			if(ret != 0) {
				return ret;
			}
			break;
			
		case STMT_WHILE:
			/* Recurse for the loop body */
			ret = foreachStmt(stmt->stmt.while_stmt.do_stmt, filter_types, stmt_cb, cookie);
			if(ret != 0) {
				return ret;
			}
			break;
			
		default:
			/* Ignore this statement, as it can't contain child statements */
			break;
	}
	
	return ret;
}

static int checkAssignsToReturn(AST_Stmt* stmt, void* cookie) {
	bool* doesReturn = cookie;
	ASSERT(stmt->type == STMT_ASSIGN);
	
	if(strcmp(stmt->stmt.assign.ident, "assign") == 0) {
		*doesReturn = true;
		return 1;
	}
}

static bool genProc(LLVMModuleRef module, AST_Proc* proc) {
	/* Find the return type (which right now is either Word or void) */
	bool doesReturn = false;
	foreachStmt(proc->body->stmt, ENUM_TO_BIT(STMT_ASSIGN), checkAssignsToReturn, &doesReturn);
	LLVMTypeRef returnType = doesReturn ? getWordTy() : LLVMVoidType();
	
	/* Build the parameter type list */
	unsigned paramCount = (unsigned)proc->param_decls->params.count;
	LLVMTypeRef paramTypes[paramCount];
	unsigned i;
	for(i = 0; i < paramCount; i++) {
		paramTypes[i] = getWordTy();
	}
	
	/* Build the function type */
	LLVMTypeRef procType = LLVMFunctionType(returnType, paramTypes, paramCount, false);
	
	/* Create and add the function for this procedure to the module */
	LLVMValueRef procValue = LLVMAddFunction(module, proc->ident, procType);
	
	/* Generate entrypoint to this procedure */
	LLVMBasicBlockRef entrypoint = LLVMAppendBasicBlock(procValue, "entry");
	
	/* Create an IR builder to generate the code in this procedure's body */
	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMPositionBuilderAtEnd(builder, entrypoint);
	
	//TODO
	
	if(!genStmt(builder, proc->body->stmt)) {
		return false;
	}
	
	//TODO
}

static bool genStmt(LLVMBuilderRef builder, AST_Stmt* stmt) {
	switch(stmt->type) {
		case STMT_ASSIGN:
			//TODO
			/* Generate code to evaluate the expression */
			if(!genExpr(scope, code, statement->stmt.assign.value)) {
				return false;
			}
			
			/* Generate code to store the evaluation result in the variable */
			return genStoreVar(scope, code, statement->stmt.assign.ident);
			
		case STMT_CALL:
			//TODO
			/* Generate a call but don't increment the stack afterwards (ignore return value) */
			return genCall(scope, code, statement->stmt.call.ident, statement->stmt.call.param_list);
			
		case STMT_BEGIN: {
			size_t i;
			for(i = 0; i < stmt->stmt.begin.stmt_count; i++) {
				/* Generate the code for each statement */
				if(!genStmt(builder, stmt->stmt.begin.stmts[i])) {
					return false;
				}
			}
			return true;
		}
			
		case STMT_IF: {
			//TODO
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
			//TODO
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
			//TODO
			/* Generate code to read a value to the top of the stack */
			BasicBlock_addInsn(*code, MAKE_READ());
			
			/* Generate code to store the value that was read on top of the stack into the variable */
			return genStoreVar(scope, code, statement->stmt.read.ident);
			
		case STMT_WRITE:
			//TODO
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


static bool genExpr(LLVMBuilderRef builder, AST_Expr* expr, LLVMValueRef* pResult) {
	/* Used for binary operations */
	LLVMOpcode opType;
	
	switch(expr->type) {
		case EXPR_VAR:
			//TODO
			return genLoadIdent(scope, code, expression->values.ident);
			
		case EXPR_NUM:
			*pResult = LLVMConstInt(getWordTy(), expr->values.num, true);
			return true;
			
		case EXPR_NEG: {
			LLVMValueRef val;
			
			/* Evaluate inner expression */
			if(!genExpr(builder, expr->values.operand, &val)) {
				return false;
			}
			
			/* Negate result of expression */
			*pResult = LLVMBuildNeg(builder, val, "negtmp");
			return true;
		}
			
		/* Binary operations */
		case EXPR_ADD: opType = LLVMAdd; break;
		case EXPR_SUB: opType = LLVMSub; break;
		case EXPR_MUL: opType = LLVMMul; break;
		case EXPR_DIV: opType = LLVMSDiv; break;
			
		case EXPR_CALL:
			//TODO
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
	LLVMValueRef left, right;
	
	/* Generate left expression value */
	if(!genExpr(builder, expr->values.binop.left, &left)) {
		return false;
	}
	
	/* Generate right expression value */
	if(!genExpr(builder, expr->values.binop.right, &right)) {
		return false;
	}
	
	/* Apply binary operation */
	*result = LLVMBuildBinOp(builder, opType, left, right, "binoptmp");
	return true;
}
