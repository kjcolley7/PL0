//
//  pm0.c
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#include "pm0.h"
#include "config.h"
#include "machine.h"
#include "instruction.h"
#include "tee.h"
#include "debugengine.h"


Destroyer(VMFiles) {
	fclose(self->mcode);
	fclose(self->acode);
	fclose(self->stacktrace);
}
DEF(VMFiles);


int run_vm(VMFiles* files, bool markdown, bool debug) {
	/* Create virtual machine */
	Machine* cpu = Machine_initWithPorts(Machine_alloc(), stdin, stdout);
	
	/* Markdown formatted output */
	if(markdown) {
		Machine_enableMarkdown(cpu);
	}
	
	/* Load the code from the specified file into code memory (and disassemble it) */
	if(!Machine_loadCode(cpu, files->mcode)) {
		release(&cpu);
		return EXIT_FAILURE;
	}
	
	/* Write disassembly table to the stacktrace file */
	Machine_printDisassembly(cpu, files->acode);
	fflush(files->acode);
	
	/* Enable logging to the stacktrace file */
	Machine_setLogFile(cpu, files->stacktrace);
	
	bool success;
	if(debug) {
		/* Create and run the debugger */
		DebugEngine* dbg = DebugEngine_initWithCPU(DebugEngine_alloc(), cpu);
		success = DebugEngine_run(dbg);
	}
	else {
		/* Begin execution */
		success = Machine_run(cpu);
	}
	
	/* Clean up resources and exit */
	release(&cpu);
	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
