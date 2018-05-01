/* Generate a thread-safe reentrant parser */
%define api.pure full

/* Declares yylex as `int yylex(YYSTYPE* lvalp, yyscan_t scanner)` */
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}{AST_Block** program}

%define parse.trace
%define parse.error verbose

/* Normal includes and declarations */
%code requires {
	#include "config.h"
	#include "compiler/ast_nodes.h"
	
	/* Forward declaration */
	typedef struct TokenStream* yyscan_t;
	void yyerror(yyscan_t scanner, AST_Block** program, char const* msg);
}

/* token_stream.h includes the generated header, so it must go last */
%code provides {
	#include "compiler/parser/token_stream.h"
}

/* All types used by tokens or non-terminals */
%union {
	char* ident;
	Word num;
	AST_Block* block;
	AST_ConstDecls* const_decls;
	AST_VarDecls* var_decls;
	AST_ProcDecls* proc_decls;
	AST_ParamDecls* param_decls;
	AST_Stmt* stmt;
	COND_TYPE cond_op;
	AST_Cond* cond;
	EXPR_TYPE expr_op;
	AST_Expr* expr;
	AST_ParamList* param_list;
}

/* All tokens are prefixed with "TOK_" for external usage */
%define api.token.prefix {TOK_}

/* Tokens this parser understands and their associated types (if any) */
%token         NUL         1 "EOF"        /*!< End of file */
%token <ident> IDENT       2 "IDENT"      /*!< An identifier (symbol), such as "myVar" */
%token <num>   NUMBER      3 "NUMBER"     /*!< A numeric literal, such as "12345" */
%token         ADD         4 "+"          /*!< The addition operator, "+" */
%token         SUB         5 "-"          /*!< The subtraction operator, "-" */
%token         MUL         6 "*"          /*!< The multiplication operator, "*" */
%token         DIV         7 "/"          /*!< The division operator, "/" */
%token         ODD         8 "odd"        /*!< The "odd" keyword */
%token         EQ          9 "="          /*!< The equality operator, "=" */
%token         NE         10 "<>"         /*!< The inequality operator, "<>" */
%token         LT         11 "<"          /*!< The less than operator, "<" */
%token         LE         12 "<="         /*!< The less than or equal operator, "<=" */
%token         GT         13 ">"          /*!< The greater than operator, ">" */
%token         GE         14 ">="         /*!< The greater than or equal operator, ">=" */
%token         LPAREN     15 "("          /*!< The left parenthesis character, "(" */
%token         RPAREN     16 ")"          /*!< The right parenthesis character, ")" */
%token         COMMA      17 ","          /*!< The comma character, "," */
%token         SEMICOLON  18 ";"          /*!< The semicolon character, ";" */
%token         DOT        19 "."          /*!< The period character, "." */
%token         ASSIGN     20 ":="         /*!< The assignment operator, ":=" */
%token         BEGIN      21 "begin"      /*!< The "begin" keyword */
%token         END        22 "end"        /*!< The "end" keyword */
%token         IF         23 "if"         /*!< The "if" keyword */
%token         THEN       24 "then"       /*!< The "then" keyword */
%token         WHILE      25 "while"      /*!< The "while" keyword */
%token         DO         26 "do"         /*!< The "do" keyword */
%token         CALL       27 "call"       /*!< The "call" keyword */
%token         CONST      28 "const"      /*!< The "const" keyword */
%token         VAR        29 "var"        /*!< The "var" keyword */
%token         PROCEDURE  30 "procedure"  /*!< The "proc" keyword */
%token         WRITE      31 "write"      /*!< The "write" keyword */
%token         READ       32 "read"       /*!< The "read" keyword */
%token         ELSE       33 "else"       /*!< The "else" keyword */
%token         MOD        34 "%"          /*!< The modulus operator, "%" */

/* Fix "dangling else" ambiguity (shift/reduce conflict) */
%precedence    THEN
%precedence    ELSE

%type <block> block
%type <const_decls> const_decls const_decls_list
%type <var_decls> var_decls var_decls_list
%type <proc_decls> proc_decls
%type <param_decls> param_decls param_decls_list
%type <stmt> stmt stmt_assign stmt_call stmt_begin stmts stmt_if stmt_while stmt_read stmt_write
%type <cond_op> cond_op
%type <cond> cond
%type <expr_op> expr_op term_op
%type <expr> expr raw_expr negated_expr term negated_term factor negated_factor
%type <param_list> params params_list

%%

program
	: block "."
		{ *program = $1; }
	;

block
	: const_decls var_decls proc_decls stmt
		{ $$ = AST_Block_create($1, $2, $3, $4); }
	;

const_decls
	: %empty
		{ $$ = NULL; }
	| "const" const_decls_list IDENT "=" NUMBER ";"
		{ $$ = AST_ConstDecls_append($2, $3, $5); }
	;

const_decls_list
	: %empty
		{ $$ = NULL; }
	| const_decls_list IDENT "=" NUMBER ","
		{ $$ = AST_ConstDecls_append($1, $2, $4); }
	;

var_decls
	: %empty
		{ $$ = NULL; }
	| VAR var_decls_list IDENT ";"
		{ $$ = AST_VarDecls_append($2, $3); }
	;

var_decls_list
	: %empty
		{ $$ = NULL; }
	| var_decls_list IDENT ","
		{ $$ = AST_VarDecls_append($1, $2); }
	;

proc_decls
	: %empty
		{ $$ = NULL; }
	| proc_decls "procedure" IDENT "(" param_decls ")" ";" block ";"
		{ $$ = AST_ProcDecls_append($1, $3, $5, $8); }
	;

param_decls
	: %empty
		{ $$ = NULL; }
	| param_decls_list
		{ $$ = $1; }
	;

param_decls_list
	: IDENT
		{ $$ = AST_ParamDecls_append(NULL, $1); }
	| param_decls_list "," IDENT
		{ $$ = AST_ParamDecls_append($1, $3); }
	;

stmt
	: %empty      { $$ = NULL; }
	| stmt_assign { $$ = $1; }
	| stmt_call   { $$ = $1; }
	| stmt_begin  { $$ = $1; }
	| stmt_if     { $$ = $1; }
	| stmt_while  { $$ = $1; }
	| stmt_read   { $$ = $1; }
	| stmt_write  { $$ = $1; }
	;

stmt_assign
	: IDENT ":=" expr
		{ $$ = AST_Stmt_create(STMT_ASSIGN, $1, $3); }
	;

stmt_call
	: "call" IDENT
		{ $$ = AST_Stmt_create(STMT_CALL, $2, NULL); }
	| "call" IDENT "(" params ")"
		{ $$ = AST_Stmt_create(STMT_CALL, $2, $4); }
	;

stmt_begin
	: "begin" stmts "end"
		{ $$ = $2; }
	;

stmts
	: stmt
		{ $$ = AST_Stmt_append(NULL, $1); }
	| stmts ";" stmt
		{ $$ = AST_Stmt_append($1, $3); }
	;

stmt_if
	: "if" cond "then" stmt
		{ $$ = AST_Stmt_create(STMT_IF, $2, $4, NULL); }
	| "if" cond "then" stmt "else" stmt
		{ $$ = AST_Stmt_create(STMT_IF, $2, $4, $6); }
	;

stmt_while
	: "while" cond "do" stmt
		{ $$ = AST_Stmt_create(STMT_WHILE, $2, $4); }
	;

stmt_read
	: "read" IDENT
		{ $$ = AST_Stmt_create(STMT_READ, $2); }
	;

stmt_write
	: "write" IDENT
		{ $$ = AST_Stmt_create(STMT_WRITE, $2); }
	;

cond_op
	: "="  { $$ = COND_EQ; }
	| "<>" { $$ = COND_NE; }
	| "<"  { $$ = COND_LT; }
	| "<=" { $$ = COND_LE; }
	| ">"  { $$ = COND_GT; }
	| ">=" { $$ = COND_GE; }
	;

cond
	: "odd" expr
		{ $$ = AST_Cond_create(COND_ODD, $2); }
	| expr cond_op expr
		{ $$ = AST_Cond_create($2, $1, $3); }
	;

expr
	: "+" raw_expr
		{ $$ = $2; }
	| "-" negated_expr
		{ $$ = $2; }
	| raw_expr
		{ $$ = $1; }
	;

expr_op
	: "+" { $$ = EXPR_ADD; }
	| "-" { $$ = EXPR_SUB; }
	;

raw_expr
	: term
		{ $$ = $1; }
	| term expr_op raw_expr
		{ $$ = AST_Expr_create($2, $1, $3); }
	;

negated_expr
	: negated_term
		{ $$ = $1; }
	| negated_term expr_op raw_expr
		{ $$ = AST_Expr_create($2, $1, $3); }
	;

term_op
	: "*" { $$ = EXPR_MUL; }
	| "/" { $$ = EXPR_DIV; }
	| "%" { $$ = EXPR_MOD; }
	;

term
	: factor
		{ $$ = $1; }
	| factor term_op term
		{ $$ = AST_Expr_create($2, $1, $3); }
	;

negated_term
	: negated_factor
		{ $$ = $1; }
	| negated_factor term_op term
		{ $$ = AST_Expr_create($2, $1, $3); }
	;

factor
	: IDENT
		{ $$ = AST_Expr_create(EXPR_VAR, $1); }
	| NUMBER
		{ $$ = AST_Expr_create(EXPR_NUM, $1); }
	| "(" expr ")"
		{ $$ = $2; }
	| "call" IDENT
		{ $$ = AST_Expr_create(EXPR_CALL, $2, NULL); }
	| "call" IDENT "(" params ")"
		{ $$ = AST_Expr_create(EXPR_CALL, $2, $4); }
	;

negated_factor
	: factor
		{ $$ = AST_Expr_create(EXPR_NEG, $1); }
	;

params
	: %empty
		{ $$ = NULL; }
	| params_list
		{ $$ = $1; }
	;

params_list
	: expr
		{ $$ = AST_ParamList_append(AST_ParamList_new(), $1); }
	| params_list "," expr
		{ $$ = AST_ParamList_append($1, $3); }
	;

%%

#include <stdio.h>

void yyerror(yyscan_t scanner, AST_Block** program, char const* msg) {
	(void)scanner;
	(void)program;
	fprintf(stderr, "%s\n", msg);
}
