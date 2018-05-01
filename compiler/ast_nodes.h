//
//  ast_nodes.h
//  PL/0
//
//  Created by Kevin Colley on 10/26/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

/****************************************************************************************\
 * Just a bunch of boilerplate code to make AST node objects that manage memory nicely. *
 * Should be fairly straightforward, hence the lack of commenting.                      *
\****************************************************************************************/

#ifndef PL0_AST_NODES_H
#define PL0_AST_NODES_H

#include <stddef.h>
#include <stdbool.h>
#include "object.h"
#include "config.h"

/* Enum declarations */
typedef enum STMT_TYPE STMT_TYPE;
typedef enum COND_TYPE COND_TYPE;
typedef enum EXPR_TYPE EXPR_TYPE;

/* Forward declarations */
typedef struct AST_Block AST_Block;
typedef struct AST_ConstDecls AST_ConstDecls;
typedef struct AST_VarDecls AST_VarDecls;
typedef struct AST_ProcDecls AST_ProcDecls;
typedef struct AST_Proc AST_Proc;
typedef struct AST_ParamDecls AST_ParamDecls;
typedef struct AST_Stmt AST_Stmt;
typedef struct AST_Cond AST_Cond;
typedef struct AST_Expr AST_Expr;
typedef struct AST_Term AST_Term;
typedef struct AST_Factor AST_Factor;
typedef struct AST_ParamList AST_ParamList;


enum STMT_TYPE {
	STMT_UNINITIALIZED = 0,
	STMT_ASSIGN = 1,
	STMT_CALL,
	STMT_BEGIN,
	STMT_IF,
	STMT_WHILE,
	STMT_READ,
	STMT_WRITE
};

enum COND_TYPE {
	COND_UNINITIALIZED = 0,
	COND_ODD = 1,
	COND_EQ,
	COND_NE,
	COND_LT,
	COND_LE,
	COND_GT,
	COND_GE
};

enum EXPR_TYPE {
	EXPR_UNINITIALIZED = 0,
	EXPR_VAR = 1,
	EXPR_NUM,
	EXPR_NEG,
	EXPR_ADD,
	EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,
	EXPR_MOD,
	EXPR_CALL
};


/* AST node structures */

struct AST_Block {
	OBJECT_BASE;

	AST_ConstDecls* consts;             /*!< Optional */
	AST_VarDecls* vars;                 /*!< Optional */
	AST_ProcDecls* procs;               /*!< Optional */
	AST_Stmt* stmt;                     /*!< Optional */
};
DECL(AST_Block);

struct AST_ConstDecls {
	OBJECT_BASE;
	
	dynamic_array(struct {
		char* ident;
		Word value;
	}) consts;                          /*!< At least one required */
};
DECL(AST_ConstDecls);

struct AST_VarDecls {
	OBJECT_BASE;
	
	dynamic_array(char*) vars;          /*!< At least one required */
};
DECL(AST_VarDecls);

struct AST_ProcDecls {
	OBJECT_BASE;
	
	dynamic_array(AST_Proc*) procs;     /*!< At least one required */
};
DECL(AST_ProcDecls);

struct AST_Proc {
	OBJECT_BASE;
	
	char* ident;                        /*!< Required */
	AST_ParamDecls* param_decls;        /*!< Required */
	AST_Block* body;                    /*!< Required */
};
DECL(AST_Proc);

struct AST_ParamDecls {
	OBJECT_BASE;
	
	dynamic_array(char*) params;        /*!< Zero or more */
};
DECL(AST_ParamDecls);

struct AST_Stmt {
	OBJECT_BASE;
	
	STMT_TYPE type;
	union {
		struct {
			char* ident;
			AST_Expr* value;
		} assign;                       /*!< STMT_ASSIGN */
		struct {
			char* ident;
			AST_ParamList* param_list;
		} call;                         /*!< STMT_CALL */
		struct {
			dynamic_array(AST_Stmt*) stmts;
		} begin;                        /*!< STMT_BEGIN */
		struct {
			AST_Cond* cond;
			AST_Stmt* then_stmt;
			AST_Stmt* else_stmt;
		} if_stmt;                      /*!< STMT_IF */
		struct {
			AST_Cond* cond;
			AST_Stmt* do_stmt;
		} while_stmt;                   /*!< STMT_WHILE */
		struct {
			char* ident;
		} read;                         /*!< STMT_READ */
		struct {
			char* ident;
		} write;                        /*!< STMT_WRITE */
	} stmt;                             /*!< Required */
};
DECL(AST_Stmt);

struct AST_Cond {
	OBJECT_BASE;
	
	COND_TYPE type;
	union {
		AST_Expr* operand;              /*!< COND_ODD (unary operator) */
		struct {
			AST_Expr* left;
			AST_Expr* right;
		} binop;                        /*!< COND_{EQ,NE,LT,LE,GT,GE} (binary operators) */
	} values;                           /*!< Required */
};
DECL(AST_Cond);

struct AST_Expr {
	OBJECT_BASE;
	
	EXPR_TYPE type;
	union {
		char* ident;                    /*!< EXPR_VAR (variable name) */
		Word num;                       /*!< EXPR_NUM (integer literal) */
		AST_Expr* operand;              /*!< EXPR_NEG (unary operator) */
		struct {
			AST_Expr* left;
			AST_Expr* right;
		} binop;                        /*!< EXPR_{ADD,SUB,MUL,DIV} (binary operators) */
		struct {
			char* ident;
			AST_ParamList* param_list;
		} call;                         /*!< EXPR_CALL (procedure call) */
	} values;                           /*!< Required */
};
DECL(AST_Expr);

struct AST_ParamList {
	OBJECT_BASE;
	
	dynamic_array(AST_Expr*) params;    /*!< Zero or more */
};
DECL(AST_ParamList);


/* Additional AST helper methods */

/*! Create an AST node for a block with the provided child nodes
 @param consts List of constant declarations
 @param vars List of variable declarations
 @param procs List of procedure declarations
 @param stmt Body of the block
 @return AST node for a block
 */
AST_Block* AST_Block_create(AST_ConstDecls* consts, AST_VarDecls* vars, AST_ProcDecls* procs, AST_Stmt* stmt);

/*! Append a constant declaration to a list
 @param ident Name of constant
 @param value Value of constant
 @return self
 */
AST_ConstDecls* AST_ConstDecls_append(AST_ConstDecls* self, char* ident, Word value);

/*! Append a variable declaration to a list
 @param ident Name of variable
 @return self
 */
AST_VarDecls* AST_VarDecls_append(AST_VarDecls* self, char* ident);

/*! Append a procedure declaration to a list
 @param ident Name of procedure
 @param param_decls List of parameter declarations
 @param body Block making up the procedure's body
 @return self
 */
AST_ProcDecls* AST_ProcDecls_append(AST_ProcDecls* self, char* ident, AST_ParamDecls* param_decls, AST_Block* body);

/*! Append a parameter declaration to a list
 @param ident Name of parameter
 @return self
 */
AST_ParamDecls* AST_ParamDecls_append(AST_ParamDecls* self, char* ident);

/*! Create an AST node for a statement with the provided type and child nodes
 @param type Statement type
 @note Further parameters depend on the statement type
 @return AST node for a statement
 */
AST_Stmt* AST_Stmt_create(STMT_TYPE type, ...);

/*! Append a statement to a begin statement
 @param self Statement of type STMT_BEGIN (will be checked with an assertion in debug builds)
 @param stmt Statement to append to this begin statement
 @return self
 */
AST_Stmt* AST_Stmt_append(AST_Stmt* self, AST_Stmt* stmt);

/*! Create an AST node for a condition with the provided type and child nodes
 @param type Condition type
 @note Further parameters depend on the condition type
 @return AST node for a condition
 */
AST_Cond* AST_Cond_create(COND_TYPE type, ...);

/*! Create an AST node for an expression with the provided type and child nodes
 @param type Expression type
 @note Further parameters depend on the expression type
 @return AST node for an expression
 */
AST_Expr* AST_Expr_create(EXPR_TYPE type, ...);

/*! Applies a unary operator (+/-) to an expression
 @param unary_op Either EXPR_ADD (+) or EXPR_SUB (-)
 @return New EXPR_NEG object if unary_op was EXPR_SUB, otherwise just self
 */
AST_Expr* AST_Expr_applyUnaryOperator(AST_Expr* self, EXPR_TYPE unary_op);

/*! Append an expression to a parameter list
 @param expr Expression to compute the parameter value
 @return self
 */
AST_ParamList* AST_ParamList_append(AST_ParamList* self, AST_Expr* expr);

#endif /* PL0_AST_NODES_H */
