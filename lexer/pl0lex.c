//
//  pl0lex.c
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "pl0lex.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lexer.h"
#include "graphviz.h"


Destroyer(LexerFiles) {
	fclose(self->input);
	fclose(self->table);
	fclose(self->clean);
	fclose(self->tokenlist);
	fclose(self->graph);
}
DEF(LexerFiles);


static Token* accept_comment(Lexer* lexer);
static Token* accept_identifier(Lexer* lexer);
static Token* accept_number(Lexer* lexer);
static Token* accept_invalid_varname(Lexer* lexer);
static bool match_ident_begin(char c);
static bool match_ident_middle(char c);
static bool match_number(char c);
static void add_identifier_transitions(State* cur, Transition* trans);
static void add_pl0_tokens(Lexer* lexer);
static void writeWhitespace(void* cookie, char c);


int run_lexer(LexerFiles* files) {
	/* Create the PL/0 lexer instance */
	Lexer* lexer = PL0Lexer_initWithFile(Lexer_alloc(), files->input);
	
	/* Make sure that we write the whitespace ignored by the lexer to the clean source file */
	Lexer_setWhitespaceCallback(lexer, &writeWhitespace, files);
	
	/* Draw lexer graph */
	Graphviz* gv = Graphviz_initWithFile(Graphviz_alloc(), files->graph, "Lexer");
	if(gv != NULL) {
		Lexer_drawGraph(lexer, gv);
		release(&gv);
	}
	
	/* Print the lexeme table header */
	fprintf(files->table, "lexeme\ttoken type\n");
	
	/* Scan all tokens */
	Token* tok;
	bool first = true;
	int err = EXIT_FAILURE;
	while((tok = Lexer_nextToken(lexer))) {
		/* nulsym is used as the EOF token, null terminates the token stream */
		if(tok->type == nulsym) {
			/* All done! (successfully) */
			err = EXIT_SUCCESS;
			release(&tok);
			break;
		}
		
		/* Print to the clean source file */
		fprintf(files->clean, "%s", tok->lexeme);
		
		/* Print to the lexeme table */
		fprintf(files->table, "%s\t%d\n", tok->lexeme, tok->type);
		
		/* Print to the token list */
		fprintf(files->tokenlist, "%s%d", first ? "" : " ", tok->type);
		first = false;
		if(tok->type == numbersym || tok->type == identsym) {
			fprintf(files->tokenlist, " %s", tok->lexeme);
		}
		
		/* Done processing this token so release our reference to it so it can be freed */
		release(&tok);
	}
	
	/* End the tokenlist file with a newline */
	fprintf(files->tokenlist, "\n");
	fflush(files->tokenlist);
	
	return err;
}

Lexer* PL0Lexer_initWithFile(Lexer* self, FILE* fin) {
	if((self = Lexer_initWithFile(self, fin))) {
		/* Add PL/0's tokens to the lexer */
		add_pl0_tokens(self);
	}
	
	return self;
}

static Token* accept_comment(Lexer* lexer) {
	/* Ignore all characters until the following two characters are found: */
	int c = getc(lexer->stream);
	do {
		while(c != '*') {
			if(c == EOF) {
				break;
			}
			else if(c == '\n') {
				++lexer->line_number;
			}
			c = getc(lexer->stream);
		}
		
		c = getc(lexer->stream);
		if(c == EOF) {
			break;
		}
		else if(c == '\n') {
			++lexer->line_number;
		}
	} while(c != '/');
	
	if(c == EOF) {
		fprintf(stdout, "Syntax Error on line %d: End of file occurred within a comment.\n", lexer->line_number);
		return NULL;
	}
	
	/* Need to return a token, so invoke the lexer again */
	return Lexer_nextToken(lexer);
}

static Token* accept_identifier(Lexer* lexer) {
	/* Make sure the identifier doesn't have more than 1 characters */
	if(string_length(&lexer->lexeme) > 11) {
		fprintf(stdout, "Syntax Error on line %d: Identifier cannot be longer than 11 characters: \"%s\"\n",
		        lexer->line_number, string_cstr(&lexer->lexeme));
		return NULL;
	}
	
	/* Return an identifier token normally */
	return Token_initWithType(Token_alloc(), identsym, string_cstr(&lexer->lexeme), lexer->line_number);
}

static Token* accept_number(Lexer* lexer) {
	/* Make sure the number doesn't have more than 5 digits */
	if(string_length(&lexer->lexeme) > 5) {
		fprintf(stdout, "Syntax Error on line %d: Number literal cannot be longer than 5 digits: \"%s\"\n",
		        lexer->line_number, string_cstr(&lexer->lexeme));
		return NULL;
	}
	
	/* Return a number token normally */
	return Token_initWithType(Token_alloc(), numbersym, string_cstr(&lexer->lexeme), lexer->line_number);
}

static Token* accept_invalid_varname(Lexer* lexer) {
	fprintf(stdout, "Syntax Error on line %d: Invalid identifier: \"%s\"\n", lexer->line_number, string_cstr(&lexer->lexeme));
	return NULL;
}

static bool match_ident_begin(char c) {
	return isalpha(c);
}

static bool match_ident_middle(char c) {
	return isalnum(c);
}

static bool match_number(char c) {
	return isdigit(c);
}

static void add_identifier_transitions(State* cur, Transition* trans) {
	foreach(&cur->transitions, pnext) {
		Transition* next = *pnext;
		
		/* This code will break if we ever add a reserved word with numbers in it */
		if(next->matcher == NULL && isalpha(next->exact)) {
			/* Recurse to all later states with alphabetical prefixes */
			add_identifier_transitions(next->state, trans);
			
			/* Add a fallback transition from the state with an alphabetic prefix to trans */
			State_addTransition(next->state, trans);
			
			/* Make this an acceptor state if it isn't already */
			if(!next->state->acceptor) {
				State_setLabel(next->state, "ID");
				next->state->acceptor = true;
				next->state->acceptfn = trans->state->acceptfn;
				next->state->simple_type = trans->state->simple_type;
			}
		}
	}
}

static void add_pl0_tokens(Lexer* lexer) {
	/* Label the initial state */
	State* first_state = Lexer_getState(lexer, "");
	State_setLabel(first_state, "START");
	
	/* Recognize the tail of an identifier, [a-zA-Z0-9]* (weakly referenced) */
	State* state_ident_middle = State_initWithAcceptor(State_alloc(), "ID", &accept_identifier);
	Transition* trans_ident_middle = Transition_initWithMatcher(
		Transition_alloc(),
		"[a-zA-Z0-9]",
		state_ident_middle,
		false,
		&match_ident_middle);
	State_addTransition(state_ident_middle, trans_ident_middle);
	
	/* Punctuation, matching exactly */
	Lexer_addToken(lexer, "+", plussym);
	Lexer_addToken(lexer, "-", minussym);
	Lexer_addToken(lexer, "*", multsym);
	Lexer_addToken(lexer, "/", slashsym);
	Lexer_addToken(lexer, "%", percentsym);
	Lexer_addToken(lexer, "=", eqsym);
	Lexer_addToken(lexer, "<", lessym);
	Lexer_addToken(lexer, "<>", neqsym);
	Lexer_addToken(lexer, "<=", leqsym);
	Lexer_addToken(lexer, ">", gtrsym);
	Lexer_addToken(lexer, ">=", geqsym);
	Lexer_addToken(lexer, "(", lparentsym);
	Lexer_addToken(lexer, ")", rparentsym);
	Lexer_addToken(lexer, ",", commasym);
	Lexer_addToken(lexer, ";", semicolonsym);
	Lexer_addToken(lexer, ".", periodsym);
	Lexer_addToken(lexer, ":=", becomessym);
	
	/* Reserved words. When only a prefix matches, fall back to recognizing it as an identifier */
	Lexer_addToken(lexer, "begin", beginsym);
	Lexer_addToken(lexer, "call", callsym);
	Lexer_addToken(lexer, "const", constsym);
	Lexer_addToken(lexer, "do", dosym);
	Lexer_addToken(lexer, "else", elsesym);
	Lexer_addToken(lexer, "end", endsym);
	Lexer_addToken(lexer, "if", ifsym);
	Lexer_addToken(lexer, "odd", oddsym);
	Lexer_addToken(lexer, "procedure", procsym);
	Lexer_addToken(lexer, "read", readsym);
	Lexer_addToken(lexer, "then", thensym);
	Lexer_addToken(lexer, "var", varsym);
	Lexer_addToken(lexer, "while", whilesym);
	Lexer_addToken(lexer, "write", writesym);
	
	/* First character of identifiers, matching [a-zA-Z] (strongly referenced) */
	Transition* trans_ident_begin = Transition_initWithMatcher(
		Transition_alloc(),
		"[a-zA-Z]",
		state_ident_middle,
		true,
		&match_ident_begin);
	State_addTransition(first_state, trans_ident_begin);
	
	/* Add transitions from each partial reserved word to identsym, for example "white" */
	add_identifier_transitions(first_state, trans_ident_middle);
	
	/* Numbers, matching [0-9]+ (weakly referenced) */
	State* state_number = State_initWithAcceptor(State_alloc(), "NUMBER", &accept_number);
	Transition* weak_trans_number = Transition_initWithMatcher(
		Transition_alloc(),
		"[0-9]",
		state_number,
		false,
		&match_number);
	State_addTransition(state_number, weak_trans_number);
	
	/* So that first_state indirectly holds a strong reference to state_number */
	Transition* trans_number = Transition_initWithMatcher(
		Transition_alloc(),
		"[0-9]",
		state_number,
		true,
		&match_number);
	State_addTransition(first_state, trans_number);
	
	/* Invalid variable names, matching [0-9]+[a-zA-Z] (strongly referenced) */
	State* state_invalid_varname = State_initWithAcceptor(State_alloc(), "INVALID", &accept_invalid_varname);
	Transition* trans_invalid_varname = Transition_initWithMatcher(
		Transition_alloc(),
		"[a-zA-Z]",
		state_invalid_varname,
		true,
		&match_ident_begin);
	State_addTransition(state_number, trans_invalid_varname);
	
	/* Comments, like this one */
	State* state_comment = Lexer_getState(lexer, "/*");
	State_setLabel(state_comment, "COMMENT");
	state_comment->acceptor = true;
	state_comment->acceptfn = &accept_comment;
	
	/* Cleanup, since the lexer now holds references to all of these */
	release(&trans_invalid_varname);
	release(&state_invalid_varname);
	release(&trans_number);
	release(&weak_trans_number);
	release(&state_number);
	release(&trans_ident_begin);
	release(&trans_ident_middle);
	release(&state_ident_middle);
}

static void writeWhitespace(void* cookie, char c) {
	LexerFiles* files = cookie;
	fputc(c, files->clean);
}
