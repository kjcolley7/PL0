//
//  machine.c
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#include "machine.h"
#include <string.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>
#include "object.h"


static void interrupt_handler(int sig);
static void enable_interrupt_handler(void);
static void disable_interrupt_handler(void);
static bool Machine_fetch(Machine* self);
static void Machine_readChunk(Machine* self);
static bool Machine_readIntString(Machine* self, dynamic_string* intstr);
static bool Machine_readWord(Machine* self, Word* pvalue);
static bool Machine_execute(Machine* self);
static bool Machine_execALU(Machine* self);
static bool Machine_runOne(Machine* self);


/* Convenience macros, to decrease source code size and convolution */
#define SP            self->state.sp
#define BP            self->state.bp
#define IR            self->state.ir
#define PC            self->state.pc
#define STACK(sp)     self->stack[check_sp(sp)]
#define BASE(l)       get_base(l)
#define TOP           STACK(SP)
#define POPPED        STACK(SP + 1)
#define POP()         (void)(--SP)
#define PUSH(x)       (void)(STACK(++SP) = (x))


static void vRuntimeError(const char* fmt, va_list ap) {
	printf("Runtime Error: ");
	vprintf(fmt, ap);
	printf("\n");
}

static void runtimeError(const char* fmt, ...) {
	VARIADIC(fmt, ap, {
		vRuntimeError(fmt, ap);
	});
}


#define check_pc(pc, code_length) UNIQUIFY(check_pc_, pc, code_length)
#define check_pc_(id, pc, code_length) ({ \
	Word _pc_##id = (pc); \
	Word _code_length_##id = (code_length); \
	if(_pc_##id < 0) { \
		runtimeError("PC(-0x%"PRIxWORD") < 0", -_pc_##id); \
		self->status = STATUS_ERROR; \
		return false; \
	} \
	else if(_pc_##id >= _code_length_##id) { \
		runtimeError("PC(0x%"PRIxWORD") >= code_length(0x%"PRIxWORD")", _pc_##id, _code_length_##id); \
		self->status = STATUS_ERROR; \
		return false; \
	} \
	_pc_##id; \
})

#define check_sp(sp) UNIQUIFY(check_sp_, sp)
#define check_sp_(id, sp) ({ \
Word _sp_##id = (sp); \
	if(_sp_##id < 0) { \
		runtimeError("SP(-0x%"PRIxWORD") < 0", -_sp_##id); \
		self->status = STATUS_ERROR; \
		return false; \
	} \
	else if(_sp_##id >= MAX_STACK_HEIGHT) { \
		runtimeError("SP(0x%"PRIxWORD") >= MAX_STACK_HEIGHT(0x%"PRIxWORD")", _sp_##id, MAX_STACK_HEIGHT); \
		self->status = STATUS_ERROR; \
		return false; \
	} \
	_sp_##id; \
})

/*! Find the base of the stack frame @p l frames below the current one */
#define get_base(l) UNIQUIFY(get_base_, l)
#define get_base_(id, l) ({ \
	Word _l_##id = (l); \
	Word _cur_##id = BP; \
	Word* _stack_##id = self->stack; \
	while(_l_##id-- > 0) { \
	Word _index_##id = check_sp_(_get_base_##id, _cur_##id + 1); \
		_cur_##id = _stack_##id[_index_##id]; \
	} \
	_cur_##id; \
})

/*! Repeat the given character to create a string */
static inline char* repeated(char c, size_t count) {
	char* ret = malloc_ff(count + 1);
	memset(ret, c, count);
	ret[count] = '\0';
	return ret;
}


/* Interrupt handler */
static volatile sig_atomic_t interrupted = 0;
static void interrupt_handler(int sig) {
	(void)sig;
	
	/* Flag the process as having been interrupted */
	interrupted = 1;
	
	/* Cannot use printf in the context of a signal handler */
	const char* msg = "Received keyboard interrupt\n";
	write(STDERR_FILENO, msg, strlen(msg));
}

static struct sigaction old_handler;
static void enable_interrupt_handler(void) {
	interrupted = false;
	
	struct sigaction sa;
	sa.sa_flags = SA_RESETHAND;
	sa.sa_handler = &interrupt_handler;
	
	sigaction(SIGINT, &sa, &old_handler);
}

static void disable_interrupt_handler(void) {
	if(!interrupted) {
		sigaction(SIGINT, &old_handler, NULL);
	}
}


Destroyer(Machine) {
	array_clear(&self->bps);
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

bool Machine_loadCode(Machine* self, FILE* fp) {
	/* Parse code from text file into instructions */
	self->insn_count = read_program(&self->codemem[0], MAX_CODE_LENGTH, fp);
	if(self->insn_count < 0) {
		return false;
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
		return false;
	}
	
	return true;
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

static void Machine_readChunk(Machine* self) {
	if(interrupted) {
		return;
	}
	
	/* Read a maximum of 100 bytes from the input file descriptor */
	char buffer[100];
	ssize_t bytes_read = read(fileno(self->fin), buffer, sizeof(buffer));
	if(bytes_read < 0) {
		if(interrupted) {
			return;
		}
		else {
			perror("read");
			return;
		}
	}
	
	/* Append the bytes that were read to the input buffer */
	string_appendLength(&self->input_buffer, buffer, bytes_read);
}

static inline bool isSignedDigit(char c) {
	return c == '-' || isdigit(c);
}

static bool Machine_readIntString(Machine* self, dynamic_string* intstr) {
	bool done = false;
	do {
		size_t length = string_length(&self->input_buffer);
		
		/* Find the index of the first digit */
		size_t start = 0;
		while(start < length && !isSignedDigit(self->input_buffer.elems[start])) {
			++start;
		}
		
		/* If we've already found digits and the input buffer doesn't start with a digit, we're done */
		if(!string_empty(intstr) && start != 0) {
			return true;
		}
		
		/* Did the input buffer contain at least one digit? */
		if(start < length) {
			/* Find the end of the substring of digits in the input buffer */
			size_t end = start + 1;
			while(end < length && isdigit(self->input_buffer.elems[end])) {
				++end;
			}
			
			/* Append substring of digits from the input buffer to the integer string */
			string_appendLength(intstr, &self->input_buffer.elems[start], end - start);
			
			/* Only done if there are more characters in the input that aren't digits (like newline) */
			if(end < length) {
				done = true;
			}
			
			/* Remove everything that was read from the input buffer */
			string_removeRange(&self->input_buffer, 0, end);
		}
		else {
			/* Input buffer contained no digits, so clear its contents */
			string_clear(&self->input_buffer);
		}
		
		/* Read more data from the input file descriptor if we haven't found the end of the number */
		if(!done) {
			Machine_readChunk(self);
			if(interrupted) {
				/* Need to add the data we read back to the input buffer */
				string_append(intstr, string_cstr(&self->input_buffer));
				string_clear(&self->input_buffer);
				memcpy(&self->input_buffer, intstr, sizeof(self->input_buffer));
				memset(intstr, 0, sizeof(*intstr));
				return false;
			}
		}
	} while(!done);
	
	return true;
}

static bool Machine_readWord(Machine* self, Word* pvalue) {
	bool success = false;
	dynamic_string intstr = {};
	long value = 0;
	
	do {
		/* Read what might be a signed integer string */
		if(!Machine_readIntString(self, &intstr)) {
			string_clear(&intstr);
			return false;
		}
	
		/* Try to convert the string to an integer */
		char* end = NULL;
		value = strtol(string_cstr(&intstr), &end, 0);
		
		/* Check if the conversion succeeded */
		success = end && *end == '\0';
	
		if(success) {
			/* Check if the long fits in a Word */
			if(value > WORD_MAX || value < WORD_MIN) {
				success = false;
			}
		}
		
		if(!success) {
			fprintf(stderr, "Runtime Error: Could not convert input \"%s\" to an integer.\n", string_cstr(&intstr));
		}
		
		/* Deallocate string contents */
		string_clear(&intstr);
	} while(!success);
	
	/* Store result and return success */
	*pvalue = (Word)value;
	return true;
}

static bool Machine_fetch(Machine* self) {
	/* Peek at the current instruction */
	Insn cur = self->codemem[check_pc(PC, self->insn_count)];
	
	/* Allow resuming from a breakpoint */
	if(self->debugFlags & DEBUG_RESUMING) {
		/* If the instruction is a breakpoint, fetch the original instruction instead */
		if(IS_BREAK(cur) && Machine_breakpointExists(self, cur.imm)) {
			/* Use the original instruction as the instruction to execute */
			IR = self->bps.elems[cur.imm].orig;
			return true;
		}
		self->debugFlags &= ~DEBUG_RESUMING;
	}
	
	/* Prepare to execute this instruction and increment program counter */
	IR = cur;
	return true;
}

static bool Machine_execute(Machine* self) {
	switch(IR.op) {
		case OP_BREAK:
			/* Can be used by the compiler to insert a breakpoint */
			if(IR.lvl != 0) {
				/* BREAK instructions with L=1 are created by a debugger */
				/* This shouldn't be possible as the bp should be caught before execute */
				ASSERT(IR.lvl != 1);
				
				self->status = STATUS_PAUSED;
				return false;
			}
			
			fprintf(stderr, "Illegal instruction!\n");
			self->status = STATUS_ERROR;
			return false;
		
		case OP_LIT:
			PUSH(IR.imm);
			break;
			
		case OP_OPR:
			return Machine_execALU(self);
			
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
			ASSERT(self->framecount < MAX_LEXI_LEVELS);
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
					fprintf(self->fout, "%"PRIdWORD"\n", TOP);
					POP();
					break;
					
				case 2: /* READ */ {
					Word n;
					if(!Machine_readWord(self, &n)) {
						if(interrupted) {
							self->status = STATUS_PAUSED;
							return false;
						}
						
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
					runtimeError("Unknown SIO instruction: SIO %"PRIdWORD, IR.imm);
					self->status = STATUS_ERROR;
					return false;
			}
			break;
			
		default:
			runtimeError("Unknown instruction: %d", IR.op);
			self->status = STATUS_ERROR;
			return false;
	}
	
	return true;
}

static bool Machine_execALU(Machine* self) {
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
				self->status = STATUS_ERROR;
				return false;
			}
			if(TOP == WORD_MIN && POPPED == -1) {
				runtimeError("Tried to divide WORD_MIN by -1!");
				self->status = STATUS_ERROR;
				return false;
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
				self->status = STATUS_ERROR;
				return false;
			}
			if(TOP == WORD_MIN && POPPED == -1) {
				runtimeError("Tried to mod WORD_MIN by -1!");
				self->status = STATUS_ERROR;
				return false;
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
			runtimeError("Unknown OPR instruction: OPR %"PRIdWORD, IR.imm);
			self->status = STATUS_ERROR;
			return false;
	}
	
	return true;
}

static bool Machine_runOne(Machine* self) {
	/* Fetch cycle */
	if(!Machine_fetch(self)) {
		return false;
	}
	
	/* Check if this is a breakpoint instruction */
	if(IS_BREAK(IR)) {
		/* Make sure the breakpoint ID is valid */
		if(!Machine_breakpointExists(self, IR.imm)) {
			runtimeError("Illegal instruction!\n");
			self->status = STATUS_ERROR;
			return false;
		}
		
		/* Use original IR so that a debugger can print the real instruction */
		IR = self->bps.elems[IR.imm].orig;
		
		/* Stop fetching and set the state to paused */
		self->status = STATUS_PAUSED;
		return false;
	}
	
	/* Increment program counter after we know we don't need to break */
	++PC;
	
	/* Execute cycle */
	bool success = Machine_execute(self);
	if(!success) {
		/* The instruction didn't execute successfully, so reset PC */
		--PC;
	}
	
	return success;
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
					"%4$*2$"PRIdWORD"%7$s%5$*2$"PRIdWORD"%7$s%6$*2$"PRIdWORD"%7$s\n",
					DIS_FIRST_COL_WIDTH, DIS_COL_WIDTH,
					"", PC, BP, SP,
					self->sep);
		}
		else {
			/*                                   pc|        bp|        sp| stack */
			fprintf(self->flog, "%3$-*2$s%4$*1$"PRIdWORD"%7$s%5$*1$"PRIdWORD"%7$s%6$*1$"PRIdWORD"%7$s\n",
					DIS_COL_WIDTH, DIS_FIRST_COL_WIDTH + 3 * DIS_COL_WIDTH,
					"Initial values", PC, BP, SP,
					self->sep);
		}
		
		fflush(self->flog);
	}
}

bool Machine_run(Machine* self) {
	/* Start the machine */
	Machine_start(self);
	
	/* The only correct way to end is in the halted state */
	return Machine_continue(self) == STATUS_HALTED;
}

CPUStatus Machine_continue(Machine* self) {
	/* Can't resume unless the CPU was paused */
	if(self->status != STATUS_PAUSED) {
		return self->status;
	}
	
	/* If the first instruction we fetch is a breakpoint, skip the breakpoint */
	self->debugFlags |= DEBUG_RESUMING;
	
	if(self->debugFlags & DEBUG_ACTIVE) {
		/* Catch Ctrl+C interrupt */
		enable_interrupt_handler();
	}
	
	/* Keep executing instructions until an exception or halt */
	do {
		/* Get disassembly string before fetching (since that updates PC) */
		const char* dis = &self->codelines[2 + PC][0];
		
		/* Only transition to running status if we aren't single stepping */
		if(!(self->debugFlags & DEBUG_STEPPING)) {
			self->status = STATUS_RUNNING;
		}
		
		/* Execute current instruction */
		if(!Machine_runOne(self)) {
			break;
		}
		
		if(self->flog != NULL) {
			/* Print out next row in stack trace table */
			/*             ...|        pc|        bp|        sp| stack */
			fprintf(self->flog, "%1$s%3$*2$"PRIdWORD"%6$s%4$*2$"PRIdWORD"%6$s%5$*2$"PRIdWORD"%6$s  ",
				dis, DIS_COL_WIDTH, PC, BP, SP,
				self->sep);
			
			/* Dump the stack contents to the log file */
			Machine_printStack(self, self->flog);
			fflush(self->flog);
		}
		
		/* Did we receive a Ctrl+C? */
		if(interrupted) {
			self->status = STATUS_PAUSED;
		}
	} while(self->status == STATUS_RUNNING);
	
	if(self->debugFlags & DEBUG_ACTIVE) {
		/* Disable Ctrl+C handler */
		disable_interrupt_handler();
	}
	
	return self->status;
}

CPUStatus Machine_step(Machine* self) {
	/* Fetch and execute a single instruction */
	self->debugFlags |= DEBUG_STEPPING;
	bool didStep = Machine_continue(self);
	self->debugFlags &= ~DEBUG_STEPPING;
	
	if(didStep) {
		/* Fetch the next instruction into IR so the state is correct */
		Machine_fetch(self);
	}
	
	return self->status;
}

CPUStatus Machine_getStatus(Machine* self) {
	return self->status;
}

Word Machine_addBreakpoint(Machine* self, Word addr) {
	/* Make sure breakpoint address is within the code segment */
	if(addr < 0 || addr >= self->insn_count) {
		return -1;
	}
	
	/* If there's already a breakpoint at that address, just return its ID */
	if(IS_BREAK(self->codemem[addr])) {
		return self->codemem[addr].imm;
	}
	
	/* Get breakpoint ID */
	Word breakpointID = (Word)self->bps.count;
	
	/* Save breakpoint information */
	array_append(&self->bps, (Breakpoint){addr, self->codemem[addr], true});
	
	/* Replace real instruction with breakpoint instruction */
	self->codemem[addr] = MAKE_BREAK(breakpointID);
	
	/* Return breakpoint ID */
	return breakpointID;
}

bool Machine_breakpointExists(Machine* self, Word bpid) {
	return bpid >= 0 && bpid < (Word)self->bps.count;
}

void Machine_disableBreakpoint(Machine* self, Word bpid) {
	/* Make sure breakpoint ID is valid */
	if(!Machine_breakpointExists(self, bpid)) {
		return;
	}
	
	/* Lookup breakpoint */
	Breakpoint* bp = &self->bps.elems[bpid];
	
	/* Already disabled? */
	if(!bp->enabled) {
		return;
	}
	
	/* Restore original instruction */
	self->codemem[bp->addr] = bp->orig;
	
	/* Mark breakpoint as disabled */
	bp->enabled = false;
}

void Machine_enableBreakpoint(Machine* self, Word bpid) {
	/* Make sure breakpoint ID is valid */
	if(!Machine_breakpointExists(self, bpid)) {
		return;
	}
	
	/* Lookup breakpoint */
	Breakpoint* bp = &self->bps.elems[bpid];
	
	/* Already enabled? */
	if(bp->enabled) {
		return;
	}
	
	/* Replace original instruction with breakpoint instruction */
	self->codemem[bp->addr] = MAKE_BREAK(bpid);
	
	/* Mark breakpoint as enabled */
	bp->enabled = true;
}

bool Machine_toggleBreakpoint(Machine* self, Word bpid) {
	/* Make sure breakpoint ID is valid */
	if(!Machine_breakpointExists(self, bpid)) {
		return false;
	}
	
	/* Toggle breakpoint */
	bool enabled = self->bps.elems[bpid].enabled;
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
	enumerate(&self->bps, i, _) {
		Machine_disableBreakpoint(self, (Word)i);
	}
	
	/* Destroy the breakpoint array */
	array_clear(&self->bps);
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
		
		fprintf(fp, &" %"PRIdWORD[stackpos == 1 && curframe == 0], self->stack[stackpos]);
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
		default: ASSERT(!"Unknown machine status");
	}
	fprintf(fp, "Status: %s\n", status);
	
	/* Show registers */
	fprintf(fp, "BP: %"PRIdWORD"\n", BP);
	fprintf(fp, "SP: %"PRIdWORD"\n", SP);
	fprintf(fp, "PC: %"PRIdWORD"\n", PC);
	
	/* Show instructions around PC */
	fprintf(fp, "\n");
	Word addr;
	for(addr = PC - 2; addr <= PC + 2; addr++) {
		if(addr >= 0 && addr < self->insn_count) {
			fprintf(fp, "%c%s\n", " >"[addr == PC], self->codelines[addr + 2]);
		}
	}
	fprintf(fp, "\n");
	
	/* Show contents of stack */
	fprintf(fp, "Stack contents:\n");
	if(SP <= 0) {
		fprintf(fp, "Empty\n");
	}
	else {
		Machine_printStack(self, fp);
	}
	fprintf(fp, "\n");
	
	/* Show contents of IR */
	fprintf(fp, "\nInstruction:\n");
	fprintf(fp, "OP: %s\n", Insn_getMnemonic(IR));
	fprintf(fp, "L:  %hu\n", IR.lvl);
	fprintf(fp, "M:  %"PRIdWORD"\n", IR.imm);
}
