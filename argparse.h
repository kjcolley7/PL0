//
//  argparse.h
//  PL/0
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#ifndef PL0_ARGPARSE_H
#define PL0_ARGPARSE_H

#include "macros.h"


#if 0 && EXAMPLE
static int argtest(int argc, char* argv[]) {
	bool flag = false;
	
	ARGPARSE(argc, argv) {
		ARG('h', "help", "Display this help message") {
			USAGE();
			return 0;
		}
		ARG('t', "test", "This is a test lol") {
			int i;
			for(i = 0; i < 10; i++) {
				printf("%d\n", i);
			}
		}
		ARG('f', "flag", "Turns this flag on") {
			flag = true;
		}
		ARG_OTHER(arg) {
			printf("ARG_OTHER: %s\n", arg);
		}
	}
	
	if(flag) {
		printf("Flag is on!");
	}
	
	printf("Done!\n");
	return 0;
}
#endif /* EXAMPLE */


#define ARGPARSE(argc, argv) \
for(int _argparse_once = 1; _argparse_once; _argparse_once = 0) \
	for(struct argparse _argparse_info = {}; _argparse_once; _argparse_once = 0) \
		for(int _argidx = 1, _arg = ARG_VALUE_INIT; \
			_arg != ARG_VALUE_DONE; \
			_arg = _argparse_parse(&_argparse_info, &_argidx, argc, argv))

#define MAKE_ARG_VALUE(value) ((value) | ARG_NORMAL_BIT)

#define ARG(short_name, long_name, description) ARG_(__COUNTER__, short_name, long_name, description)
#define ARG_(id, short_name, long_name, description) ARG__(id, short_name, long_name, description)
#define ARG__(id, short_name, long_name, description) \
if(_arg == ARG_VALUE_INIT) { \
	_argparse_add(&_argparse_info, MAKE_ARG_VALUE(id), short_name, long_name, description); \
} \
else if(_arg == MAKE_ARG_VALUE(id))

#define ARG_OTHER(argname) \
for(const char* argname = argv[_argidx-1]; argname != NULL; argname = NULL) \
	if(_arg == ARG_VALUE_INIT) { \
		_argparse_info.has_catchall = 1; \
	} \
	else if(_arg == ARG_VALUE_OTHER)

#define USAGE() _argparse_usage(&_argparse_info, argv[0])


#define LONG_ARG_MAX_WIDTH 30
#define ARG_VALUE_INIT 0
#define ARG_VALUE_ERROR (-1)
#define ARG_VALUE_DONE (-2)
#define ARG_VALUE_OTHER (-3)
#define ARG_NORMAL_BIT (0x80)


struct argparse {
	size_t arg_count;
	size_t arg_cap;
	struct arg_info {
		int arg_id;
		char short_name;
		const char* long_name;
		const char* description;
	} *args;
	int long_name_width;
	int has_catchall;
};

static inline void _argparse_add(struct argparse* argparse_info, int arg_id, char short_name, const char* long_name, const char* description) {
	/* Expand the args array if it's full */
	if(argparse_info->arg_count == argparse_info->arg_cap) {
		expand(&argparse_info->args, &argparse_info->arg_cap);
	}
	
	/* Build the argument description structure */
	struct arg_info* arg = &argparse_info->args[argparse_info->arg_count++];
	arg->arg_id = arg_id;
	arg->short_name = short_name;
	arg->long_name = long_name;
	arg->description = description;
	
	/* Update max long_name_width while respecting the upper bound */
	if(arg->long_name != NULL) {
		int arglen = (int)MIN(strlen(arg->long_name), LONG_ARG_MAX_WIDTH);
		if(arglen > argparse_info->long_name_width) {
			argparse_info->long_name_width = arglen;
		}
	}
}

static inline int _argparse_parse(struct argparse* argparse_info, int* argidx, int argc, char** argv) {
	int ret = ARG_VALUE_OTHER;
	
	/* Did we parse all of the arguments? */
	if(*argidx == argc) {
		ret = ARG_VALUE_DONE;
		goto out;
	}
	
	const char* arg = argv[(*argidx)++];
	if(arg[0] != '-') {
		/* Argument doesn't start with a dash, so it's an "other" argument */
		ret = ARG_VALUE_OTHER;
		goto out;
	}
	
	/* Get argument length just once */
	size_t arglen = strlen(arg);
	if(arglen == 2) {
		/* This is a short argument, so check all the short args */
		size_t i;
		for(i = 0; i < argparse_info->arg_count; i++) {
			if(arg[1] == argparse_info->args[i].short_name) {
				ret = argparse_info->args[i].arg_id;
				goto out;
			}
		}
	}
	else if(arglen < 2 || arg[1] != '-') {
		/*
		 * This is either an empty string, a single dash, or a long arg with only
		 * a single dash at the beginning. All of those are "other" cases.
		 */
		ret = ARG_VALUE_OTHER;
		goto out;
	}
	else {
		/* This is a properly formed long argument, so check all the long args */
		size_t i;
		for(i = 0; i < argparse_info->arg_count; i++) {
			if(strcmp(&arg[2], argparse_info->args[i].long_name) == 0) {
				ret = argparse_info->args[i].arg_id;
				goto out;
			}
		}
	}
	
out:
	/* Did we fail to parse this argument? */
	if(ret == ARG_VALUE_OTHER) {
		/* Error out unless there's an ARG_OTHER catchall present */
		if(argparse_info->has_catchall) {
			return ret;
		}
		else {
			printf("Unknown argument: %s\n", argv[*argidx]);
			return ARG_VALUE_ERROR;
		}
	}
	
	return ret;
}

static inline int charcmp(const void* a, const void* b) {
	int x = *(const char*)a;
	int y = *(const char*)b;
	return x - y;
}

static inline void _argparse_usage(struct argparse* argparse_info, const char* progname) {
	char shortOptions[256] = {};
	size_t shortOptionCount = 0;
	size_t i;
	
	/* Get all short option names */
	for(i = 0; i < argparse_info->arg_count; i++) {
		struct arg_info* arg = &argparse_info->args[i];
		if(arg->short_name != 0) {
			assert(shortOptionCount < sizeof(shortOptions));
			shortOptions[shortOptionCount++] = arg->short_name;
		}
	}
	
	/* Print usage header with program name */
	printf("Usage: %s", progname);
	
	if(shortOptionCount > 0) {
		/* Sort short option names by their ASCII values */
		qsort(shortOptions, shortOptionCount, sizeof(*shortOptions), charcmp);
		
		/* Print all available short options */
		printf(" [-%s]", shortOptions);
	}
	
	printf("\n");
	printf("Options:\n");
	
	/* Print description of each argument */
	for(i = 0; i < argparse_info->arg_count; i++) {
		struct arg_info* arg = &argparse_info->args[i];
		printf("    ");
		
		/* Print short option (if set) */
		if(arg->short_name != '\0') {
			printf("-%c", arg->short_name);
		}
		else {
			printf("  ");
		}
		
		/* Print long option (if set) */
		if(arg->long_name != NULL) {
			/* If short option was set, separate it with a comma */
			if(arg->short_name != '\0') {
				printf(", ");
			}
			else {
				printf("  ");
			}
			
			/* Print long option */
			printf("--%-*s", argparse_info->long_name_width, arg->long_name);
		}
		else {
			/* Print spaces to pad the space where a long option would normally be */
			printf("%*s", 2 + argparse_info->long_name_width, "");
		}
		
		/* Print the description (if set) */
		if(arg->description != NULL) {
			printf("  %s", arg->description);
		}
		
		/* End the line */
		printf("\n");
	}
}

#endif /* PL0_ARGPARSE_H */
