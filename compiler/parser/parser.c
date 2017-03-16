//
//  parser.c
//  PL/0
//
//  Created by Kevin Colley on 10/22/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "parser.h"
#include <stdlib.h>
#include <stdarg.h>

Destroyer(Parser) {
	release(&self->token_stream);
}
DEF(Parser);

Parser* Parser_initWithFile(Parser* self, FILE* fin) {
	return Parser_initWithStream(self, TokenStream_initWithFile(TokenStream_alloc(), fin));
}

Parser* Parser_initWithLexer(Parser* self, Lexer* lexer) {
	return Parser_initWithStream(self, TokenStream_initWithLexer(TokenStream_alloc(), lexer));
}

Parser* Parser_initWithStream(Parser* self, TokenStream* stream) {
	if((self = Parser_init(self))) {
		self->token_stream = stream;
	}
	
	return self;
}

#if WITH_BISON

#include "compiler/parser/parser.y.h"

bool Parser_parseProgram(Parser* self, AST_Block** program) {
	return yyparse(self->token_stream, program) == 0;
}

#else /* WITH_BISON */

/* Private parser function declarations */
static bool Parser_parseBlock(Parser* self, AST_Block** block);
static bool Parser_parseConstDecls(Parser* self, AST_ConstDecls** const_decls);
static bool Parser_parseVarDecls(Parser* self, AST_VarDecls** var_decls);
static bool Parser_parseProcDecls(Parser* self, AST_ProcDecls** proc_decls);
static bool Parser_parseProc(Parser* self, AST_Proc** procedure);
static bool Parser_parseParamDecls(Parser* self, AST_ParamDecls** param_decls);
static bool Parser_parseStmt(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtAssign(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtCall(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtBegin(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtIf(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtWhile(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtRead(Parser* self, AST_Stmt** statement);
static bool Parser_parseStmtWrite(Parser* self, AST_Stmt** statement);
static bool Parser_parseCond(Parser* self, AST_Cond** condition);
static bool Parser_parseExpr(Parser* self, AST_Expr** expression);
static bool Parser_parseRawExpr(Parser* self, AST_Expr** expression, bool negate);
static bool Parser_parseTerm(Parser* self, AST_Expr** term, bool negate);
static bool Parser_parseFactor(Parser* self, AST_Expr** factor);
static bool Parser_parseParamList(Parser* self, AST_ParamList** param_list);
static bool Parser_parseIdent(Parser* self, char** identifier);
static bool Parser_parseNumber(Parser* self, Word* number);
static bool Parser_parseCall(Parser* self, char** identifier, AST_ParamList** param_list);


static void vSyntaxError(const char* fmt, va_list ap) {
	printf("Syntax Error: ");
	vprintf(fmt, ap);
	printf("\n");
}

static void syntaxError(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vSyntaxError(fmt, ap);
	va_end(ap);
}


/*! Grammar:
 @code
 program ::= block "."
 @endcode
 */
bool Parser_parseProgram(Parser* self, AST_Block** program) {
	*program = NULL;
	AST_Block* prog = AST_Block_new();
	Token* tok;
	
	/* Parse the program block */
	if(!Parser_parseBlock(self, &prog)) {
		release(&prog);
		return false;
	}
	
	/* Consume "." token */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != periodsym) {
		syntaxError("Expected \".\" at end of program block, not \"%s\"", tok->lexeme);
		release(&prog);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*program = prog;
	return true;
}


/* Private parser function definitions */

/*! Grammar:
 @code
 block ::= const-declaration var-declaration proc-declaration statement
 @endcode
 */
static bool Parser_parseBlock(Parser* self, AST_Block** block) {
	*block = NULL;
	AST_Block* blk = AST_Block_new();
	
	/* Parse all parts of the block. Any errors will already have been printed */
	if(!Parser_parseConstDecls(self, &blk->consts) ||
	   !Parser_parseVarDecls(self, &blk->vars) ||
	   !Parser_parseProcDecls(self, &blk->procs) ||
	   !Parser_parseStmt(self, &blk->stmt)) {
		release(&blk);
		return false;
	}
	
	*block = blk;
	return true;
}

/*! Grammar:
 @code
 const-declaration ::= [ "const" ident "=" number {"," ident "=" number} ";" ]
 @endcode
 */
static bool Parser_parseConstDecls(Parser* self, AST_ConstDecls** const_decls) {
	*const_decls = NULL;
	AST_ConstDecls* consts = AST_ConstDecls_new();
	Token* tok;
	
	/* Read "const" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != constsym) {
		/* This is optional, so return success but leave the output parameter NULL */
		release(&consts);
		return true;
	}
	
	/* Parse constant assignments */
	do {
		/* This will consume "const" on first loop and separating commas after that */
		TokenStream_consumeToken(self->token_stream);
		
		/* Enlarge consts array if necessary */
		if(consts->const_count == consts->const_cap) {
			/* Enlarge both allocations */
			size_t oldcap = consts->const_cap;
			expand(&consts->idents, &oldcap);
			expand(&consts->values, &consts->const_cap);
		}
		
		/* Parse name of constant being defined */
		if(!Parser_parseIdent(self, &consts->idents[consts->const_count])) {
			syntaxError("Expected identifier in constant declaration");
			release(&consts);
			return false;
		}
		
		/* Consume "=" */
		if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != eqsym) {
			if(tok != NULL) {
				syntaxError("Expected \"=\" after name in constant declaration, not \"%s\"", tok->lexeme);
			}
			else {
				syntaxError("Unexpected EOF after name in constant declaration");
			}
			release(&consts);
			return false;
		}
		TokenStream_consumeToken(self->token_stream);
		
		/* Parse value of constant being defined */
		if(!Parser_parseNumber(self, &consts->values[consts->const_count++])) {
			syntaxError("Expected number after \"=\" in constant declaration");
			release(&consts);
			return false;
		}
	} while(TokenStream_peekToken(self->token_stream, &tok) && tok->type == commasym);
	
	/* Consume ";" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != semicolonsym) {
		if(tok != NULL && tok->type == identsym) {
			syntaxError("Expected \",\" between constant declarations");
		}
		else {
			syntaxError("Expected \";\" at end of constant declaration");
		}
		
		release(&consts);
		return NULL;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*const_decls = consts;
	return true;
}

/*! Grammar:
 @code
 var-declaration ::= [ "var" ident {"," ident} ";" ]
 @endcode
 */
static bool Parser_parseVarDecls(Parser* self, AST_VarDecls** var_decls) {
	*var_decls = NULL;
	AST_VarDecls* vars = AST_VarDecls_new();
	Token* tok;
	
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != varsym) {
		/* This is optional, so return success but leave the output parameter NULL */
		release(&vars);
		return true;
	}
	
	/* Parse variable declarations */
	do {
		/* This will consume "var" on first loop and separating commas after that */
		TokenStream_consumeToken(self->token_stream);
		
		/* Enlarge consts array if necessary */
		if(vars->var_count == vars->var_cap) {
			expand(&vars->vars, &vars->var_cap);
		}
		
		/* Parse name of variable being declared */
		if(!Parser_parseIdent(self, &vars->vars[vars->var_count++])) {
			syntaxError("Expected identifier in variable declaration");
			release(&vars);
			return false;
		}
	} while(TokenStream_peekToken(self->token_stream, &tok) && tok->type == commasym);
	
	/* Consume ";" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != semicolonsym) {
		if(tok != NULL && tok->type == identsym) {
			syntaxError("Expected \",\" between variable declarations");
		}
		else {
			syntaxError("Expected \";\" at end of variable declarations");
		}
		
		release(&vars);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*var_decls = vars;
	return true;
}

/*! Grammar:
 @code
 proc-decls ::= { proc-decl }
 @endcode
 */
static bool Parser_parseProcDecls(Parser* self, AST_ProcDecls** proc_decls) {
	*proc_decls = NULL;
	AST_ProcDecls* procs = AST_ProcDecls_new();
	Token* tok;
	
	/* Parse procedures as long as we see "procedure" */
	while(TokenStream_peekToken(self->token_stream, &tok) && tok->type == procsym) {
		/* Expand array if necessary */
		if(procs->proc_count == procs->proc_cap) {
			expand(&procs->procs, &procs->proc_cap);
		}
		
		/* Parse a procedure and append it to the array */
		if(!Parser_parseProc(self, &procs->procs[procs->proc_count++])) {
			release(&procs);
			return false;
		}
	}

	*proc_decls = procs;
	return true;
}

/*! Grammar:
 @code
 proc-decl ::= "procedure" ident parameter-block ";" block ";"
 @endcode
 */
static bool Parser_parseProc(Parser* self, AST_Proc** procedure) {
	*procedure = NULL;
	AST_Proc* proc = AST_Proc_new();
	Token* tok;
	
	/* Consume "procedure" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != procsym) {
		/* Shouldn't be possible */
		abort();
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse the procedure's name */
	if(!Parser_parseIdent(self, &proc->ident)) {
		syntaxError("Expected identifier after \"procedure\"");
		release(&proc);
		return false;
	}
	
	/* Parse parameter declarations */
	if(!Parser_parseParamDecls(self, &proc->param_decls)) {
		release(&proc);
		return false;
	}
	
	/* Consume ";" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != semicolonsym) {
		syntaxError("Expected \";\" after name of procedure in procedure declaration");
		release(&proc);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse the procedure's block */
	if(!Parser_parseBlock(self, &proc->body)) {
		release(&proc);
		return false;
	}
	
	/* Consume ";" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != semicolonsym) {
		syntaxError("Expected \";\" at end of procedure declaration");
		release(&proc);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*procedure = proc;
	return true;
}

/*! Grammar:
 @code
 param-decls ::= "(" [ ident { "," ident } ] ")"
 @endcode
 */
static bool Parser_parseParamDecls(Parser* self, AST_ParamDecls** param_decls) {
	*param_decls = NULL;
	AST_ParamDecls* params = AST_ParamDecls_new();
	Token* tok;
	
	/* Consume "(" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != lparentsym) {
		syntaxError("Expected parameter declaration list after procedure declaration");
		release(&params);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Check if there are any parameters to parse */
	if(TokenStream_peekToken(self->token_stream, &tok) && tok->type != rparentsym) {
		/* Enlarge array if necessary */
		if(params->param_count == params->param_cap) {
			expand(&params->params, &params->param_cap);
		}
		
		/* Parse name of first parameter */
		if(!Parser_parseIdent(self, &params->params[params->param_count++])) {
			syntaxError("Expected identifier for first parameter in parameter declarations list");
			release(&params);
			return false;
		}
		
		/* Keep going as long as we have another parameter to parse */
		while(TokenStream_peekToken(self->token_stream, &tok) && tok->type == commasym) {
			/* Consume "," */
			TokenStream_consumeToken(self->token_stream);
			
			/* Enlarge array if necessary */
			if(params->param_count == params->param_cap) {
				expand(&params->params, &params->param_cap);
			}
			
			/* Parse name of next parameter */
			if(!Parser_parseIdent(self, &params->params[params->param_count++])) {
				syntaxError("Expected identifier for parameter %zu in parameter declarations list",
							params->param_count);
				release(&params);
				return false;
			}
		}
	}
	
	/* Consume ")" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != rparentsym) {
		syntaxError("Expected \")\" at end of parameter declarations");
		release(&params);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*param_decls = params;
	return true;
}

/*! Grammar:
 @code
 statement ::= [ stmt-assign |
                 stmt-call   |
                 stmt-begin  |
                 stmt-if     |
                 stmt-while  |
                 stmt-read   |
                 stmt-write ]
 @endcode
 */
static bool Parser_parseStmt(Parser* self, AST_Stmt** statement) {
	*statement = NULL;
	Token* tok;
	
	/* Make sure to handle EOF */
	if(!TokenStream_peekToken(self->token_stream, &tok)) {
		return true;
	}
	
	/* Invoke the parser function that corresponds to each branch of the alternation */
	switch(tok->type) {
		case identsym: return Parser_parseStmtAssign(self, statement);
		case callsym:  return Parser_parseStmtCall(self, statement);
		case beginsym: return Parser_parseStmtBegin(self, statement);
		case ifsym:    return Parser_parseStmtIf(self, statement);
		case whilesym: return Parser_parseStmtWhile(self, statement);
		case readsym:  return Parser_parseStmtRead(self, statement);
		case writesym: return Parser_parseStmtWrite(self, statement);
		default:       return true;
	}
}

/*! Grammar:
 @code
 condition ::= "odd" expression | expression rel-op expression
 rel-op    ::= "=" | "<>" | "<" | "<=" | ">" | ">="
 @endcode
 */
static bool Parser_parseCond(Parser* self, AST_Cond** condition) {
	*condition = NULL;
	AST_Cond* cond = AST_Cond_new();
	Token* tok;
	
	/* Peek the next token */
	if(!TokenStream_peekToken(self->token_stream, &tok)) {
		syntaxError("Expected a condition but encountered end of file");
		release(&cond);
		return false;
	}
	
	if(tok->type == oddsym) {
		/* Consume "odd" */
		TokenStream_consumeToken(self->token_stream);
		cond->type = COND_ODD;
		
		/* Parse the single operand to the "odd" keyword */
		if(!Parser_parseExpr(self, &cond->values.operand)) {
			release(&cond);
			return false;
		}
	}
	else {
		/* Parse the left operand to the condition */
		if(!Parser_parseExpr(self, &cond->values.binop.left)) {
			release(&cond);
			return false;
		}
		
		/* Peek next token */
		if(!TokenStream_peekToken(self->token_stream, &tok)) {
			syntaxError("Expected a relational operator but encountered end of file");
			return false;
		}
		
		/* Determine the type of the conditional operator and consume it */
		switch(tok->type) {
			case eqsym:  cond->type = COND_EQ; break;
			case neqsym: cond->type = COND_NE; break;
			case lessym: cond->type = COND_LT; break;
			case leqsym: cond->type = COND_LE; break;
			case gtrsym: cond->type = COND_GT; break;
			case geqsym: cond->type = COND_GE; break;
				
			default:
				syntaxError("Expected a relational operator but encountered \"%s\"", tok->lexeme);
				release(&cond);
				return false;
		}
		TokenStream_consumeToken(self->token_stream);
		
		/* Parse the right operand to the condition */
		if(!Parser_parseExpr(self, &cond->values.binop.right)) {
			release(&cond);
			return false;
		}
	}
	
	*condition = cond;
	return true;
}

/*! Grammar:
 @code
 expression ::= [ "+"|"-" ] raw-expression
 @endcode
 */
static bool Parser_parseExpr(Parser* self, AST_Expr** expression) {
	*expression = NULL;
	Token* tok;
	bool negate = false;
	
	/* Try to consume a unary plus or minus operator token */
	if(TokenStream_peekToken(self->token_stream, &tok)
	   && (tok->type == plussym || tok->type == minussym)) {
		negate = tok->type == minussym;
		TokenStream_consumeToken(self->token_stream);
	}
	
	/* Parse the raw expression */
	return Parser_parseRawExpr(self, expression, negate);
}

/*! Grammar:
 @code
 raw-expression ::= term [ ("+"|"-") raw-expression ]
 @endcode
 */
static bool Parser_parseRawExpr(Parser* self, AST_Expr** expression, bool negate) {
	*expression = NULL;
	Token* tok;
	AST_Expr* expr;
	
	/* Parse left term of the expression */
	if(!Parser_parseTerm(self, &expr, negate)) {
		return false;
	}
	
	/* Try to consume a plus or minus */
	if(TokenStream_peekToken(self->token_stream, &tok)
	   && (tok->type == plussym || tok->type == minussym)) {
		/* Determine expression type */
		EXPR_TYPE type = tok->type == plussym ? EXPR_ADD : EXPR_SUB;
		TokenStream_consumeToken(self->token_stream);
		
		/* Parse right expression of the expression */
		AST_Expr* right;
		if(!Parser_parseRawExpr(self, &right, false)) {
			release(&expr);
			return false;
		}
		
		/* Build binary expression */
		expr = AST_Expr_create(type, expr, right);
	}
	
	*expression = expr;
	return true;
}

/*! Grammar:
 @code
 term ::= factor [ ("*"|"/") term ]
 @endcode
 */
static bool Parser_parseTerm(Parser* self, AST_Expr** term, bool negate) {
	*term = NULL;
	Token* tok;
	AST_Expr* trm;
	
	/* Parse the left factor */
	if(!Parser_parseFactor(self, &trm)) {
		return false;
	}
	
	/* Negate left factor if told */
	if(negate) {
		trm = AST_Expr_create(EXPR_NEG, trm);
	}
	
	/* Try to consume a multiplication or division operator token */
	if(TokenStream_peekToken(self->token_stream, &tok)
	   && (tok->type == multsym || tok->type == slashsym)) {
		/* Determine expression type */
		EXPR_TYPE type = tok->type == multsym ? EXPR_MUL : EXPR_DIV;
		TokenStream_consumeToken(self->token_stream);
		
		/* Parse right side */
		AST_Expr* right;
		if(!Parser_parseTerm(self, &right, false)) {
			release(&trm);
			return false;
		}
		
		/* Create binary operator expression */
		trm = AST_Expr_create(type, trm, right);
	}
	
	*term = trm;
	return true;
}

/*! Grammar:
 @code
 factor ::= ident | number | "(" expression ")" | call-expr
 @endcode
 */
static bool Parser_parseFactor(Parser* self, AST_Expr** factor) {
	*factor = NULL;
	Token* tok;
	AST_Expr* fact;
	
	if(!TokenStream_peekToken(self->token_stream, &tok)) {
		syntaxError("Expected identifier, number, or parenthesized subexpression, but got EOF");
		release(&fact);
		return false;
	}
	
	switch(tok->type) {
		case identsym: {
			char* ident;
			if(!Parser_parseIdent(self, &ident)) {
				/* Shouldn't be possible */
				abort();
			}
			fact = AST_Expr_create(EXPR_VAR, ident);
			break;
		}
		
		case numbersym: {
			Word number;
			if(!Parser_parseNumber(self, &number)) {
				/* Shouldn't be possible */
				abort();
			}
			fact = AST_Expr_create(EXPR_NUM, number);
			break;
		}
		
		case lparentsym:
			/* Consume "(" */
			TokenStream_consumeToken(self->token_stream);
			
			/* Parse subexpression within parentheses */
			if(!Parser_parseExpr(self, &fact)) {
				return false;
			}
			
			/* Consume ")" */
			if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != rparentsym) {
				syntaxError("Expected \")\" after subexpression");
				release(&fact);
				return false;
			}
			TokenStream_consumeToken(self->token_stream);
			break;
		
		case callsym: {
			char* ident;
			AST_ParamList* param_list;
			if(!Parser_parseCall(self, &ident, &param_list)) {
				/* Don't print an error now as one should already have been printed */
				release(&fact);
				return false;
			}
			fact = AST_Expr_create(EXPR_CALL, ident, param_list);
			break;
		}
		
		default:
			syntaxError("Unexpected token \"%s\" while parsing factor", tok->lexeme);
			release(&fact);
			return false;
	}
	
	*factor = fact;
	return true;
}

/*! Grammar:
 @code
 number := [0-9]{1,5}
 @endcode
 */
static bool Parser_parseNumber(Parser* self, Word* number) {
	Token* tok;
	
	/* Try to peek the number token */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != numbersym) {
		return false;
	}
	assert(strlen(tok->lexeme) <= 5);
	
	/* Convert the lexeme into an unsigned integer and store it in the number node */
	char* end;
	*number = (Word)strtoul(tok->lexeme, &end, 10);
	assert(end == tok->lexeme + strlen(tok->lexeme));
	
	/* Consume the number token */
	TokenStream_consumeToken(self->token_stream);
	return true;
}

/*! Grammar:
 @code
 ident := [a-zA-Z]{1,11}
 @endcode
 */
static bool Parser_parseIdent(Parser* self, char** identifier) {
	*identifier = NULL;
	Token* tok;
	
	/* Peek the identifier token */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != identsym) {
		return false;
	}
	assert(strlen(tok->lexeme) <= 11);
	
	/* Copy the identifier's name into the identifier and consume the token */
	*identifier = strdup_ff(tok->lexeme);
	TokenStream_consumeToken(self->token_stream);
	return true;
}

/*! Grammar:
 @code
 call-expr ::= "call" ident parameter-list
 @endcode
 */
static bool Parser_parseCall(Parser* self, char** identifier, AST_ParamList** param_list) {
	*identifier = NULL;
	*param_list = NULL;
	Token* tok;
	
	/* Consume "call" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != callsym) {
		syntaxError("Expected \"call\"");
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse the identifier */
	if(!Parser_parseIdent(self, identifier)) {
		syntaxError("Expected identifier after \"call\"");
		return false;
	}
	
	/* Parse the procedure call's parameter list */
	if(!Parser_parseParamList(self, param_list)) {
		destroy(identifier);
		return false;
	}
	return true;
}

/*! Grammar:
 @code
 parameter-list ::= "(" [ expression { "," expression } ] ")"
 @endcode
 */
static bool Parser_parseParamList(Parser* self, AST_ParamList** param_list) {
	*param_list = NULL;
	AST_ParamList* paramList = AST_ParamList_new();
	Token* tok;
	
	/* Consume "(" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != lparentsym) {
		syntaxError("Expected parameter list after procedure call");
		release(&paramList);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Check if there are any parameters to parse */
	if(TokenStream_peekToken(self->token_stream, &tok) && tok->type != rparentsym) {
		/* Enlarge array if necessary */
		if(paramList->param_count == paramList->param_cap) {
			expand(&paramList->params, &paramList->param_cap);
		}
		
		/* Parse expression for first parameter */
		if(!Parser_parseExpr(self, &paramList->params[paramList->param_count++])) {
			syntaxError("Expected expression for first parameter in parameter list");
			release(&paramList);
			return false;
		}
		
		/* Keep going as long as we have another parameter to parse */
		while(TokenStream_peekToken(self->token_stream, &tok) && tok->type == commasym) {
			/* Consume "," */
			TokenStream_consumeToken(self->token_stream);
			
			/* Enlarge array if necessary */
			if(paramList->param_count == paramList->param_cap) {
				expand(&paramList->params, &paramList->param_cap);
			}
			
			/* Parse expression for next parameter */
			if(!Parser_parseExpr(self, &paramList->params[paramList->param_count++])) {
				syntaxError("Expected expression for parameter %zu in parameter list",
					paramList->param_count);
				release(&paramList);
				return false;
			}
		}
	}
	
	/* Consume ")" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != rparentsym) {
		syntaxError("Expected \")\" at end of parameter list");
		release(&paramList);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*param_list = paramList;
	return true;
}

/*! Grammar:
 @code
 stmt-assign ::= ident ":=" expression
 @endcode
 */
static bool Parser_parseStmtAssign(Parser* self, AST_Stmt** assign_statement) {
	*assign_statement = NULL;
	Token* tok;
	char* ident;
	AST_Expr* value;
	
	/* Parse the name of the variable */
	if(!Parser_parseIdent(self, &ident)) {
		/* Shouldn't be possible */
		abort();
	}
	
	/* Read and consume the := token */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != becomessym) {
		syntaxError("Expected \":=\" after identifier in assignment statement");
		destroy(&ident);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse the expression */
	if(!Parser_parseExpr(self, &value)) {
		destroy(&ident);
		return false;
	}
	
	/* Create assign statement */
	*assign_statement = AST_Stmt_create(STMT_ASSIGN, ident, value);
	return true;
}

/*! Grammar:
 @code
 stmt-call ::= "call" ident [ parameter-list ]
 @endcode
 */
static bool Parser_parseStmtCall(Parser* self, AST_Stmt** call_statement) {
	*call_statement = NULL;
	Token* tok;
	char* ident;
	AST_ParamList* param_list = NULL;
	
	/* Consume "call" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != callsym) {
		/* Shouldn't be possible */
		abort();
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Read the identifier, which should be the name of the subprocedure */
	if(!Parser_parseIdent(self, &ident)) {
		syntaxError("Expected identifier after \"call\"");
		return false;
	}
	
	/* Check if the next token is a left parenthesis */
	if(TokenStream_peekToken(self->token_stream, &tok) && tok->type == lparentsym) {
		/* Parse the parameter list following the procedure call statement */
		if(!Parser_parseParamList(self, &param_list)) {
			destroy(&ident);
			return false;
		}
	}
	
	/* Create call statement */
	*call_statement = AST_Stmt_create(STMT_CALL, ident, param_list);
	return true;
}

/*! Grammar:
 @code
 stmt-begin ::= "begin" statement { ";" statement } "end"
 @endcode
 */
static bool Parser_parseStmtBegin(Parser* self, AST_Stmt** begin_statement) {
	*begin_statement = NULL;
	Token* tok;
	AST_Stmt* begin = AST_Stmt_create(STMT_BEGIN);
	AST_Stmt* stmt;
	
	/* Peek for "begin" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != beginsym) {
		/* Shouldn't be possible */
		abort();
	}
	
	/* Keep parsing statements as long as we have a semicolon to separate them */
	do {
		/* This will consume "begin" on first loop and separating semicolons otherwise */
		TokenStream_consumeToken(self->token_stream);
		
		/* Parse next statement */
		if(!Parser_parseStmt(self, &stmt)) {
			release(&begin);
			return false;
		}
		
		/* Append statement to begin statement */
		AST_Stmt_append(begin, stmt);
	} while(TokenStream_peekToken(self->token_stream, &tok) && tok->type == semicolonsym);
	
	/* Consume "end" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != endsym) {
		syntaxError("Expected \"end\" at end of block");
		release(&begin);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	*begin_statement = begin;
	return true;
}

/*! Grammar:
 @code
 stmt-if ::= "if" condition "then" statement [ "else" statement ]
 @endcode
 */
static bool Parser_parseStmtIf(Parser* self, AST_Stmt** if_statement) {
	*if_statement = NULL;
	Token* tok;
	AST_Cond* cond;
	AST_Stmt* then_stmt;
	AST_Stmt* else_stmt = NULL;
	
	/* Consume "if" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != ifsym) {
		/* Shouldn't be possible */
		abort();
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse condition */
	if(!Parser_parseCond(self, &cond)) {
		syntaxError("Expected condition after \"if\"");
		return false;
	}
	
	/* Consume "then" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != thensym) {
		syntaxError("Expected \"then\" after condition of \"if\" statement");
		release(&cond);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse body of the "then" branch of the if statement */
	if(!Parser_parseStmt(self, &then_stmt)) {
		syntaxError("Expected statement after \"then\" in \"if\" statement");
		release(&cond);
		return false;
	}
	
	/* Check if we have an "else" branch for this if statement */
	if(TokenStream_peekToken(self->token_stream, &tok) && tok->type == elsesym) {
		/* Consume "else" */
		TokenStream_consumeToken(self->token_stream);
		
		/* Parse body of the "else" branch of the if statement */
		if(!Parser_parseStmt(self, &else_stmt)) {
			syntaxError("Expected statement after \"else\" in \"if\" statement");
			release(&then_stmt);
			release(&cond);
			return false;
		}
	}
	
	/* Create if statement */
	*if_statement = AST_Stmt_create(STMT_IF, cond, then_stmt, else_stmt);
	return true;
}

/*! Grammar:
 @code
 stmt-while ::= "while" condition "do" statement
 @endcode
 */
static bool Parser_parseStmtWhile(Parser* self, AST_Stmt** while_statement) {
	*while_statement = NULL;
	Token* tok;
	AST_Cond* cond;
	AST_Stmt* body;
	
	/* Consume "while" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != whilesym) {
		/* Shouldn't be possible */
		abort();
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse condition */
	if(!Parser_parseCond(self, &cond)) {
		syntaxError("Expected condition after \"while\"");
		return false;
	}
	
	/* Consume "do" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != dosym) {
		syntaxError("Expected \"do\" after condition of \"while\" statement");
		release(&cond);
		return false;
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse body of while statement */
	if(!Parser_parseStmt(self, &body)) {
		syntaxError("Expected statement after \"do\" in \"while\" statement");
		release(&cond);
		return false;
	}
	
	/* Create while statement */
	*while_statement = AST_Stmt_create(STMT_WHILE, cond, body);
	return true;
}

/*! Grammar:
 @code
 stmt-read ::= "read" ident
 @endcode
 */
static bool Parser_parseStmtRead(Parser* self, AST_Stmt** read_statement) {
	*read_statement = NULL;
	Token* tok;
	char* ident;
	
	/* Consume "read" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != readsym) {
		/* Shouldn't be possible */
		abort();
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse name of variable to read into */
	if(!Parser_parseIdent(self, &ident)) {
		syntaxError("Expected identifier after \"read\"");
		return false;
	}
	
	/* Create read statement */
	*read_statement = AST_Stmt_create(STMT_READ, ident);
	return true;
}

/*! Grammar:
 @code
 stmt-write ::= "write" ident
 @endcode
 */
static bool Parser_parseStmtWrite(Parser* self, AST_Stmt** write_statement) {
	*write_statement = NULL;
	Token* tok;
	char* ident;
	
	/* Consume "write" */
	if(!TokenStream_peekToken(self->token_stream, &tok) || tok->type != writesym) {
		/* Shouldn't be possible */
		abort();
	}
	TokenStream_consumeToken(self->token_stream);
	
	/* Parse name of variable to write */
	if(!Parser_parseIdent(self, &ident)) {
		syntaxError("Expected identifier after \"write\"");
		return false;
	}
	
	/* Create write statement */
	*write_statement = AST_Stmt_create(STMT_WRITE, ident);
	return true;
}

#endif /* WITH_BISON */
