//
//  config.h
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_CONFIG_H
#define PL0_CONFIG_H

#include <stdint.h>

/* Size of the data stack */
#define MAX_STACK_HEIGHT    2000

/* Size of the code section */
#define MAX_CODE_LENGTH     500

/* Maximum number of stack frames (not possible to store more than this) */
#define MAX_LEXI_LEVELS     (MAX_STACK_HEIGHT / 4)


/* Width of the first column (line number) */
/* If this changes, update the table header printing in main.c */
#define DIS_FIRST_COL_WIDTH 4

/* Width of every other column */
/* If this changes, update the table header printing in main.c */
#define DIS_COL_WIDTH       8

/* String length of a line of the disassembly table output including null terminator */
#define DIS_LINE_LENGTH     (5 + DIS_FIRST_COL_WIDTH + 3 * DIS_COL_WIDTH + 1)

/* Data type used for storing memory words on PM/0 architecture */
typedef int32_t Word;

/* Represents an address that is currently undefined */
#define ADDR_UND ((Word)-1)


#endif /* PL0_CONFIG_H */
