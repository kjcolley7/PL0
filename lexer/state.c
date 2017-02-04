//
//  state.c
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "state.h"
#include <stdlib.h>
#include <string.h>


Destroyer(State) {
	destroy(&self->label);
	
	size_t i;
	for(i = 0; i < self->transition_count; i++) {
		release(&self->transitions[i]);
	}
	
	destroy(&self->transitions);
}
DEF(State);

State* State_initWithLabel(State* self, const char* label) {
	if((self = State_init(self))) {
		State_setLabel(self, label);
	}
	
	return self;
}

State* State_initWithAcceptor(State* self, const char* label, Acceptor* acceptfn) {
	if((self = State_initWithLabel(self, label))) {
		self->acceptor = true;
		self->acceptfn = acceptfn;
	}
	
	return self;
}

State* State_initWithType(State* self, const char* label, token_type type) {
	if((self = State_initWithLabel(self, label))) {
		self->acceptor = true;
		self->simple_type = type;
	}
	
	return self;
}

void State_setLabel(State* self, const char* label) {
	self->label = html_str(label);
}

void State_addTransition(State* self, Transition* trans) {
	if(trans == NULL) {
		return;
	}
	
	/* Enlarge transition array if necessary */
	if(self->transition_count == self->transition_cap) {
		expand(&self->transitions, &self->transition_cap);
	}
	
	/* Add the new transition */
	self->transitions[self->transition_count++] = retain(trans);
}

State* State_transition(State* self, char c) {
	size_t i;
	for(i = 0; i < self->transition_count; i++) {
		Transition* trans = self->transitions[i];
		bool match = trans->matcher ? trans->matcher(c) : trans->exact == c;
		if(match) {
			return trans->state;
		}
	}
	
	return NULL;
}

void State_drawGraph(State* self, Graphviz* gv) {
	/* Draw state node */
	const char* shape = self->acceptor ? "doublecircle" : "circle";
	Graphviz_draw(gv, "<%p> [label = <%s>, shape = %s];", self, self->label ?: " ", shape);
	
	/* Draw all transitions */
	size_t i;
	for(i = 0; i < self->transition_count; i++) {
		Transition_drawGraph(self->transitions[i], gv, self);
	}
}
