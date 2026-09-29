#include "dfa_builder.h"

#include "vec.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const size_t DEFAULT_BUILDER_CAP = 8;

static DfaResult builder_grow(DfaBuilder *builder) {
	size_t new_cap;

	if (!vec_next_cap(builder->state_cap, DEFAULT_BUILDER_CAP, &new_cap)) {
		return DFA_ERR;
	}

	if (builder->word_count != 0 && new_cap > SIZE_MAX / builder->word_count) {
		return DFA_ERR;
	}

	size_t subset_words = new_cap * builder->word_count;

	uint64_t *subsets = vec_realloc(builder->subsets, subset_words, sizeof *builder->subsets);
	if (subsets == NULL) {
		return DFA_ERR;
	}

	builder->subsets = subsets;

	unsigned char *accepting = vec_realloc(
		builder->accepting,
		new_cap,
		sizeof *builder->accepting
	);

	if (accepting == NULL) {
		return DFA_ERR;
	}

	builder->accepting = accepting;

	if (builder->alphabet_count != 0) {
		if (new_cap > SIZE_MAX / builder->alphabet_count) {
			return DFA_ERR;
		}

		size_t transition_count = new_cap * builder->alphabet_count;

		StateId *transitions = vec_realloc(
			builder->transitions,
			transition_count,
			sizeof *builder->transitions
		);

		if (transitions == NULL) {
			return DFA_ERR;
		}

		builder->transitions = transitions;
	}

	builder->state_cap = new_cap;
	return DFA_OK;
}

void dfa_builder_init(
	DfaBuilder          *builder,
	const unsigned char *alphabet,
	size_t               alphabet_count,
	size_t               word_count
) {
	assert(builder  != NULL);
	assert(alphabet != NULL || alphabet_count == 0);
	assert(word_count > 0);

	*builder = (DfaBuilder){
		.alphabet       = alphabet,
		.alphabet_count = alphabet_count,
		.word_count     = word_count,
	};
}

StateSet dfa_builder_subset(DfaBuilder *builder, StateId state) {
	assert(builder != NULL);
	assert(state < builder->state_count);

	return (StateSet){
		.words      = &builder->subsets[state * builder->word_count],
		.word_count = builder->word_count,
	};
}

DfaResult dfa_builder_intern(
	DfaBuilder     *builder,
	const StateSet *subset,
	int             accepting,
	StateId        *state
) {
	assert(builder != NULL);
	assert(subset  != NULL);
	assert(state   != NULL);
	assert(subset->word_count == builder->word_count);

	for (StateId id = 0; id < builder->state_count; ++id) {
		StateSet existing = dfa_builder_subset(builder, id);

		if (stateset_equal(&existing, subset)) {
			*state = id;
			return DFA_OK;
		}
	}

	if (builder->state_count == builder->state_cap && builder_grow(builder) != DFA_OK) {
		return DFA_ERR;
	}

	StateId id = builder->state_count;

	memcpy(&builder->subsets[id * builder->word_count],
	       subset->words, builder->word_count * sizeof *subset->words);

	builder->accepting[id] = (unsigned char)(accepting != 0);
	builder->state_count++;

	*state = id;
	return DFA_OK;
}

void dfa_builder_set_transition(
	DfaBuilder *builder,
	StateId     from,
	size_t      symbol_index,
	StateId     to
) {
	assert(builder != NULL);
	assert(from         < builder->state_count);
	assert(to           < builder->state_count);
	assert(symbol_index < builder->alphabet_count);

	builder->transitions[from * builder->alphabet_count + symbol_index] = to;
}

DfaResult dfa_builder_finish(DfaBuilder *builder, StateId start, Dfa *dfa) {
	assert(builder != NULL);
	assert(dfa     != NULL);
	assert(builder->state_count > 0);
	assert(start < builder->state_count);

	assert(dfa->state_count    == 0);
	assert(dfa->alphabet       == NULL);
	assert(dfa->alphabet_count == 0);
	assert(dfa->accepting      == NULL);
	assert(dfa->transitions    == NULL);

	unsigned char *alphabet = NULL;

	if (builder->alphabet_count != 0) {
		alphabet = malloc(builder->alphabet_count * sizeof *alphabet);
		if (alphabet == NULL) {
			return DFA_ERR;
		}

		memcpy(alphabet, builder->alphabet, builder->alphabet_count * sizeof *alphabet);
	}

	*dfa = (Dfa){
		.state_count    = builder->state_count,
		.start          = start,
		.alphabet       = alphabet,
		.alphabet_count = builder->alphabet_count,
		.accepting      = builder->accepting,
		.transitions    = builder->transitions,
	};

	free(builder->subsets);

	builder->subsets     = NULL;
	builder->accepting   = NULL;
	builder->transitions = NULL;

	*builder = (DfaBuilder){0};

	return DFA_OK;
}

void dfa_builder_free(DfaBuilder *builder) {
	assert(builder != NULL);

	free(builder->subsets);
	free(builder->accepting);
	free(builder->transitions);

	*builder = (DfaBuilder){0};
}
