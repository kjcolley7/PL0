//
//  token_stream.c
//  PL/0
//
//  Created by Kevin Colley on 10/28/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "token_stream.h"
#include <stdlib.h>
#include <string.h>
#include "config.h"


Destroyer(TokenStream) {
	release(&self->token);
	release(&self->lexer);
}
DEF(TokenStream);

TokenStream* TokenStream_initWithFile(TokenStream* self, FILE* stream) {
	if((self = TokenStream_init(self))) {
		self->fin = stream;
	}
	
	return self;
}

TokenStream* TokenStream_initWithLexer(TokenStream* self, Lexer* lexer) {
	if((self = TokenStream_init(self))) {
		/* Hold a strong reference to the lexer */
		self->lexer = retain(lexer);
	}
	
	return self;
}

bool TokenStream_peekToken(TokenStream* self, Token** tok) {
	if(self->token != NULL) {
		*tok = self->token;
		return true;
	}
	
	*tok = NULL;
	if(self->lexer != NULL) {
		/* Need to get the next token from the lexer */
		self->token = Lexer_nextToken(self->lexer);
	}
	else {
		/* Need to read a token from the tokenlist file */
		token_type type;
		char lexeme[12] = {};
		
		/* Read in the token type and its lexeme */
		int val;
		if(fscanf(self->fin, "%d", &val) != 1) {
			return false;
		}
		type = val;
		
		switch(type) {
			/* Identifiers and numbers have a lexeme that goes with them */
			case identsym:
			case numbersym:
				/* Read the lexeme (max 11 chars) */
				if(fscanf(self->fin, "%11s", lexeme) != 1) {
					return false;
				}
				break;
				
			case plussym:      strcpy(lexeme, "+");     break;
			case minussym:     strcpy(lexeme, "-");     break;
			case multsym:      strcpy(lexeme, "*");     break;
			case slashsym:     strcpy(lexeme, "/");     break;
			case oddsym:       strcpy(lexeme, "odd");   break;
			case eqsym:        strcpy(lexeme, "=");     break;
			case neqsym:       strcpy(lexeme, "<>");    break;
			case lessym:       strcpy(lexeme, "<");     break;
			case leqsym:       strcpy(lexeme, "<=");    break;
			case gtrsym:       strcpy(lexeme, ">");     break;
			case geqsym:       strcpy(lexeme, ">=");    break;
			case lparentsym:   strcpy(lexeme, "(");     break;
			case rparentsym:   strcpy(lexeme, ")");     break;
			case commasym:     strcpy(lexeme, ",");     break;
			case semicolonsym: strcpy(lexeme, ";");     break;
			case periodsym:    strcpy(lexeme, ".");     break;
			case becomessym:   strcpy(lexeme, ":=");    break;
			case beginsym:     strcpy(lexeme, "begin"); break;
			case endsym:       strcpy(lexeme, "end");   break;
			case ifsym:        strcpy(lexeme, "if");    break;
			case thensym:      strcpy(lexeme, "then");  break;
			case whilesym:     strcpy(lexeme, "while"); break;
			case dosym:        strcpy(lexeme, "do");    break;
			case callsym:      strcpy(lexeme, "call");  break;
			case constsym:     strcpy(lexeme, "const"); break;
			case varsym:       strcpy(lexeme, "var");   break;
			case procsym:      strcpy(lexeme, "proc");  break;
			case writesym:     strcpy(lexeme, "write"); break;
			case readsym:      strcpy(lexeme, "read");  break;
			case elsesym:      strcpy(lexeme, "else");  break;
			
			default:
				printf("Invalid token type: %d\n", type);
				return false;
		}
		
		/* Make sure the lexeme is NULL-terminated */
		lexeme[sizeof(lexeme) - 1] = '\0';
		
		/* Create and return token (line number set to zero since it's not known) */
		self->token = Token_initWithType(Token_alloc(), type, lexeme, 0);
	}
	
	*tok = self->token;
	return true;
}

void TokenStream_consumeToken(TokenStream* self) {
	release(&self->token);
}

#if WITH_BISON
int yylex(YYSTYPE* lvalp, yyscan_t scanner) {
	/* Get next token */
	Token* tok;
	if(!TokenStream_peekToken(scanner, &tok)) {
		return -1;
	}
	
	/* Set lvalp if the token has an associated value */
	int ret = tok->type;
	switch(tok->type) {
		case identsym:
			lvalp->ident = strdup_ff(tok->lexeme);
			break;
		
		case numbersym:
			lvalp->num = (Word)strtoul(tok->lexeme, NULL, 10);
			break;
		
		case nulsym:
			/* EOF */
			ret = 0;
			break;
		
		default:
			break;
	}
	
	/* Release tok */
	TokenStream_consumeToken(scanner);
	return ret;
}
#endif /* WITH_BISON */
