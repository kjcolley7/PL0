//
//  lexer.c
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "lexer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>


Destroyer(Lexer) {
	release(&self->fsm);
}
DEF(Lexer);

Lexer* Lexer_initWithFile(Lexer* self, FILE* fin) {
	if((self = Lexer_init(self))) {
		self->line_number = 1;
		self->stream = fin;
		self->fsm = State_new();
		ASSERT(self->fsm != NULL);
	}
	
	return self;
}

static State* getState(State* cur, const char* prefix) {
	/* Base case, token fully matched */
	if(*prefix == '\0') {
		/* At the final state */
		return cur;
	}
	
	/* Check if there is already a state for the next character */
	foreach(&cur->transitions, ptrans) {
		if(prefix[0] == (*ptrans)->exact) {
			/* Found pre-existing state for next character, so descend into that state */
			return getState((*ptrans)->state, &prefix[1]);
		}
	}
	
	/* Creating a new intermediate state */
	State* next = State_initWithLabel(State_alloc(), " ");
	
	/* Transition from the current state to the next one using the current character */
	char* label = strdup_ff((const char[2]){prefix[0], '\0'});
	Transition* trans = Transition_initWithExact(Transition_alloc(), label, next, true, prefix[0]);
	destroy(&label);
	
	/* Release our reference since trans now has a reference to next, but keep a weak reference */
	State* weak_next = next;
	release(&next);
	
	/* Add the transition to the current state */
	State_addTransition(cur, trans);
	
	/* Release our reference since cur now has a reference to trans */
	release(&trans);
	
	/* Recurse until the string is fully traversed */
	return getState(weak_next, &prefix[1]);
}

State* Lexer_getState(Lexer* self, const char* prefix) {
	return getState(self->fsm, prefix);
}

void Lexer_addToken(Lexer* self, const char* text, token_type type) {
	/* Find the state and make it an acceptor */
	State* weak_state = Lexer_getState(self, text);
	State_setLabel(weak_state, text);
	weak_state->acceptor = true;
	weak_state->acceptfn = NULL;
	weak_state->simple_type = type;
}

void Lexer_setWhitespaceCallback(Lexer* self, WhitespaceCB* ws_cb, void* cookie) {
	self->ws_cb = ws_cb;
	self->cb_cookie = cookie;
}

Token* Lexer_nextToken(Lexer* self) {
	/* After the lexer has returned a nulsym token, only return NULL afterwards */
	if(self->at_eof) {
		return NULL;
	}
	
	/* Start the machine at the initial state */
	State* cur = NULL;
	State* next = self->fsm;
	
	/* Reset the lexeme buffer */
	size_t lexeme_end = 0;
	destroy(&self->lexeme);
	self->lexeme_cap = 0;
	self->lexeme = NULL;
	
	/* Keep reading characters until a complete token is scanned */
	int c = ' ';
	while(next != NULL) {
		/* Advance current state */
		cur = next;
		
		/* Read a character from the input stream */
		c = getc(self->stream);
		
		/* Enlarge lexeme buffer if necessary */
		if(lexeme_end == self->lexeme_cap) {
			expand_array(&self->lexeme, &self->lexeme_cap);
		}
		
		/* Store the current character in the next position in the lexeme buffer */
		self->lexeme[lexeme_end++] = c;
		
		/* Try to transition to the next state */
		next = State_transition(cur, c);
		
		/* At initial state and read a character without a matching transition? */
		if(next == NULL && cur == self->fsm) {
			if(isspace(c)) {
				/* Don't include whitespace in a lexeme */
				self->lexeme[--lexeme_end] = '\0';
				
				/* Keep track of line numbers */
				if(c == '\n') {
					++self->line_number;
				}
				
				/* Invoke whitespace callback when we encounter whitespace */
				if(self->ws_cb != NULL) {
					self->ws_cb(self->cb_cookie, c);
				}
				
				/* Stay on the initial state */
				next = cur;
				continue;
			}
			else if(c == EOF) {
				self->lexeme_cap = 0;
				destroy(&self->lexeme);
				self->lexeme = NULL;
				
				/* End of file */
				self->at_eof = true;
				
				/* Return nulsym as an indicator of EOF */
				return Token_initWithType(Token_alloc(), nulsym, "EOF", self->line_number);
			}
		}
	}
	
	/* Scanning of the current lexeme finished, so the current state should be an acceptor */
	if(!cur->acceptor) {
		/* The current state isn't an acceptor state, so this is an error */
		if(isspace(c)) {
			self->lexeme[--lexeme_end] = '\0';
		}
		fprintf(stdout, "Syntax Error [%d]: Unknown sequence: \"%s\"\n", self->line_number, self->lexeme);
		return NULL;
	}
	
	/* Ended lexeme on an acceptor state, so a token was matched */
	ungetc(c, self->stream);
	self->lexeme[--lexeme_end] = '\0';
	
	/* If the state has an acceptor function, call it */
	if(cur->acceptfn != NULL) {
		return cur->acceptfn(self);
	}
	
	/* State doesn't have an acceptor function, so it must be a simple acceptor */
	return Token_initWithType(Token_alloc(), cur->simple_type, self->lexeme, self->line_number);
}

void Lexer_drawGraph(Lexer* self, Graphviz* gv) {
	State_drawGraph(self->fsm, gv);
}
