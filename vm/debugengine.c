//
//  debugengine.c
//  PL/0
//
//  Created by Kevin Colley on 11/23/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "debugengine.h"
#include <ctype.h>


typedef enum CMD_TYPE {
	CMD_INVALID = 0,
	CMD_BREAKPOINT = 1,
	CMD_CONTINUE,
	CMD_STATE,
	CMD_STEP,
	CMD_HELP,
	CMD_QUIT,
	CMD_COUNT /*!< Number of command types */
} CMDTYPE;

static struct {
	const char* name;
	CMDTYPE type;
} cmd_map[] = {
	{"b",           CMD_BREAKPOINT},
	{"bp",          CMD_BREAKPOINT},
	{"break",       CMD_BREAKPOINT},
	{"breakpoint",  CMD_BREAKPOINT},
	{"c",           CMD_CONTINUE},
	{"cont",        CMD_CONTINUE},
	{"continue",    CMD_CONTINUE},
	{"exit",        CMD_QUIT},
	{"h",           CMD_HELP},
	{"help",        CMD_HELP},
	{"q",           CMD_QUIT},
	{"quit",        CMD_QUIT},
	{"s",           CMD_STEP},
	{"si",          CMD_STEP},
	{"state",       CMD_STATE},
	{"step",        CMD_STEP}
};

typedef struct Command {
	OBJECT_BASE;
	
	/*! Command type */
	CMDTYPE type;
	
	/*! Array of arguments */
	dynamic_array(char*) args;
} Command;
DECL(Command);

Destroyer(Command) {
	array_destroy(&self->args);
}
DEF(Command);

static Command* Command_initWithLine(Command* self, const char* line);
static char* Command_getNextArgument(const char** line);
static CMDTYPE Command_getType(const char* cmdstr);

static Command* Command_initWithLine(Command* self, const char* line) {
	if((self = Command_init(self))) {
		/* Add all arguments */
		char* cmdstr;
		while((cmdstr = Command_getNextArgument(&line))) {
			array_append(&self->args, cmdstr);
		}
		
		/* Need at least the command name */
		if(self->args.count == 0) {
			release(&self);
			return NULL;
		}
		
		/* Set command type */
		self->type = Command_getType(self->args.elems[0]);
		
		/* Invalid command? */
		if(self->type == CMD_INVALID) {
			printf("Invalid command\n");
			release(&self);
			return NULL;
		}
	}
	
	return self;
}

static char* Command_getNextArgument(const char** line) {
	/* Skip leading spaces */
	while(isspace(**line)) {
		++*line;
	}
	
	/* Check for end of string */
	if(**line == '\0') {
		return NULL;
	}
	
	/* Found start of argument */
	const char* arg_begin = *line;
	
	/* Find end of argument */
	while(!isspace(**line) && **line != '\0') {
		++*line;
	}
	
	/* Compute argument string length using current (end) pointer */
	size_t arg_length = *line - arg_begin;
	
	/* Allocate a copy of the argument */
	return strndup(arg_begin, arg_length);
}

static CMDTYPE Command_getType(const char* cmdstr) {
	/* Lookup the command string in the command map */
	size_t i;
	for(i = 0; i < ARRAY_COUNT(cmd_map); i++) {
		if(strcasecmp(cmdstr, cmd_map[i].name) == 0) {
			return cmd_map[i].type;
		}
	}
	
	/* Didn't find the command */
	return CMD_INVALID;
}



static int DebugEngine_perform(DebugEngine* self, Command* cmd);
static void DebugEngine_helpBreakpoint(DebugEngine* self);
static void DebugEngine_doBreakpoint(DebugEngine* self, Command* cmd);
static void DebugEngine_helpRunning(DebugEngine* self);
static void DebugEngine_doContinue(DebugEngine* self, Command* cmd);
static void DebugEngine_doState(DebugEngine* self, Command* cmd);
static void DebugEngine_doStep(DebugEngine* self, Command* cmd);
static void DebugEngine_doHelp(DebugEngine* self, Command* cmd);


Destroyer(DebugEngine) {
	release(&self->cpu);
};
DEF(DebugEngine);

DebugEngine* DebugEngine_initWithCPU(DebugEngine* self, Machine* cpu) {
	if((self = DebugEngine_init(self))) {
		self->cpu = retain(cpu);
		self->cpu->debugFlags |= DEBUG_ACTIVE;
	}
	
	return self;
}

bool DebugEngine_run(DebugEngine* self) {
	bool success = true;
	char line[1024];
	bool keepGoing = true;
	const char* prompt = "\r(dbg) ";
	
	/* Initialize the program */
	Machine_start(self->cpu);
	
	printf("\nPM/0 debugger\n");
	printf("%s", prompt);
	
	Command* lastCmd = NULL;
	while(keepGoing && fgets(line, sizeof(line), stdin)) {
		Command* cmd;
		
		/* If the line is empty, run the previous command again */
		if(strchr(line, '\n') == line) {
			cmd = lastCmd;
		}
		else {
			/* Parse the command line */
			cmd = Command_initWithLine(Command_alloc(), line);
			lastCmd = cmd;
		}
		
		if(!cmd) {
			if(feof(stdin)) {
				break;
			}
			
			printf("%s", prompt);
			continue;
		}
		
		/* Run the command */
		int quit = DebugEngine_perform(self, cmd);
		if(quit != 0) {
			/* We were told to quit */
			break;
		}
		
		/* Check on the status of the CPU */
		switch(Machine_getStatus(self->cpu)) {
			case STATUS_PAUSED:
				printf("%s", prompt);
				break;
			
			case STATUS_HALTED:
				printf("\nProgram execution halted successfully\n");
				keepGoing = false;
				break;
			
			case STATUS_ERROR:
				printf("\nState of the CPU when exception was thrown:\n");
				Machine_printState(self->cpu, stdout);
				success = false;
				keepGoing = false;
				break;
			
			case STATUS_RUNNING:
			case STATUS_NOT_STARTED:
			default:
				ASSERT(!"Unknown machine status");
		}
	}
	
	/* If stdin is closed, remove breakpoints */
	Machine_clearBreakpoints(self->cpu);
	return success;
}


static int DebugEngine_perform(DebugEngine* self, Command* cmd) {
	switch(cmd->type) {
		case CMD_BREAKPOINT: DebugEngine_doBreakpoint(self, cmd); break;
		case CMD_CONTINUE:   DebugEngine_doContinue(self, cmd); break;
		case CMD_STATE:      DebugEngine_doState(self, cmd); break;
		case CMD_STEP:       DebugEngine_doStep(self, cmd); break;
		case CMD_HELP:       DebugEngine_doHelp(self, cmd); break;
		case CMD_QUIT:       return 1;
		default: ASSERT(!"Unknown debugger command type");
	}
	
	return 0;
}

static void DebugEngine_helpBreakpoint(DebugEngine* self) {
	(void)self;
	printf("Available breakpoint commands:\n");
	printf("bp add <addr>   -- Add a breakpoint at code address <addr>\n");
	printf("bp list         -- List all breakpoints and their enabled status\n");
	printf("bp disable <id> -- Disable the breakpoint with ID <id>\n");
	printf("bp enable <id>  -- Enable the breakpoint with ID <id>\n");
	printf("bp toggle <id>  -- Toggle the enabled state of breakpoint with ID <id>\n");
	printf("bp clear        -- Clear all breakpoints\n");
}


#define CHECK_ARG_COUNT(argc) do { \
	if(cmd->args.count != (argc)) { \
		printf("Wrong argument count for %s %s\n", cmd->args.elems[0], cmd->args.elems[1]); \
		DebugEngine_helpBreakpoint(self); \
		return; \
	} \
} while(0)

static void DebugEngine_doBreakpoint(DebugEngine* self, Command* cmd) {
	char* end;
	const char* subcmd;
	const char* str;
	Word addr;
	int bpid;
	bool enabled;
	
	if(cmd->args.count < 2) {
		DebugEngine_helpBreakpoint(self);
		return;
	}
	
	subcmd = cmd->args.elems[1];
	if(strcasecmp(subcmd, "add") == 0 || isdigit(subcmd[0])) {
		/* Allow a short form like "b <addr>" */
		if(isdigit(subcmd[0])) {
			CHECK_ARG_COUNT(2);
			str = subcmd;
		}
		else {
			CHECK_ARG_COUNT(3);
			str = cmd->args.elems[2];
		}
		
		/* Parse addr */
		addr = (Word)strtol(str, &end, 0);
		if(*end != '\0') {
			printf("Invalid address given to `breakpoint add`\n");
			DebugEngine_helpBreakpoint(self);
			return;
		}
		
		/* Add breakpoint */
		bpid = Machine_addBreakpoint(self->cpu, addr);
		printf("Created breakpoint #%d at address %"PRIdWORD"\n", bpid + 1, addr);
	}
	else if(strcasecmp(subcmd, "list") == 0) {
		CHECK_ARG_COUNT(2);
		
		/* List breakpoints */
		printf("Breakpoints:\n");
		enumerate(&self->cpu->bps, i, bp) {
			printf("Breakpoint #%d at address %"PRIdWORD" is %sabled\n",
				(int)i + 1, bp->addr, bp->enabled ? "en" : "dis");
		}
	}
	else if(strcasecmp(subcmd, "disable") == 0) {
		CHECK_ARG_COUNT(3);
		
		/* Parse id */
		bpid = (int)strtol(cmd->args.elems[2], &end, 0);
		if(*end != '\0') {
			printf("Invalid breakpoint ID given to `breakpoint disable`\n");
			DebugEngine_helpBreakpoint(self);
			return;
		}
		--bpid;
		
		/* Disable breakpoint */
		Machine_disableBreakpoint(self->cpu, bpid);
		printf("Disabled breakpoint #%d\n", bpid);
	}
	else if(strcasecmp(subcmd, "enable") == 0) {
		CHECK_ARG_COUNT(3);
		
		/* Parse id */
		bpid = (int)strtol(cmd->args.elems[2], &end, 0);
		if(*end != '\0') {
			printf("Invalid breakpoint ID given to `breakpoint enable`\n");
			DebugEngine_helpBreakpoint(self);
			return;
		}
		--bpid;
		
		/* Enable breakpoint */
		Machine_enableBreakpoint(self->cpu, bpid);
		printf("Enabled breakpoint #%d\n", bpid);
	}
	else if(strcasecmp(subcmd, "toggle") == 0) {
		CHECK_ARG_COUNT(3);
		
		/* Parse id */
		bpid = (int)strtol(cmd->args.elems[2], &end, 0);
		if(*end != '\0') {
			printf("Invalid breakpoint ID given to `breakpoint toggle`\n");
			DebugEngine_helpBreakpoint(self);
			return;
		}
		--bpid;
		
		/* Toggle breakpoint */
		enabled = Machine_toggleBreakpoint(self->cpu, bpid);
		printf("Toggled breakpoint #%d %s\n", bpid, enabled ? "on" : "off");
	}
	else if(strcasecmp(subcmd, "clear") == 0) {
		CHECK_ARG_COUNT(2);
		
		/* Clear breakpoints */
		Machine_clearBreakpoints(self->cpu);
		printf("Cleared all breakpoints\n");
	}
	else {
		printf("Invalid breakpoint command!\n");
		DebugEngine_helpBreakpoint(self);
	}
}

static void DebugEngine_helpRunning(DebugEngine* self) {
	(void)self;
	printf("Available running commands:\n");
	printf("continue        -- Run until a breakpoint is encountered or the program halts\n");
	printf("step            -- Runs a single instruction and returns to the debugger\n");
	printf("state           -- Shows stack contents and register values\n");
}

static void DebugEngine_doContinue(DebugEngine* self, Command* cmd) {
	if(cmd->args.count != 1) {
		printf("Wrong argument count for continue\n");
		DebugEngine_helpRunning(self);
		return;
	}
	
	/* Continue execution */
	if(Machine_continue(self->cpu) == STATUS_PAUSED) {
		if(self->cpu->codemem[self->cpu->state.pc].imm == OP_BREAK) {
			printf("Hit breakpoint at address %"PRIdWORD"\n", self->cpu->state.pc);
		}
		else {
			printf("Program paused at address %"PRIdWORD"\n", self->cpu->state.pc);
		}
	}
	Machine_printState(self->cpu, stdout);
}

static void DebugEngine_doState(DebugEngine* self, Command* cmd) {
	if(cmd->args.count != 1) {
		printf("Wrong argument count for state\n");
		DebugEngine_helpRunning(self);
		return;
	}
	
	/* Display the machine's state */
	Machine_printState(self->cpu, stdout);
}

static void DebugEngine_doStep(DebugEngine* self, Command* cmd) {
	if(cmd->args.count != 1) {
		printf("Wrong argument count for state\n");
		DebugEngine_helpRunning(self);
		return;
	}
	
	/* Step a single instruction */
	Machine_step(self->cpu);
	Machine_printState(self->cpu, stdout);
}

static void DebugEngine_doHelp(DebugEngine* self, Command* cmd) {
	if(cmd->args.count >= 2) {
		/* Specific help for commands */
		CMDTYPE type = Command_getType(cmd->args.elems[1]);
		if(type == 0) {
			if(strcasecmp(cmd->args.elems[1], "running") == 0) {
				/* Set the type to some random running command */
				type = CMD_CONTINUE;
			}
		}
		
		switch(type) {
			case CMD_BREAKPOINT:
				DebugEngine_helpBreakpoint(self);
				return;
			
			case CMD_CONTINUE:
			case CMD_STATE:
			case CMD_STEP:
				DebugEngine_helpRunning(self);
				return;
			
			default:
				/* Fall back to general help */
				break;
		}
	}
	
	/* General help */
	printf("Available topics (type help <topic> to learn more):\n");
	printf("breakpoint      -- Setting and modifying breakpoints\n");
	printf("running         -- Controlling how a program runs and getting info\n");
	printf("quit            -- Exit the debugger and stop execution\n");
}
