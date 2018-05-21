//
//  machine.h
//  PL/0
//
//  Created by Kevin Colley on 9/3/15.
//  Copyright (c) 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_MACHINE_H
#define PL0_MACHINE_H

#include <stdio.h>
#include <stdbool.h>

typedef enum CPUStatus CPUStatus;
typedef struct CPUState CPUState;
typedef enum CPUDebugFlags CPUDebugFlags;
typedef struct Breakpoint Breakpoint;
typedef struct Machine Machine;

#include "object.h"
#include "config.h"
#include "instruction.h"

/*! Execution status of the CPU */
enum CPUStatus {
	STATUS_NOT_STARTED = 1,
	STATUS_RUNNING,
	STATUS_PAUSED,
	STATUS_HALTED,
	STATUS_ERROR
};

/*! Registers used by the PM/0 virtual machine */
struct CPUState {
	Word bp;
	Word sp;
	Insn ir;
	Word pc;
};

/*! Debugging flags used by the CPU */
enum CPUDebugFlags {
	/*! Whether the CPU is running under a debugger */
	DEBUG_ACTIVE   = (1 << 0),
	
	/*! Whether we should skip the next instruction if it's a breakpoint */
	DEBUG_RESUMING = (1 << 1),
	
	/*! Whether we are single stepping and should stop after the next instruction */
	DEBUG_STEPPING = (1 << 2)
};

/*! The data associated with a breakpoint */
struct Breakpoint {
	Word addr;
	Insn orig;
	bool enabled;
};

/*! The entire state of the PM/0 virtual machine */
struct Machine {
	OBJECT_BASE;
	
	/*! Input port */
	FILE* fin;
	
	/*! Output port */
	FILE* fout;
	
	/*! Column separator */
	char* sep;
	
	/*! Whether to enable markdown formatted output */
	bool markdown;
	
	/*! Output file stream to log stacktrace info to */
	FILE* flog;
	
	/*! Current running status of the CPU */
	CPUStatus status;
	
	/*! Internal state of the CPU (registers) */
	CPUState state;
	
	/*! Actual number of instructions loaded */
	Word insn_count;
	
	/*! All stack frame pointers for printing the stacktrace */
	Word frames[MAX_LEXI_LEVELS];
	
	/*! Number of stack frames currently on the stack */
	Word framecount;
	
	/*! Disassembled instructions with line numbers in table format */
	char codelines[2+MAX_CODE_LENGTH][DIS_LINE_LENGTH];
	
	/*! The code segment of the program */
	Insn codemem[MAX_CODE_LENGTH];
	
	/*! The data stack */
	Word stack[MAX_STACK_HEIGHT];
	
	/*! Array of breakpoints set */
	dynamic_array(Breakpoint) bps;
	
	/*! CPU debugging flags */
	CPUDebugFlags debugFlags;
	
	/*! Input stream buffer */
	dynamic_string input_buffer;
};
DECL(Machine);


/*! Initialize a new virtual machine using the given files as the input and output ports
 @param fin Input port
 @param fout Output port
 @return Initialized machine instance
 */
Machine* Machine_initWithPorts(Machine* self, FILE* fin, FILE* fout);

/*! Sets the string used to separate output columns
 @param sep String used to separate columns
 */
void Machine_setSeparator(Machine* self, const char* sep);

/*! Instructs the machine to enable markdown formatted output */
void Machine_enableMarkdown(Machine* self);

/*! Set the output file stream where stacktrace info will be logged */
void Machine_setLogFile(Machine* self, FILE* flog);

/*! Loads a machine code program from the specified file
 @param fp Input file to load code from
 @return True on success, or false on error
 */
bool Machine_loadCode(Machine* self, FILE* fp);

/*! Writes the disassembled code in table format to the given file
 @param fp Output file stream to pring the disassembly table
 */
void Machine_printDisassembly(Machine* self, FILE* fp);

/*! Gets the machine started */
void Machine_start(Machine* self);

/*! Begin execution of the machine until the program halts (or an exception occurs)
 @return True on success, or false on error
 */
bool Machine_run(Machine* self);

/*! Resume execution after a breakpoint */
CPUStatus Machine_continue(Machine* self);

/*! Steps over a single instruction */
CPUStatus Machine_step(Machine* self);

/*! Returns the running status of the CPU */
CPUStatus Machine_getStatus(Machine* self);

/*! Adds a breakpoint at the specified code address
 @param addr Code address to insert a breakpoint
 @return Breakpoint ID, or -1 on error
 */
Word Machine_addBreakpoint(Machine* self, Word addr);

/*! Check if a breakpoint with the specified ID exists
 @param bpid ID of breakpoint to check for
 @return True if the breakpoint exists, false otherwise
 */
bool Machine_breakpointExists(Machine* self, Word bpid);

/*! Disable a breakpoint referenced by its ID
 @param bpid ID of breakpoint to disable
 */
void Machine_disableBreakpoint(Machine* self, Word bpid);

/*! Enable a previously disabled breakpoint by its ID
 @param bpid ID of breakpoint to disable
 */
void Machine_enableBreakpoint(Machine* self, Word bpid);

/*! Toggle whether a breakpoint is disabled
 @param bpid ID of breakpoint to toggle
 @return New state of the breakpoint
 */
bool Machine_toggleBreakpoint(Machine* self, Word bpid);

/*! Removes all breakpoints */
void Machine_clearBreakpoints(Machine* self);

/*! Prints the contents of the machine's stack to the specified file stream */
void Machine_printStack(Machine* self, FILE* fp);

/*! Prints the stack and registers to the specified file stream */
void Machine_printState(Machine* self, FILE* fp);


#endif /* PL0_MACHINE_H */
