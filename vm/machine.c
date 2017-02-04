//
//  machine.c
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#include "machine.h"
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "object.h"


static void interrupt_handler(int sig);
static void Machine_fetch(Machine* self, jmp_buf jmp);
static void Machine_execute(Machine* self, jmp_buf jmp);
static void Machine_execALU(Machine* self, jmp_buf jmp);
static void Machine_runOne(Machine* self);


/* Convenience macros, to decrease source code size and convolution */
#define SP            self->state.sp
#define BP            self->state.bp
#define IR            self->state.ir
#define PC            self->state.pc
#define STACK(sp)     self->stack[check_sp(sp, jmp)]
#define BASE(l)       get_base(l, BP, &self->stack[0], jmp)
#define TOP           STACK(SP)
#define POPPED        STACK(SP + 1)
#define POP()         (void)(--SP)
#define PUSH(x)       STACK(++SP) = (x)


static void vRuntimeError(const char* fmt, va_list ap) {
	printf("Runtime Error: ");
	vprintf(fmt, ap);
	printf("\n");
}

static void runtimeError(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vRuntimeError(fmt, ap);
	va_end(ap);
}

static inline Word check_pc(Word pc, Word code_length, jmp_buf jmp) {
	if(pc < 0) {
		runtimeError("PC(-0x%x) < 0", -pc);
		longjmp(jmp, STATUS_ERROR);
	}
	else if(pc >= code_length) {
		runtimeError("PC(0x%x) > code_length(0x%x)", pc, code_length);
		longjmp(jmp, STATUS_ERROR);
	}
	
	return pc;
}

static inline Word check_sp(Word sp, jmp_buf jmp) {
	if(sp < 0) {
		runtimeError("SP(-0x%x) < 0", -sp);
		longjmp(jmp, STATUS_ERROR);
	}
	else if(sp >= MAX_STACK_HEIGHT) {
		runtimeError("SP(0x%x) > MAX_STACK_HEIGHT(0x%x)", sp, MAX_STACK_HEIGHT);
		longjmp(jmp, STATUS_ERROR);
	}
	
	return sp;
}

/*! Find the base of the stack frame @p l frames below the current one */
static inline Word get_base(Word l, Word bp, Word* stack, jmp_buf jmp) {
	Word cur = bp;
	while(l-- > 0) {
		cur = stack[check_sp(cur + 1, jmp)];
	}
	return cur;
}

/*! Repeat the given character to create a string */
static inline char* repeated(char c, size_t count) {
	char* ret = malloc_ff(count + 1);
	memset(ret, c, count);
	ret[count] = '\0';
	return ret;
}


/* Interrupt handler */
static sigjmp_buf intjmp;
static void interrupt_handler(int sig) {
	(void)sig;
	/* Disable signal handler */
	signal(SIGINT, SIG_DFL);
	
	/* Jump to code used to catch ^C */
	siglongjmp(intjmp, 1);
}


Destroyer(Machine) {
	destroy(&self->bps);
}
DEF(Machine);

Machine* Machine_initWithPorts(Machine* self, FILE* fin, FILE* fout) {
	if((self = Machine_init(self))) {
		/* Set IO ports */
		self->fin = fin;
		self->fout = fout;
		
		/* Hasn't yet started */
		self->status = STATUS_NOT_STARTED;
		
		/* Default to no separator */
		self->sep = strdup_ff("");
		
		/* Initialize CPU state (all registers except bp start at zero) */
		BP = 1;
	}
	
	return self;
}

void Machine_setSeparator(Machine* self, const char* sep) {
	self->sep = strdup_ff(sep);
}

void Machine_enableMarkdown(Machine* self) {
	self->markdown = true;
	Machine_setSeparator(self, "|");
}

void Machine_setLogFile(Machine* self, FILE* flog) {
	self->flog = flog;
}

int Machine_loadCode(Machine* self, FILE* fp) {
	/* Parse code from text file into instructions */
	self->insn_count = read_program(&self->codemem[0], MAX_CODE_LENGTH, fp);
	if(self->insn_count < 0) {
		return -1;
	}
	
	/* Create string for table column headers */
	snprintf(&self->codelines[0][0], DIS_LINE_LENGTH,
			 /*           |      Insn|        OP|         L|         M| */
			 "%7$s%3$*1$s%7$s%4$*2$s%7$s%5$*2$s%7$s%6$*2$s%7$s",
			 DIS_FIRST_COL_WIDTH, DIS_COL_WIDTH,
			 "Line", "OP", "L", "M",
			 self->sep);
	
	/* Create string for horizontal line */
	char* first_horiz = NULL;
	char* col_horiz = NULL;
	if(self->markdown) {
		first_horiz = repeated('-', DIS_FIRST_COL_WIDTH - 1);
		col_horiz = repeated('-', DIS_COL_WIDTH - 1);
		snprintf(&self->codelines[1][0], DIS_LINE_LENGTH,
				 /*        | Insn|   OP|    L|    M| */
				 "|%1$s:|%2$s:|%2$s:|%2$s:|",
				 first_horiz, col_horiz);
	}
	
	destroy(&col_horiz);
	destroy(&first_horiz);
	
	/* Disassemble instructions in code memory to the rest of the string table */
	if(!dis_program(
		&self->codelines[2][0],
		DIS_LINE_LENGTH,
		self->insn_count,
		&self->codemem[0],
		self->sep)) {
		return -1;
	}
	
	return 0;
}

void Machine_printDisassembly(Machine* self, FILE* fp) {
	/* Print table and body */
	Word i;
	for(i = 0; i < self->insn_count + 2; i++) {
		if(self->codelines[i][0] == '\0') {
			continue;
		}
		
		fprintf(fp, "%s\n", &self->codelines[i][0]);
	}
	
	/* Trailing newline to separate disassembly from running stacktrace */
	fprintf(fp, "\n");
}

static void Machine_fetch(Machine* self, jmp_buf jmp) {
	/* Peek at the current instruction */
	Insn cur = self->codemem[check_pc(PC, self->insn_count, jmp)];
	
	/* Allow resuming from a breakpoint */
	if(self->isResuming) {
		/* If the instruction is a breakpoint, fetch the original instruction instead */
		if(IS_BREAK(cur) && Machine_breakpointExists(self, cur.imm)) {
			/* Use the original instruction as the instruction to execute */
			IR = self->bps[cur.imm].orig;
			return;
		}
		self->isResuming = false;
	}
	
	/* Prepare to execute this instruction and increment program counter */
	IR = cur;
}

static void Machine_execute(Machine* self, jmp_buf jmp) {
	switch(IR.op) {
		case OP_BREAK:
			/* Can be used by the compiler to insert a breakpoint */
			if(IR.lvl != 0) {
				/* BREAK instructions with L=1 are created by a debugger */
				if(IR.lvl == 1) {
					/* This shouldn't be possible as the bp should be caught before execute */
					abort();
				}
				
				longjmp(jmp, STATUS_PAUSED);
			}
			
			printf("Illegal instruction!\n");
			longjmp(jmp, STATUS_ERROR);
		
		case OP_LIT:
			PUSH(IR.imm);
			break;
			
		case OP_OPR:
			Machine_execALU(self, jmp);
			break;
			
		case OP_LOD:
			PUSH(STACK(BASE(IR.lvl) + IR.imm));
			break;
			
		case OP_STO:
			STACK(BASE(IR.lvl) + IR.imm) = STACK(SP--);
			break;
			
		case OP_CAL:
			STACK(SP + 1) = 0;
			STACK(SP + 2) = BASE(IR.lvl);
			STACK(SP + 3) = BP;
			STACK(SP + 4) = PC;
			BP = SP + 1;
			PC = IR.imm;
			assert(self->framecount < MAX_LEXI_LEVELS);
			self->frames[self->framecount++] = BP;
			break;
			
		case OP_INC:
			SP += IR.imm;
			break;
			
		case OP_JMP:
			PC = IR.imm;
			break;
			
		case OP_JPC:
			if(TOP == 0) {
				PC = IR.imm;
			}
			POP();
			break;
		
		case OP_SIO:
			switch(IR.imm) {
				case 1: /* WRITE */
					fprintf(self->fout, "%d\n", TOP);
					POP();
					break;
					
				case 2: /* READ */ {
					Word n;
					if(fscanf(self->fin, "%d", &n) != 1) {
						/* Not sure how to handle EOF */
						n = -1;
					}
					PUSH(n);
					break;
				}
				
				case 3: /* HALT */
					/* Clear state */
					memset(&self->state, 0, sizeof(self->state));
					self->framecount = 0;
					self->status = STATUS_HALTED;
					break;
				
				default:
					abort();
			}
			break;
			
		default:
			abort();
	}
}

static void Machine_execALU(Machine* self, jmp_buf jmp) {
	switch(IR.imm) {
		case ALU_RET:
			SP = BP - 1;
			PC = STACK(SP + 4);
			BP = STACK(SP + 3);
			--self->framecount;
			break;
			
		case ALU_NEG:
			TOP *= -1;
			break;
			
		case ALU_ADD:
			POP();
			TOP += POPPED;
			break;
			
		case ALU_SUB:
			POP();
			TOP -= POPPED;
			break;
			
		case ALU_MUL:
			POP();
			TOP *= POPPED;
			break;
			
		case ALU_DIV:
			POP();
			if(POPPED == 0) {
				runtimeError("Tried to divide by zero!");
				longjmp(jmp, STATUS_ERROR);
			}
			TOP /= POPPED;
			break;
			
		case ALU_ODD:
			TOP &= 1;
			break;
			
		case ALU_MOD:
			POP();
			if(POPPED == 0) {
				runtimeError("Tried to mod by zero!");
				longjmp(jmp, STATUS_ERROR);
			}
			TOP %= POPPED;
			break;
			
		case ALU_EQL:
			POP();
			TOP = (TOP == POPPED);
			break;
			
		case ALU_NEQ:
			POP();
			TOP = (TOP != POPPED);
			break;
			
		case ALU_LSS:
			POP();
			TOP = (TOP < POPPED);
			break;
			
		case ALU_LEQ:
			POP();
			TOP = (TOP <= POPPED);
			break;
			
		case ALU_GTR:
			POP();
			TOP = (TOP > POPPED);
			break;
			
		case ALU_GEQ:
			POP();
			TOP = (TOP >= POPPED);
			break;
			
		default:
			abort();
	}
}

static void Machine_runOne(Machine* self) {
	/* Setup error handler */
	jmp_buf jmp;
	CPUStatus err = setjmp(jmp);
	if(err != 0) {
		/* Landing from a longjmp, so set the new status and return */
		self->status = err;
		return;
	}
	
	/* Fetch cycle */
	Machine_fetch(self, jmp);
	
	/* Check if this is a breakpoint instruction */
	if(IS_BREAK(IR)) {
		/* Make sure the breakpoint ID is valid */
		if(!Machine_breakpointExists(self, IR.imm)) {
			runtimeError("Illegal instruction!\n");
			longjmp(jmp, STATUS_ERROR);
		}
		
		/* Use original IR so that a debugger can print the real instruction */
		IR = self->bps[IR.imm].orig;
		
		/* Stop fetching and set the state to paused */
		self->status = STATUS_PAUSED;
		return;
	}
	
	/* Increment program counter after we know we don't need to break */
	++PC;
	
	/* Create a jump target in case an interrupt signal is received */
	if(sigsetjmp(intjmp, 1) == 0) {
		/* Setup signal handler to catch ^C */
		signal(SIGINT, &interrupt_handler);
	}
	else {
		/* Landing from a siglongjmp, so decrement PC as it was before we fetched */
		--PC;
		
		/* We are pausing now */
		self->status = STATUS_PAUSED;
		return;
	}
	
	/* Execute cycle */
	Machine_execute(self, jmp);
	
	/* Disable signal handler if it wasn't needed */
	signal(SIGINT, SIG_DFL);
}

void Machine_start(Machine* self) {
	/* Now just paused */
	self->status = STATUS_PAUSED;
	
	if(self->flog != NULL) {
		/* Print table header */
		const char* prefix = &self->codelines[0][0];
		char* spaces = NULL;
		if(!self->markdown) {
			spaces = repeated(' ', DIS_FIRST_COL_WIDTH + 3 * DIS_COL_WIDTH);
			prefix = spaces;
		}
		
		/*             ...|     pc|        bp|        sp| stack */
		fprintf(self->flog, "%1$s%3$*2$s%6$s%4$*2$s%6$s%5$*2$s%6$s  stack\n",
				prefix, DIS_COL_WIDTH, "pc", "bp", "sp",
				self->sep);
		
		destroy(&spaces);
		
		/* Print horizontal table line */
		char* col_horiz = NULL;
		if(self->markdown) {
			col_horiz = repeated('-', DIS_COL_WIDTH - 1);
			fprintf(self->flog, "%1$s%2$s:|%2$s:|%2$s:|:------\n",
					&self->codelines[1][0], col_horiz);
		}
		destroy(&col_horiz);
		
		/* Print initial state row */
		if(self->markdown) {
			/*                      |      Insn|        OP|         L|         M| */
			fprintf(self->flog, "%7$s%3$*1$s%7$s%3$*2$s%7$s%3$*2$s%7$s%3$*2$s%7$s"
					/*       pc|        bp|        sp| stack */
					"%4$*2$d%7$s%5$*2$d%7$s%6$*2$d%7$s\n",
					DIS_FIRST_COL_WIDTH, DIS_COL_WIDTH,
					"", PC, BP, SP,
					self->sep);
		}
		else {
			/*                                   pc|        bp|        sp| stack */
			fprintf(self->flog, "%3$-*2$s%4$*1$d%7$s%5$*1$d%7$s%6$*1$d%7$s\n",
					DIS_COL_WIDTH, DIS_FIRST_COL_WIDTH + 3 * DIS_COL_WIDTH,
					"Initial values", PC, BP, SP,
					self->sep);
		}
		
		fflush(self->flog);
	}
}

int Machine_run(Machine* self) {
	/* Start the machine */
	Machine_start(self);
	
	/* The only correct way to end is in the halted state */
	return Machine_continue(self) == STATUS_HALTED ? 0 : EXIT_FAILURE;
}

CPUStatus Machine_continue(Machine* self) {
	/* Can't resume unless the CPU was paused */
	if(self->status != STATUS_PAUSED) {
		return self->status;
	}
	
	/* If the first instruction we fetch is a breakpoint, skip the breakpoint */
	self->isResuming = true;
	
	/* Keep executing instructions until an exception or halt */
	do {
		/* Get disassembly string before fetching (since that updates PC) */
		const char* dis = &self->codelines[2 + PC][0];
		
		/* Only transition to running status if we aren't single stepping */
		if(!self->isStepping) {
			self->status = STATUS_RUNNING;
		}
		
		/* Execute current instruction */
		Machine_runOne(self);
		
		if(self->flog != NULL) {
			/* Print out next row in stack trace table */
			/*             ...|        pc|        bp|        sp| stack */
			fprintf(self->flog, "%1$s%3$*2$d%6$s%4$*2$d%6$s%5$*2$d%6$s  ",
				dis, DIS_COL_WIDTH, PC, BP, SP,
				self->sep);
			
			/* Dump the stack contents to the log file */
			Machine_printStack(self, self->flog);
			fflush(self->flog);
		}
	} while(self->status == STATUS_RUNNING);
	
	return self->status;
}

CPUStatus Machine_step(Machine* self) {
	/* Fetch and execute a single instruction */
	self->isStepping = true;
	Machine_continue(self);
	self->isStepping = false;
	
	/* Setup error handler */
	jmp_buf jmp;
	CPUStatus err = setjmp(jmp);
	if(err != 0) {
		return self->status = err;
	}
	
	/* Fetch the next instruction into IR so the state is correct */
	Machine_fetch(self, jmp);
	return self->status;
}

CPUStatus Machine_getStatus(Machine* self) {
	return self->status;
}

int Machine_addBreakpoint(Machine* self, Word addr) {
	/* Make sure breakpoint address is within the code segment */
	if(addr < 0 || addr >= self->insn_count) {
		return -1;
	}
	
	/* If there's already a breakpoint at that address, just return its ID */
	if(IS_BREAK(self->codemem[addr])) {
		return self->codemem[addr].imm;
	}
	
	/* Expand breakpoint array if necessary */
	if(self->bp_count == self->bp_cap) {
		expand(&self->bps, &self->bp_cap);
	}
	
	/* Save breakpoint information */
	self->bps[self->bp_count] = (Breakpoint){addr, self->codemem[addr], true};
	
	/* Replace real instruction with breakpoint instruction */
	self->codemem[addr] = MAKE_BREAK((int)self->bp_count);
	
	/* Return breakpoint ID */
	return (int)self->bp_count++;
}

bool Machine_breakpointExists(Machine* self, int bpid) {
	return bpid >= 0 && bpid < (int)self->bp_count;
}

void Machine_disableBreakpoint(Machine* self, int bpid) {
	/* Make sure breakpoint ID is valid */
	if(!Machine_breakpointExists(self, bpid)) {
		return;
	}
	
	/* Lookup breakpoint */
	Breakpoint* bp = &self->bps[bpid];
	
	/* Already disabled? */
	if(!bp->enabled) {
		return;
	}
	
	/* Restore original instruction */
	self->codemem[bp->addr] = bp->orig;
	
	/* Mark breakpoint as disabled */
	self->bps[bpid].enabled = false;
}

void Machine_enableBreakpoint(Machine* self, int bpid) {
	/* Make sure breakpoint ID is valid */
	if(!Machine_breakpointExists(self, bpid)) {
		return;
	}
	
	/* Lookup breakpoint */
	Breakpoint* bp = &self->bps[bpid];
	
	/* Already enabled? */
	if(bp->enabled) {
		return;
	}
	
	/* Replace original instruction with breakpoint instruction */
	self->codemem[bp->addr] = MAKE_BREAK(bpid);
	
	/* Mark breakpoint as enabled */
	self->bps[bpid].enabled = true;
}

bool Machine_toggleBreakpoint(Machine* self, int bpid) {
	/* Make sure breakpoint ID is valid */
	if(Machine_breakpointExists(self, bpid)) {
		return false;
	}
	
	/* Toggle breakpoint */
	bool enabled = self->bps[bpid].enabled;
	if(enabled) {
		Machine_disableBreakpoint(self, bpid);
	}
	else {
		Machine_enableBreakpoint(self, bpid);
	}
	
	/* Return new state */
	return !enabled;
}

void Machine_clearBreakpoints(Machine* self) {
	/* Disable all breakpoints */
	size_t i;
	for(i = 0; i < self->bp_count; i++) {
		Machine_disableBreakpoint(self, (unsigned)i);
	}
	
	/* Destroy the breakpoint array */
	self->bp_count = 0;
	self->bp_cap = 0;
	destroy(&self->bps);
}

void Machine_printStack(Machine* self, FILE* fp) {
	/* Print each value on the stack */
	Word stackpos, curframe;
	for(stackpos = 1, curframe = 0; stackpos <= SP; stackpos++) {
		/* Print a vertical separator when we encounter a new stack frame */
		if(curframe < self->framecount && stackpos == self->frames[curframe]) {
			++curframe;
			fprintf(fp, "%s|", stackpos == 1 ? "" : " ");
		}
		
		fprintf(fp, &" %d"[stackpos == 1 && curframe == 0], self->stack[stackpos]);
	}
	
	fprintf(fp, "\n");
}

void Machine_printState(Machine* self, FILE* fp) {
	/* Show CPU status */
	const char* status;
	switch(self->status) {
		case STATUS_PAUSED:  status = "PAUSED"; break;
		case STATUS_RUNNING: status = "RUNNING"; break;
		case STATUS_HALTED:  status = "HALTED"; break;
		case STATUS_ERROR:   status = "ERROR"; break;
		default: abort();
	}
	printf("Status: %s\n", status);
	
	/* Show contents of stack */
	fprintf(fp, "Stack contents:\n");
	if(SP <= 0) {
		printf("Empty\n");
	}
	else {
		Machine_printStack(self, fp);
	}
	printf("\n");
	
	/* Show registers */
	fprintf(fp, "BP: %d\n", BP);
	fprintf(fp, "SP: %d\n", SP);
	fprintf(fp, "PC: %d\n", PC);
	
	/* Show contents of IR */
	fprintf(fp, "\nInstruction:\n");
	fprintf(fp, "OP: %s\n", Insn_getMnemonic(IR));
	fprintf(fp, "L:  %hu\n", IR.lvl);
	fprintf(fp, "M:  %d\n", IR.imm);
}
