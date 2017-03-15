//
//  parser.h
//  PL/0
//
//  Created by Kevin Colley on 10/22/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_PARSER_H
#define PL0_PARSER_H

#include <stdbool.h>

typedef struct Parser Parser;

#include "object.h"
#include "token.h"
#include "compiler/ast_nodes.h"
#include "token_stream.h"
#include "lexer/lexer.h"

struct Parser {
	OBJECT_BASE;
	
	/*! File stream that tokens are read in from */
	TokenStream* token_stream;
};
DECL(Parser);


/*! Initializes a parser with a stream of tokens
 @param fin File stream that tokens are read in from
 */
Parser* Parser_initWithFile(Parser* self, FILE* fin);

/*! Initializes a parser with a lexer to read tokens from
 @param lexer Lexer used to stream tokens from
 */
Parser* Parser_initWithLexer(Parser* self, Lexer* lexer);

/*! Initializes a parser with a token stream
 @param stream Token stream to read from
 */
Parser* Parser_initWithStream(Parser* self, TokenStream* stream);

/*! Parses a program from the parser's input file stream and returns the AST
 @param program Out pointer to abstract syntax tree representing the syntax of
                the entire program, or NULL on error
 @return True on success or false on error
 */
bool Parser_parseProgram(Parser* self, AST_Block** program);


#endif /* PL0_PARSER_H */
