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
#include <stdio.h>

/* Enum declarations */
typedef enum STMT_TYPE STMT_TYPE;
typedef enum COND_TYPE COND_TYPE;
typedef enum FACT_TYPE FACT_TYPE;

/* Forward declarations */
typedef struct AST_Program AST_Program;
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
typedef struct AST_Number AST_Number;
typedef struct AST_Ident AST_Ident;
typedef struct AST_Call AST_Call;
typedef struct AST_ParamList AST_ParamList;

/* Statement types */
typedef struct Stmt_Assign Stmt_Assign;
typedef struct Stmt_Call Stmt_Call;
typedef struct Stmt_Begin Stmt_Begin;
typedef struct Stmt_If Stmt_If;
typedef struct Stmt_While Stmt_While;
typedef struct Stmt_Read Stmt_Read;
typedef struct Stmt_Write Stmt_Write;

#include "object.h"
#include "graphviz.h"


enum STMT_TYPE {
	STMT_ASSIGN = 1,
	STMT_CALL,
	STMT_BEGIN,
	STMT_IF,
	STMT_WHILE,
	STMT_READ,
	STMT_WRITE,
	STMT_EMPTY
};

enum COND_TYPE {
	COND_ODD = 1,
	COND_EQ,
	COND_NEQ,
	COND_LT,
	COND_LE,
	COND_GT,
	COND_GE
};

enum FACT_TYPE {
	FACT_NULL = 0,
	FACT_IDENT = 1,
	FACT_NUMBER,
	FACT_EXPR,
	FACT_CALL
};


/* Top-level AST nodes */

struct AST_Program {
	OBJECT_BASE;
	
	AST_Block* block;
};
DECL(AST_Program);
void AST_Program_drawGraph(AST_Program* self, Graphviz* gv);


struct AST_Block {
	OBJECT_BASE;

	AST_ConstDecls* consts;       /*!< Optional */
	AST_VarDecls* vars;           /*!< Optional */
	AST_ProcDecls* procs;         /*!< Optional */
	AST_Stmt* stmt;               /*!< Required */
};
DECL(AST_Block);
void AST_Block_drawGraph(AST_Block* self, Graphviz* gv);

struct AST_ConstDecls {
	OBJECT_BASE;
	
	size_t const_count;           /*!< At least one required */
	size_t const_cap;
	AST_Ident** idents;
	AST_Number** values;
};
DECL(AST_ConstDecls);
void AST_ConstDecls_drawGraph(AST_ConstDecls* self, Graphviz* gv);

struct AST_VarDecls {
	OBJECT_BASE;
	
	size_t var_count;             /*!< At least one required */
	size_t var_cap;
	AST_Ident** vars;
};
DECL(AST_VarDecls);
void AST_VarDecls_drawGraph(AST_VarDecls* self, Graphviz* gv);

struct AST_ProcDecls {
	OBJECT_BASE;

	size_t proc_count;            /*!< At least one required */
	size_t proc_cap;
	AST_Proc** procs;
};
DECL(AST_ProcDecls);
void AST_ProcDecls_drawGraph(AST_ProcDecls* self, Graphviz* gv);

struct AST_Proc {
	OBJECT_BASE;
	
	AST_Ident* ident;             /*!< Required */
	AST_Block* body;              /*!< Required */
	AST_ParamDecls* param_decls;  /*!< Required */
};
DECL(AST_Proc);
void AST_Proc_drawGraph(AST_Proc* self, Graphviz* gv);

struct AST_ParamDecls {
	OBJECT_BASE;
	
	size_t param_count;           /*!< Zero or more */
	size_t param_cap;
	AST_Ident** params;
};
DECL(AST_ParamDecls);
void AST_ParamDecls_drawGraph(AST_ParamDecls* self, Graphviz* gv);

struct AST_Stmt {
	OBJECT_BASE;
	
	STMT_TYPE type;
	union {
		Stmt_Assign* assign;
		Stmt_Call* call;
		Stmt_Begin* begin;
		Stmt_If* if_stmt;
		Stmt_While* while_stmt;
		Stmt_Read* read;
		Stmt_Write* write;
	} stmt;                       /*!< Optional (when type == STMT_EMPTY) */
};
DECL(AST_Stmt);
void AST_Stmt_drawGraph(AST_Stmt* self, Graphviz* gv);

struct AST_Cond {
	OBJECT_BASE;
	
	COND_TYPE type;
	AST_Expr* left;               /*!< Required */
	AST_Expr* right;              /*!< Optional (when type == COND_ODD) */
};
DECL(AST_Cond);
void AST_Cond_drawGraph(AST_Cond* self, Graphviz* gv);

struct AST_Expr {
	OBJECT_BASE;
	
	bool a_negative;
	bool subtract;
	AST_Term* left;               /*!< Required */
	AST_Expr* right;              /*!< Optional */
};
DECL(AST_Expr);
void AST_Expr_drawGraph(AST_Expr* self, Graphviz* gv);

struct AST_Term {
	OBJECT_BASE;
	
	bool divide;
	AST_Factor* left;             /*!< Required */
	AST_Term* right;              /*!< Optional */
};
DECL(AST_Term);
void AST_Term_drawGraph(AST_Term* self, Graphviz* gv);

struct AST_Factor {
	OBJECT_BASE;
	
	FACT_TYPE type;
	union {
		AST_Ident* ident;
		AST_Number* number;
		AST_Expr* expr;
		AST_Call* call;
	} value;                      /*!< Required */
};
DECL(AST_Factor);
void AST_Factor_drawGraph(AST_Factor* self, Graphviz* gv);

struct AST_Number {
	OBJECT_BASE;
	
	unsigned num;
};
DECL(AST_Number);
void AST_Number_drawGraph(AST_Number* self, Graphviz* gv);

struct AST_Ident {
	OBJECT_BASE;
	
	char* name;                   /*!< Required */
};
DECL(AST_Ident);
void AST_Ident_drawGraph(AST_Ident* self, Graphviz* gv);

struct AST_Call {
	OBJECT_BASE;
	
	AST_Ident* ident;             /*!< Required */
	AST_ParamList* param_list;    /*!< Required */
};
DECL(AST_Call);
void AST_Call_drawGraph(AST_Call* self, Graphviz* gv);

struct AST_ParamList {
	OBJECT_BASE;
	
	size_t param_count;           /*!< Zero or more */
	size_t param_cap;
	AST_Expr** params;
};
DECL(AST_ParamList);
void AST_ParamList_drawGraph(AST_ParamList* self, Graphviz* gv);


/* Statements */

struct Stmt_Assign {
	OBJECT_BASE;
	
	AST_Ident* ident;             /*!< Required */
	AST_Expr* value;              /*!< Required */
};
DECL(Stmt_Assign);
void Stmt_Assign_drawGraph(Stmt_Assign* self, Graphviz* gv);

struct Stmt_Call {
	OBJECT_BASE;
	
	AST_Ident* ident;             /*!< Required */
	AST_ParamList* param_list;    /*!< Optional */
};
DECL(Stmt_Call);
void Stmt_Call_drawGraph(Stmt_Call* self, Graphviz* gv);

struct Stmt_Begin {
	OBJECT_BASE;
	
	size_t stmt_count;            /*!< At least one required */
	size_t stmt_cap;
	AST_Stmt** stmts;
};
DECL(Stmt_Begin);
void Stmt_Begin_drawGraph(Stmt_Begin* self, Graphviz* gv);

struct Stmt_If {
	OBJECT_BASE;
	
	AST_Cond* cond;               /*!< Required */
	AST_Stmt* then_stmt;          /*!< Required */
	AST_Stmt* else_stmt;          /*!< Optional */
};
DECL(Stmt_If);
void Stmt_If_drawGraph(Stmt_If* self, Graphviz* gv);

struct Stmt_While {
	OBJECT_BASE;
	
	AST_Cond* cond;               /*!< Required */
	AST_Stmt* do_stmt;            /*!< Required */
};
DECL(Stmt_While);
void Stmt_While_drawGraph(Stmt_While* self, Graphviz* gv);

struct Stmt_Read {
	OBJECT_BASE;
	
	AST_Ident* ident;             /*!< Required */
};
DECL(Stmt_Read);
void Stmt_Read_drawGraph(Stmt_Read* self, Graphviz* gv);

struct Stmt_Write {
	OBJECT_BASE;
	
	AST_Ident* ident;             /*!< Required */
};
DECL(Stmt_Write);
void Stmt_Write_drawGraph(Stmt_Write* self, Graphviz* gv);

#endif /* PL0_AST_NODES_H */
