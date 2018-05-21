//
//  instruction.c
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#include "instruction.h"
#include "config.h"
#include "object.h"


static const char* op_str[16] = {
	"BREAK",
	"LIT",
	"OPR",
	"LOD",
	"STO",
	"CAL",
	"INC",
	"JMP",
	"JPC",
	"SIO",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};


void Insn_emit(Insn insn, FILE* fp) {
	fprintf(fp, "%hu %hu %"PRIdWORD"\n", insn.op, insn.lvl, insn.imm);
}

/* Read an entire PM/0 program from the text file into the code array given */
int read_program(Insn* code, Word maxcount, FILE* fp) {
	Insn* cur = code;
	Insn* end = &code[maxcount];
	
	/* Keep reading lines of code until the end of the file */
	while(fscanf(fp, "%hu%hu%"SCNdWORD, &cur->op, &cur->lvl, &cur->imm) == 3) {
		if(++cur == end) {
			fprintf(stderr, "Completely filled code array\n");
			break;
		}
	}
	
	/* Return number of instructions read in */
	return (int)(cur - code);
}

/* Dissassemble an entire PM/0 program into a given buffer */
bool dis_program(char* buf, size_t linesiz, Word count, Insn* code, const char* sep) {
	if(count == 0) {
		return true;
	}
	
	/* Loop over each instruction and disassemble it */
	Word i;
	for(i = 0; i < count; i++) {
		/* Get string for opcode */
		const char* opstr = op_str[code[i].op & 0xf];
		if(opstr == NULL) {
			fprintf(stderr, "Invalid opcode %hu\n", code[i].op);
			return false;
		}
		
		/*                         |      Insn|        OP|          L|         M| */
		snprintf(buf, linesiz, "%7$s%3$*1$u%7$s%4$*2$s%7$s%5$*2$hu%7$s%6$*2$"PRIdWORD"%7$s",
		         DIS_FIRST_COL_WIDTH, DIS_COL_WIDTH,
				 i, opstr, code[i].lvl, code[i].imm,
				 sep);
		buf += linesiz;
	}
	
	return true;
}

const char* Insn_getMnemonic(Insn insn) {
	switch(insn.op) {
		case OP_BREAK: return "BREAK";
		case OP_LIT: return "LIT";
		case OP_LOD: return "LOD";
		case OP_STO: return "STO";
		case OP_CAL: return "CAL";
		case OP_INC: return "INC";
		case OP_JMP: return "JMP";
		case OP_JPC: return "JPC";
		case OP_SIO:
			switch(insn.imm) {
				case 1: return "WRITE";
				case 2: return "READ";
				case 3: return "HALT";
				default: return "SIO ?";
			}
			break;
		
		case OP_OPR:
			switch(insn.imm) {
				case ALU_RET: return "RET";
				case ALU_NEG: return "NEG";
				case ALU_ADD: return "ADD";
				case ALU_SUB: return "SUB";
				case ALU_MUL: return "MUL";
				case ALU_DIV: return "DIV";
				case ALU_ODD: return "ODD";
				case ALU_MOD: return "MOD";
				case ALU_EQL: return "EQL";
				case ALU_NEQ: return "NEQ";
				case ALU_LSS: return "LSS";
				case ALU_LEQ: return "LEQ";
				case ALU_GTR: return "GTR";
				case ALU_GEQ: return "GEQ";
				default: return "OPR ?";
			}
			
		/* Stupid trigraphs... */
		default: return "?""?""?";
	}
}

static const char* Insn_getPrettyMnemonic(Insn insn) {
	switch(insn.op) {
		case OP_LIT: return "<font color=\"" LIT_COLOR "\">LIT</font>  ";
		case OP_LOD: return "<font color=\"" LDS_COLOR "\">LOD</font>  ";
		case OP_STO: return "<font color=\"" LDS_COLOR "\">STO</font>  ";
		case OP_CAL: return "<font color=\"" CAL_COLOR "\">CAL</font>  ";
		case OP_INC: return "<font color=\"" INC_COLOR "\">INC</font>  ";
		case OP_JMP: return "<font color=\"" JMP_COLOR "\">JMP</font>  ";
		case OP_JPC: return "<font color=\"" JMP_COLOR "\">JPC</font>  ";
		case OP_SIO:
			switch(insn.imm) {
				case 1: return "<font color=\"" IO_COLOR "\">WRITE</font>";
				case 2: return "<font color=\"" IO_COLOR "\">READ</font> ";
				case 3: return "<font color=\"" RET_COLOR "\">HALT</font> ";
				default: return "<font color=\"" ERR_COLOR "\">SIO ?</font>";
			}
			break;
		
		case OP_OPR:
			switch(insn.imm) {
				case ALU_RET: return "<font color=\"" RET_COLOR "\">RET</font>  ";
				case ALU_NEG: return "<font color=\"" ARITH_COLOR "\">NEG</font>  ";
				case ALU_ADD: return "<font color=\"" ARITH_COLOR "\">ADD</font>  ";
				case ALU_SUB: return "<font color=\"" ARITH_COLOR "\">SUB</font>  ";
				case ALU_MUL: return "<font color=\"" ARITH_COLOR "\">MUL</font>  ";
				case ALU_DIV: return "<font color=\"" ARITH_COLOR "\">DIV</font>  ";
				case ALU_ODD: return "<font color=\"" COND_COLOR "\">ODD</font>  ";
				case ALU_MOD: return "<font color=\"" ARITH_COLOR "\">MOD</font>  ";
				case ALU_EQL: return "<font color=\"" COND_COLOR "\">EQL</font>  ";
				case ALU_NEQ: return "<font color=\"" COND_COLOR "\">NEQ</font>  ";
				case ALU_LSS: return "<font color=\"" COND_COLOR "\">LSS</font>  ";
				case ALU_LEQ: return "<font color=\"" COND_COLOR "\">LEQ</font>  ";
				case ALU_GTR: return "<font color=\"" COND_COLOR "\">GTR</font>  ";
				case ALU_GEQ: return "<font color=\"" COND_COLOR "\">GEQ</font>  ";
				default: return "<font color=\"" ERR_COLOR "\">OPR ?</font>";
			}
			
		/* Stupid trigraphs... */
		default: return "<font color=\"" ERR_COLOR "\">?""?""?</font>  ";
	}
}

char* Insn_prettyDis(Insn insn) {
	const char* mnemonic = Insn_getPrettyMnemonic(insn);
	char* ret;
	switch(insn.op) {
		/* Instructions which have no visible operands */
		case OP_OPR:
		case OP_SIO:
			asprintf_ff(&ret, "%s", mnemonic);
			break;
		
		/* Instructions with all three operands visible */
		case OP_LOD:
		case OP_STO:
		case OP_CAL:
			if(insn.op == OP_CAL && insn.imm == ADDR_UND) {
				asprintf_ff(&ret, "%s %hu UND", mnemonic, insn.lvl);
			}
			else {
				asprintf_ff(&ret, "%s %hu %"PRIdWORD, mnemonic, insn.lvl, insn.imm);
			}
			break;
		
		/* Instructions that can have undefined targets */
		case OP_JMP:
		case OP_JPC:
			if(insn.imm == ADDR_UND) {
				asprintf_ff(&ret, "%s UND", mnemonic);
			}
			else {
				asprintf_ff(&ret, "%s %"PRIdWORD, mnemonic, insn.imm);
			}
			break;
		
		/* Instructions with only two operands visible */
		default:
			asprintf_ff(&ret, "%s %"PRIdWORD, mnemonic, insn.imm);
			break;
	}
	return ret;
}

