//
//  transition.c
//  PL/0
//
//  Created by Kevin Colley on 10/12/15.
//  Copyright © 2015 Kevin Colley. All rights reserved.
//

#include "transition.h"


Destroyer(Transition) {
	destroy(&self->label);
	
	if(self->strong) {
		release(&self->state);
	}
}
DEF(Transition);

Transition* Transition_initWithLabel(Transition* self, const char* label, State* state, bool strong) {
	if((self = Transition_init(self))) {
		Transition_setLabel(self, label);
		self->state = strong ? retain(state) : state;
		self->strong = strong;
	}
	
	return self;
}

Transition* Transition_initWithMatcher(Transition* self, const char* label, State* state, bool strong, Matcher* matcher) {
	if((self = Transition_initWithLabel(self, label, state, strong))) {
		self->matcher = matcher;
	}
	
	return self;
}

Transition* Transition_initWithExact(Transition* self, const char* label, State* state, bool strong, char exact) {
	if((self = Transition_initWithLabel(self, label, state, strong))) {
		self->exact = exact;
	}
	
	return self;
}

void Transition_setLabel(Transition* self, const char* label) {
	self->label = html_str(label);
}

void Transition_drawGraph(Transition* self, Graphviz* gv, State* src) {
	/* Draw the target state only if this is a strong reference */
	if(self->strong) {
		State_drawGraph(self->state, gv);
	}
	
	/* Draw the transition edge */
	const char* style = self->strong ? "" : " style = dashed";
	Graphviz_draw(gv, "<%p> -> <%p> [label = < %s>%s];", src, self->state, self->label, style);
}
