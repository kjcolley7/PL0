//
//  state.h
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_STATE_H
#define PL0_STATE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct State State;

#include "object.h"
#include "token.h"
#include "lexer.h"
#include "transition.h"
#include "graphviz.h"

typedef Token* Acceptor(Lexer*);

struct State {
	OBJECT_BASE;
	
	/* Label for the current state */
	char* label;
	
	/*! Number of transitions leading from this state to another */
	size_t transition_count;
	
	/*! Allocated capacity for state transitions */
	size_t transition_cap;
	
	/*! Allocated array of state transitions */
	Transition** transitions;
	
	/*! Whether the state is an acceptor state */
	bool acceptor;
	
	/*! Function pointer to accept the current state when no transitions are matched */
	Acceptor* acceptfn;
	
	/*! In place of an acceptor function, simple states can specify a token type */
	token_type simple_type;
};
DECL(State);


/*! Initialize a state with the specified name
 @param label Label for the current state
 */
State* State_initWithLabel(State* self, const char* label);

/*! Initialize the state as an acceptor state with the given callback function pointer
 @param label Label for the current state
 @param acceptfn Function to be called when this state accepts a lexeme
 */
State* State_initWithAcceptor(State* self, const char* label, Acceptor* acceptfn);

/*! Initialize the state as an acceptor state that creates a token of the specified type
 @param label Label for the current state
 @param type Type of token to create when this state accepts a lexeme
 */
State* State_initWithType(State* self, const char* label, token_type type);

/*! Safely sets the state's label
 @param label Label for the current state
 */
void State_setLabel(State* self, const char* label);

/*! Adds a transition from this state to another
 @param trans Transition from the current state to another one. Will be retained
 */
void State_addTransition(State* self, Transition* trans);

/*! Selects the state to transition to when receiving the specified character
 @param c Character received
 @return New state after transitioning using the specified character
 */
State* State_transition(State* self, char c);

/*! Draws the state and any transitions it holds
 @param gv Graphviz drawing object
 */
void State_drawGraph(State* self, Graphviz* gv);


#endif /* PL0_STATE_H */
