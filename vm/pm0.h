//
//  pm0.h
//  PL/0
//
//  Created by Kevin Colley on 11/30/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_PM0_H
#define PL0_PM0_H

#include <stdio.h>
#include <stdbool.h>

typedef struct VMFiles VMFiles;

#include "object.h"

struct VMFiles {
	OBJECT_BASE;
	
	FILE* mcode;
	FILE* acode;
	FILE* stacktrace;
};
DECL(VMFiles);


/*! Runs the PM/0 vm using the given file streams with optional settings
 @param files Open file streams used by the vm
 @param markdown True if the stacktrace and disassembly should be in markdown format
 @param debug True if the PM/0 debugger should be used when running the program
 @return Zero on success, or nonzero on error
 */
int run_vm(VMFiles* files, bool markdown, bool debug);


#endif /* PL0_PM0_H */
