//
//  transition.h
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#ifndef PL0_TRANSITION_H
#define PL0_TRANSITION_H

#include <stdbool.h>

typedef struct Transition Transition;

#include "object.h"
#include "state.h"
#include "graphviz.h"

typedef bool Matcher(char);

struct Transition {
	OBJECT_BASE;
	
	/*! Label that will appear on the transition edge in the graph */
	char* label;
	
	/*! Function to determine whether a character should transition to the next state */
	Matcher* matcher;
	
	/*! For simple options, a single character that must be matched to transition */
	char exact;
	
	/*! Whether the reference to state is strong or weak */
	bool strong;
	
	/*! Target state that is transitioned to when the character matches */
	State* state;
};
DECL(Transition);

/*! Initialize the transition with the given label and target state
 @param label Label that will appear on the transition edge in the graph
 @param state State transitioned to when the condition is met
 @param strong Whether the reference to state should be strong or weak
 */
Transition* Transition_initWithLabel(Transition* self, const char* label, State* state, bool strong);

/*! Initialize the transition with a function that matches which characters cause a transition
 @param label Label that will appear on the transition edge in the graph
 @param state State transitioned to when the condition is met
 @param strong Whether the reference to state should be strong or weak
 @param matcher Function that takes the current character and returns true when it matches
 */
Transition* Transition_initWithMatcher(Transition* self, const char* label, State* state, bool strong, Matcher* matcher);

/*! Initialize the transition with an exact character to go to the next state
 @param label Label that will appear on the transition edge in the graph
 @param state State transitioned to when the condition is met
 @param strong Whether the reference to state should be strong or weak
 @param exact Character to match in order to transition to the provided state
 */
Transition* Transition_initWithExact(Transition* self, const char* label, State* state, bool strong, char exact);

/*! Safely sets the transition's label
 @param label Label that will appear on the transition edge in the graph
 */
void Transition_setLabel(Transition* self, const char* label);

/*! Draws the transition edge and any states strongly referenced
 @param gv Graphviz drawing object
 @param src State the transition originates from
 */
void Transition_drawGraph(Transition* self, Graphviz* gv, State* src);


#endif /* PL0_TRANSITION_H */
