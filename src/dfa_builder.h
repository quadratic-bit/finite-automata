#ifndef DFA_BUILDER_H
#define DFA_BUILDER_H

#include <automata/dfa.h>

#include "stateset.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
	const unsigned char *alphabet;
	size_t               alphabet_count;

	size_t word_count;

	uint64_t      *subsets;
	unsigned char *accepting;
	StateId       *transitions;

	size_t state_count;
	size_t state_cap;
} DfaBuilder;

void dfa_builder_init(
	DfaBuilder          *builder,
	const unsigned char *alphabet,
	size_t               alphabet_count,
	size_t               word_count
);

StateSet dfa_builder_subset(DfaBuilder *builder, StateId state);

DfaResult dfa_builder_intern(
	DfaBuilder     *builder,
	const StateSet *subset,
	int             accepting,
	StateId        *state
);

void dfa_builder_set_transition(
	DfaBuilder *builder,
	StateId     from,
	size_t      symbol_index,
	StateId     to
);

DfaResult dfa_builder_finish(DfaBuilder *builder, StateId start, Dfa *dfa);

void dfa_builder_free(DfaBuilder *builder);

#endif
