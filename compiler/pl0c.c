//
//  pl0c.c
//  PL/0
//
//  Created by Kevin Colley on 10/22/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "pl0c.h"
#include <stdlib.h>
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "ast_nodes.h"
#include "ast_graph.h"
#include "lexer/pl0lex.h"


Destroyer(CompilerFiles) {
	fclose(self->tokenlist);
	fclose(self->symtab);
	fclose(self->mcode);
	fclose(self->ast);
#if DEBUG
	fclose(self->unoptimized_cfg);
#endif
	fclose(self->cfg);
}
DEF(CompilerFiles);


int run_compiler(CompilerFiles* files, PARSER_TYPE parserType, CODEGEN_TYPE codegenType) {
	int err = EXIT_SUCCESS;
	
	/* Allocate and initialize PL/0 parser object */
	FILE* input_fp = fopen("input.txt", "r");
	if(!input_fp) {
		perror("input.txt");
		return EXIT_FAILURE;
	}
	Lexer* lexer = PL0Lexer_initWithFile(Lexer_alloc(), input_fp);
	Parser* parser = Parser_initWithLexer(Parser_alloc(), lexer, parserType);
	
	/* Parse program */
	AST_Block* prog = NULL;
	bool noSyntaxErrors = Parser_parseProgram(parser, &prog);
	if(!noSyntaxErrors) {
		printf("Stopping due to an earlier parsing error\n");
		err = EXIT_FAILURE;
	}
	else {
		/* Required for HW3 */
		printf("Program is syntactically correct\n");
		
		/* Parser completed without syntax errors, now output AST graph */
		Graphviz* gv = Graphviz_initWithFile(Graphviz_alloc(), files->ast, "AST");
		AST_Block_drawGraph(prog, gv);
		release(&gv);
		
		/* Generate code using the parsed AST of the program */
		Codegen* codegen = Codegen_initWithAST(Codegen_alloc(), prog, codegenType);
		if(codegen == NULL) {
			printf("Stopping due to an earlier codegen error\n");
			err = EXIT_FAILURE;
		}
		else {
			/* Perform optimizations and layout code again, then draw optimized code flow graph */
			Codegen_drawGraph(codegen, files->cfg);
			
			/* Produce the machine code to be executed by the vm and finish the symbol table */
			Codegen_emit(codegen, files->mcode);
			fflush(files->mcode);
			
			/* Produce the symbol "table" output */
			Codegen_writeSymbolTable(codegen, files->symtab);
			fflush(files->symtab);
		}
		
		release(&codegen);
	}
	
	/* Clean up resources */
	release(&prog);
	release(&parser);
	fclose(input_fp);
	return err;
}
