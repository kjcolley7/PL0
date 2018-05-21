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
#include <inttypes.h>

/* Size of the data stack */
#define MAX_STACK_HEIGHT    ((Word)2000)

/* Size of the code section */
#define MAX_CODE_LENGTH     ((Word)500)

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
#define WORD_MIN INT32_MIN
#define WORD_MAX INT32_MAX

/* Helper macros for printing/scanning a Word */
#define PRIdWORD PRId32
#define PRIxWORD PRIx32
#define SCNdWORD SCNd32
#define SCNxWORD SCNx32

/* Represents an address that is currently undefined */
#define ADDR_UND ((Word)-1)

/* Colors for instruction groups */
#define CAL_COLOR   "magenta"
#define JMP_COLOR   "blue"
#define LDS_COLOR   "forestgreen"
#define RET_COLOR   "red"
#define ARITH_COLOR "orange"
#define COND_COLOR  "purple"
#define IO_COLOR    "brown"
#define LIT_COLOR   "dodgerblue"
#define INC_COLOR   "tan"
#define ERR_COLOR   "deeppink"

/* Colors and fonts for AST nodes */
#define NONTERMINAL_FONT  "Times-Italic"
#define TERMINAL_FONT     "Courier-Bold"
#define FACE_NONTERMINAL  "face=\"" NONTERMINAL_FONT "\""
#define FACE_TERMINAL     "face=\"" TERMINAL_FONT "\""
#define COLOR_NUM         "color=\"" LIT_COLOR "\""
#define COLOR_VAR         "color=\"" LDS_COLOR "\""
#define COLOR_PROC        "color=\"" CAL_COLOR "\""


#endif /* PL0_CONFIG_H */
