//
//  debugengine.h
//  PL/0
//
//  Created by Kevin Colley on 11/23/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_DEBUGENGINE_H
#define PL0_DEBUGENGINE_H

#include <stdbool.h>

typedef struct DebugEngine DebugEngine;

#include "object.h"
#include "machine.h"

struct DebugEngine {
	OBJECT_BASE;
	
	/*! Machine to run in the debugger */
	Machine* cpu;
};
DECL(DebugEngine);


/*! Initializes a debugger object with the cpu it will be debugging
 @param cpu Machine to run in the debugger
 */
DebugEngine* DebugEngine_initWithCPU(DebugEngine* self, Machine* cpu);

/*! Run the program through the debugger
 @return True on success, or false on error
 */
bool DebugEngine_run(DebugEngine* self);


#endif /* PL0_DEBUGENGINE_H */
