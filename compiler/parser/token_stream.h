//
//  token_stream.h
//  PL/0
//
//  Created by Kevin Colley on 10/28/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_TOKEN_STREAM_H
#define PL0_TOKEN_STREAM_H

#include <stdio.h>
#include <stdbool.h>

typedef struct TokenStream TokenStream;

#include "object.h"
#include "token.h"
#include "lexer/lexer.h"

struct TokenStream {
	OBJECT_BASE;
	
	/*! The stream's current token */
	Token* token;
	
	/*! File stream to read tokens from */
	FILE* fin;
	
	/*! Lexer to read tokens from */
	Lexer* lexer;
};
DECL(TokenStream);


/*! Initializes a TokenStream object to read tokens from the specified stream
 @param stream File stream to read tokens from
 */
TokenStream* TokenStream_initWithStream(TokenStream* self, FILE* stream);

/*! Initializes a TokenStream object to read tokens from the specified lexer
 @param lexer Lexer to read tokens from
 */
TokenStream* TokenStream_initWithLexer(TokenStream* self, Lexer* lexer);

/*! Get a pointer to the current token being read from the stream
 @param tok Out pointer to the token to read
 @return True on success, or false on error
 */
bool TokenStream_peekToken(TokenStream* self, Token** tok);

/*! Consumes the current token and releases the stream's reference to it */
void TokenStream_consumeToken(TokenStream* self);


#endif /* PL0_TOKEN_STREAM_H */
