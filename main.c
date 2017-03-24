//
//  main.c
//  PL/0
//
//  Created by Kevin Colley on 11/30/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include <stdio.h>
#include <stdbool.h>
#include "tee.h"
#include "compiler/pl0c.h"
#include "lexer/pl0lex.h"
#include "vm/pm0.h"


/* Lexer files */
static const char* const input_txt = "input.txt";
static const char* const lexemetable_txt = "lexemetable.txt";
static const char* const cleaninput_txt = "cleaninput.txt";
static const char* const tokenlist_txt = "tokenlist.txt";
static const char* const lexer_dot = "lexer.dot";

/* Compiler files */
/* tokenlist.txt already included */
static const char* const symboltable_txt = "symboltable.txt";
static const char* const mcode_txt = "mcode.txt";
static const char* const ast_dot = "ast.dot";
#if DEBUG
static const char* const unoptimized_cfg_dot = "unoptimized_cfg.dot";
#endif
static const char* const cfg_dot = "cfg.dot";

/* VM files */
/* mcode.txt already included */
static const char* const acode_txt = "acode.txt";
static const char* const stacktrace_txt = "stacktrace.txt";


static FILE* check_fopen(const char* fname, const char* mode) {
	FILE* ret = fopen(fname, mode);
	if(ret == NULL) {
		perror(fname);
		exit(EXIT_FAILURE);
	}
	return ret;
}


/* Command line argument option flags */
#define OPT_TEE_TOKLIST   (1<<0)
#define OPT_TEE_SYMTAB    (1<<1)
#define OPT_TEE_DISASM    (1<<2)
#define OPT_TEE_TRACE     (1<<3)
#define OPT_TEE_MCODE     (1<<4)
#define OPT_PRETTY        (1<<5)
#define OPT_SKIP_RUN      (1<<6)
#define OPT_SKIP_COMPILE  (1<<7)
#define OPT_DEBUGGER      (1<<8)


int main(int argc, char* argv[]) {
	/* Flags used to track command line arguments */
	unsigned opts = 0;
	int i, err = EXIT_SUCCESS;
	
	/* Loop through command line arguments */
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "--help") == 0) {
			printf(
				   "Usage: %s [-acdlrsmv]\n"
				   "  -l    Duplicate token list to stdout\n"
				   "  -s    Duplicate symbol table to stdout\n"
				   "  -a    Duplicate disassembly to stdout\n"
				   "  -v    Duplicate program trace to stdout\n"
				   "  -m    Duplicate machine code to stdout\n"
				   "  -p    Pretty print output as Markdown\n"
				   "  -c    Compile only, do not run\n"
				   "  -r    Run only, do not compile\n"
				   "  -d    Run mcode.txt through the PM/0 debugger\n",
				   argv[0]);
			return EXIT_SUCCESS;
		}
		else if(strcmp(argv[i], "-l") == 0) {
			opts |= OPT_TEE_TOKLIST;
		}
		else if(strcmp(argv[i], "-s") == 0) {
			opts |= OPT_TEE_SYMTAB;
		}
		else if(strcmp(argv[i], "-m") == 0) {
			opts |= OPT_TEE_MCODE;
		}
		else if(strcmp(argv[i], "-a") == 0) {
			opts |= OPT_TEE_DISASM;
		}
		else if(strcmp(argv[i], "-v") == 0) {
			opts |= OPT_TEE_TRACE;
		}
		else if(strcmp(argv[i], "-c") == 0) {
			opts |= OPT_SKIP_RUN;
		}
		else if(strcmp(argv[i], "-r") == 0) {
			opts |= OPT_SKIP_COMPILE;
		}
		else if(strcmp(argv[i], "-d") == 0) {
			opts |= OPT_DEBUGGER;
		}
		else {
			printf("Unknown argument: %s\n", argv[i]);
		}
	}
	
	if((opts & OPT_SKIP_COMPILE) && (opts & OPT_SKIP_RUN)) {
		printf("The -c and -r options cannot be combined because there would be nothing left to do\n");
		return EXIT_FAILURE;
	}
	
	/* Don't run the lexer or compiler when told to run only */
	if(!(opts & OPT_SKIP_COMPILE)) {
		/* Create an object used to store the lexer's files */
		LexerFiles* lexerFiles = LexerFiles_new();
		lexerFiles->input = check_fopen(input_txt, "r");
		lexerFiles->table = check_fopen(lexemetable_txt, "w");
		lexerFiles->clean = check_fopen(cleaninput_txt, "w");
		lexerFiles->tokenlist = check_fopen(tokenlist_txt, "w");
		if(opts & OPT_TEE_TOKLIST) {
			/* Duplicate token list to stdout */
			lexerFiles->tokenlist = ftee(lexerFiles->tokenlist, stdout);
		}
		lexerFiles->graph = check_fopen(lexer_dot, "w");
		
		/* Run the lexer */
		err = run_lexer(lexerFiles);
		
		/* Close all the lexer's files */
		release(&lexerFiles);
		
		/* Make sure the lexer completed successfully */
		if(err != 0) {
			return err;
		}
		
		/* Create an object to store the file pointers needed by the compiler */
		CompilerFiles* compilerFiles = CompilerFiles_new();
		compilerFiles->tokenlist = check_fopen(tokenlist_txt, "r");
		compilerFiles->symtab = check_fopen(symboltable_txt, "w");
		if(opts & OPT_TEE_SYMTAB) {
			/* Duplicate symbol table to stdout */
			compilerFiles->symtab = ftee(compilerFiles->symtab, stdout);
		}
		compilerFiles->mcode = check_fopen(mcode_txt, "w");
		if(opts & OPT_TEE_MCODE) {
			/* Duplicate machine code to stdout */
			compilerFiles->mcode = ftee(compilerFiles->mcode, stdout);
		}
		compilerFiles->ast = check_fopen(ast_dot, "w");
	#if DEBUG
		compilerFiles->unoptimized_cfg = check_fopen(unoptimized_cfg_dot, "w");
	#endif
		compilerFiles->cfg = check_fopen(cfg_dot, "w");
		
		/* Compile the tokens the lexer scanned from the source code into the machine code */
		err = run_compiler(compilerFiles);
		
		/* Close all the compiler's files */
		release(&compilerFiles);
		
		/* Make sure compilation was successful */
		if(err != 0) {
			return err;
		}
	}
	
	if(!(opts & OPT_SKIP_RUN)) {
		/* Create an object to hold the files needed by the VM */
		VMFiles* vmFiles = VMFiles_new();
		vmFiles->mcode = check_fopen(mcode_txt, "r");
		vmFiles->acode = check_fopen(acode_txt, "w");
		if(opts & OPT_TEE_DISASM) {
			/* Duplicate the disassembled code file to stdout */
			vmFiles->acode = ftee(vmFiles->acode, stdout);
		}
		vmFiles->stacktrace = check_fopen(stacktrace_txt, "w");
		if(opts & OPT_TEE_TRACE) {
			/* Duplicate the stacktrace file to stdout */
			vmFiles->stacktrace = ftee(vmFiles->stacktrace, stdout);
		}
		
		/* Run the VM on the compiled machine code */
		err = run_vm(vmFiles, !!(opts & OPT_PRETTY), !!(opts & OPT_DEBUGGER));
		
		/* Close the VM's files */
		release(&vmFiles);
	}
	
	return err;
}
