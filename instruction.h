//
//  instruction.h
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_INSTRUCTION_H
#define PL0_INSTRUCTION_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct Insn Insn;

#include "config.h"

/* Not using an enum so I can specify the underlying type */
typedef uint16_t Opcode;
#define OP_BREAK   0
#define OP_LIT     1
#define OP_OPR     2
#define OP_LOD     3
#define OP_STO     4
#define OP_CAL     5
#define OP_INC     6
#define OP_JMP     7
#define OP_JPC     8
#define OP_SIO     9
#define OP_COUNT   (OP_SIO + 1)

/* ALU operation codes */
#define ALU_RET    0    /* Not really an ALU op but easier this way */
#define ALU_NEG    1
#define ALU_ADD    2
#define ALU_SUB    3
#define ALU_MUL    4
#define ALU_DIV    5
#define ALU_ODD    6
#define ALU_MOD    7
#define ALU_EQL    8
#define ALU_NEQ    9
#define ALU_LSS    10
#define ALU_LEQ    11
#define ALU_GTR    12
#define ALU_GEQ    13
#define ALU_COUNT  (ALU_GEQ + 1)

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


/*! Defines the format of a PM/0 instruction (64-bits) */
struct Insn {
	Opcode op;         /*!< The instruction type (OP) */
	uint16_t lvl;      /*!< Lexicographical level (L) */
	Word imm;          /*!< Immediate operand (M) */
};

/* Pseudoinstruction used as a breakpoint */
#define IS_BREAK(insn)  ((insn).op == OP_BREAK && (insn).lvl != 0)
#define MAKE_BREAK(id)  ((Insn){OP_BREAK, 1, (id)})

/*! Convenience macros for building instructions */
#define MAKE_LIT(val)   ((Insn){OP_LIT, 0, (val)})
#define MAKE_OPR(aluop) ((Insn){OP_OPR, 0, (aluop)})
#define MAKE_LOD(L, M)  ((Insn){OP_LOD, (L), (M)})
#define MAKE_STO(L, M)  ((Insn){OP_STO, (L), (M)})
#define MAKE_CAL(L, M)  ((Insn){OP_CAL, (L), (M)})
#define MAKE_INC(M)     ((Insn){OP_INC, 0, (M)})
#define MAKE_JMP(addr)  ((Insn){OP_JMP, 0, (addr)})
#define MAKE_JPC(addr)  ((Insn){OP_JPC, 0, (addr)})
#define MAKE_WRITE()    ((Insn){OP_SIO, 0, 1})
#define MAKE_READ()     ((Insn){OP_SIO, 0, 2})
#define MAKE_HALT()     ((Insn){OP_SIO, 0, 3})

#define MAKE_RET()      MAKE_OPR(ALU_RET)
#define MAKE_NEG()      MAKE_OPR(ALU_NEG)
#define MAKE_ADD()      MAKE_OPR(ALU_ADD)
#define MAKE_SUB()      MAKE_OPR(ALU_SUB)
#define MAKE_MUL()      MAKE_OPR(ALU_MUL)
#define MAKE_DIV()      MAKE_OPR(ALU_DIV)
#define MAKE_ODD()      MAKE_OPR(ALU_ODD)
#define MAKE_MOD()      MAKE_OPR(ALU_MOD)
#define MAKE_EQL()      MAKE_OPR(ALU_EQL)
#define MAKE_NEQ()      MAKE_OPR(ALU_NEQ)
#define MAKE_LSS()      MAKE_OPR(ALU_LSS)
#define MAKE_LEQ()      MAKE_OPR(ALU_LEQ)
#define MAKE_GTR()      MAKE_OPR(ALU_GTR)
#define MAKE_GEQ()      MAKE_OPR(ALU_GEQ)

/* Inverts a conditional instruction (excluding ODD) */
#define MAKE_INV(cond) \
((cond).imm == ALU_EQL ? MAKE_NEQ() : \
 (cond).imm == ALU_NEQ ? MAKE_EQL() : \
 (cond).imm == ALU_GTR ? MAKE_LEQ() : \
 (cond).imm == ALU_LEQ ? MAKE_GTR() : \
 (cond).imm == ALU_LSS ? MAKE_GEQ() : \
 (cond).imm == ALU_GEQ ? MAKE_LSS() : \
 (Insn){0, 0, 0} /* Illegal instruction */)


/*! Emits the instruction to the specified file stream
 @param fp File stream where the instruction will be emitted
 */
void Insn_emit(Insn insn, FILE* fp);

/* Convenience macros used for building and emitting instructions */
#define EMIT(insn, fp)      Insn_emit(insn, fp)
#define EMIT_LIT(val, fp)   EMIT(MAKE_LIT(val), fp)
#define EMIT_LOD(L, M, fp)  EMIT(MAKE_LOD(L, M), fp)
#define EMIT_STO(L, M, fp)  EMIT(MAKE_STO(L, M), fp)
#define EMIT_CAL(L, M, fp)  EMIT(MAKE_CAL(L, M), fp)
#define EMIT_INC(M, fp)     EMIT(MAKE_INC(M), fp)
#define EMIT_JMP(addr, fp)  EMIT(MAKE_JMP(addr), fp)
#define EMIT_JPC(addr, fp)  EMIT(MAKE_JPC(addr), fp)
#define EMIT_WRITE(fp)      EMIT(MAKE_WRITE(), fp)
#define EMIT_READ(fp)       EMIT(MAKE_READ(), fp)
#define EMIT_HALT(fp)       EMIT(MAKE_HALT(), fp)

#define EMIT_RET(fp)        EMIT(MAKE_RET(), fp)
#define EMIT_NEG(fp)        EMIT(MAKE_NEG(), fp)
#define EMIT_ADD(fp)        EMIT(MAKE_ADD(), fp)
#define EMIT_SUB(fp)        EMIT(MAKE_SUB(), fp)
#define EMIT_MUL(fp)        EMIT(MAKE_MUL(), fp)
#define EMIT_DIV(fp)        EMIT(MAKE_DIV(), fp)
#define EMIT_ODD(fp)        EMIT(MAKE_ODD(), fp)
#define EMIT_MOD(fp)        EMIT(MAKE_MOD(), fp)
#define EMIT_EQL(fp)        EMIT(MAKE_EQL(), fp)
#define EMIT_NEQ(fp)        EMIT(MAKE_NEQ(), fp)
#define EMIT_LSS(fp)        EMIT(MAKE_LSS(), fp)
#define EMIT_LEQ(fp)        EMIT(MAKE_LEQ(), fp)
#define EMIT_GTR(fp)        EMIT(MAKE_GTR(), fp)
#define EMIT_GEQ(fp)        EMIT(MAKE_GEQ(), fp)

/* Invert a conditional instruction (excluding ODD) */
#define EMIT_INV(cond, fp)  EMIT(MAKE_INV(cond), fp)


/*! Read an entire PM/0 program from the text file into the code array given
 @param code Output array of instructions
 @param maxcount Number of instructions allowed to write into @p code
 @param fp Input file to read instructions from
 @return Number of instructions read in, or -1 on failure.
 */
int read_program(Insn* code, Word maxcount, FILE* fp);

/*! Dissassemble an entire PM/0 program into a given buffer
 @param buf Output buffer for holding the disassembly string
 @param linesize Size of each line including the null terminator
 @param count Number of instructions to disassemble
 @param code Array of instructions
 @param sep Separator to use between columns
 @return Whether the instructions were successfully disassembled
 */
bool dis_program(char* buf, size_t linesize, Word count, Insn* code, const char* sep);

/*! Get a string representation of the instruction's opcode */
const char* Insn_getMnemonic(Insn insn);

/*! Disassemble an individual instruction */
char* Insn_prettyDis(Insn insn);


#endif /* PL0_INSTRUCTION_H */
