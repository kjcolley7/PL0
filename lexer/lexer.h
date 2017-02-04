//
//  lexer.h
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_LEXER_H
#define PL0_LEXER_H

#include <stdio.h>
#include <stddef.h>

typedef struct Lexer Lexer;

#include "object.h"
#include "token.h"
#include "state.h"
#include "transition.h"
#include "graphviz.h"

/* Callback types */
typedef void WhitespaceCB(void* cookie, char ws);

/*! Lexer object */
struct Lexer {
	OBJECT_BASE;
	
	/*! File stream that the lexer scans from */
	FILE* stream;
	
	/*! Finite state machine that makes up the functionality of the lexer */
	State* fsm;
	
	/*! Character buffer for the lexeme of the current token */
	char* lexeme;
	
	/*! Total byte count allocated for the lexeme */
	size_t lexeme_cap;
	
	/*! Current line number (aids in debugging) */
	int line_number;
	
	/*! Whether the lexer has hit end of file */
	bool at_eof;
	
	/*! Callback function called whenever the lexer sees a whitespace character */
	WhitespaceCB* ws_cb;
	
	/*! Pointer passed to both callback functions when they are invoked */
	void* cb_cookie;
};
DECL(Lexer);


/*! Creates a lexer that scans tokens from the provided file stream
 @param stream Input file object that the lexer reads characters from
 */
Lexer* Lexer_initWithStream(Lexer* self, FILE* stream);

/*! Get the state with the exact text as a prefix, creating it if necessary
 @param prefix Exact prefix that leads to the created state
 @return Created state object. Not retained
 */
State* Lexer_getState(Lexer* self, const char* prefix);

/*! Create a simple token that is matched against an exact string
 @param text Exact text of the lexeme to match
 @param type Token type that should be created when the lexer sees this string
 */
void Lexer_addToken(Lexer* self, const char* text, token_type type);

/*! Sets the function pointer that is called whenever whitespace is seen by the lexer
 @param ws_cb Function pointer called whenever the lexer sees a whitespace character
 @param cookie Pointer passed to the callback when it is called
 */
void Lexer_setWhitespaceCallback(Lexer* self, WhitespaceCB* ws_cb, void* cookie);

/*! Scan the next token from the lexer's file stream
 @return Token read in from the input stream, or NULL on error (or after EOF)
 */
Token* Lexer_nextToken(Lexer* self);

/*! Draws the lexer's FSM as a graph
 @param gv Graphviz drawing object
 */
void Lexer_drawGraph(Lexer* self, Graphviz* gv);


#endif /* PL0_LEXER_H */
