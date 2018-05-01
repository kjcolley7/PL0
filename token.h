//
//  token.h
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_TOKEN_H
#define PL0_TOKEN_H

#include <stdint.h>
#include <stdio.h>

/*! All possible token types and their values for PL/0 */
typedef enum token_type token_type;
typedef struct Token Token;

#include "object.h"

enum token_type {
	nulsym = 1,    /*!< End of file */
	identsym,      /*!< An identifier (symbol), such as "myVar" */
	numbersym,     /*!< A numeric literal, such as "12345" */
	plussym,       /*!< The addition operator, "+" */
	minussym,      /*!< The subtraction operator, "-" */
	multsym,       /*!< The multiplication operator, "*" */
	slashsym,      /*!< The division operator, "/" */
	oddsym,        /*!< The "odd" keyword */
	eqsym,         /*!< The equality operator, "=" */
	neqsym,        /*!< The inequality operator, "<>" */
	lessym,        /*!< The less than operator, "<" */
	leqsym,        /*!< The less than or equal operator, "<=" */
	gtrsym,        /*!< The greater than operator, ">" */
	geqsym,        /*!< The greater than or equal operator, ">=" */
	lparentsym,    /*!< The left parenthesis character, "(" */
	rparentsym,    /*!< The right parenthesis character, ")" */
	commasym,      /*!< The comma character, "," */
	semicolonsym,  /*!< The semicolon character, ";" */
	periodsym,     /*!< The period character, "." */
	becomessym,    /*!< The assignment operator, ":=" */
	beginsym,      /*!< The "begin" keyword */
	endsym,        /*!< The "end" keyword */
	ifsym,         /*!< The "if" keyword */
	thensym,       /*!< The "then" keyword */
	whilesym,      /*!< The "while" keyword */
	dosym,         /*!< The "do" keyword */
	callsym,       /*!< The "call" keyword */
	constsym,      /*!< The "const" keyword */
	varsym,        /*!< The "var" keyword */
	procsym,       /*!< The "proc" keyword */
	writesym,      /*!< The "write" keyword */
	readsym,       /*!< The "read" keyword */
	elsesym,       /*!< The "else" keyword */
	percentsym     /*!< The modulus operator, "%" */
};

struct Token {
	OBJECT_BASE;
	
	/*! Type of token */
	token_type type;
	
	/*! Raw text that comprises the token */
	char* lexeme;
	
	/*! Source line number where the token came from */
	size_t line_number;
};
DECL(Token);


/*! Base token object initializer
 @param type Type of the token to create
 @param lexeme Raw text that comprises the token
 @param line_number Source line number where the token came from
 */
Token* Token_initWithType(Token* self, token_type type, const char* lexeme, size_t line_number);


#endif /* PL0_TOKEN_H */
