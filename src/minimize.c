#include <automata/dfa.h>

#include "stateset.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const Dfa *source;

	size_t word_count;

	uint64_t      *subsets;
	unsigned char *accepting;
	StateId       *transitions;

	size_t state_count;
	size_t state_cap;
} ReverseBuilder;

static uint64_t *builder_subset(ReverseBuilder *builder, StateId state) {
	assert(state < builder->state_count);

	return &builder->subsets[state * builder->word_count];
}

static void builder_free(ReverseBuilder *builder) {
	free(builder->subsets);
	free(builder->accepting);
	free(builder->transitions);

	*builder = (ReverseBuilder){0};
}

static DfaResult builder_grow(ReverseBuilder *builder) {
	size_t new_cap;

	if (builder->state_cap == 0) {
		new_cap = 8;
	} else {
		if (builder->state_cap > SIZE_MAX / 2) {
			return DFA_ERR;
		}

		new_cap = builder->state_cap * 2;
	}

	if (builder->word_count != 0 && new_cap > SIZE_MAX / builder->word_count) {
		return DFA_ERR;
	}

	size_t subset_word_count = new_cap * builder->word_count;

	if (subset_word_count > SIZE_MAX / sizeof *builder->subsets) {
		return DFA_ERR;
	}

	uint64_t *subsets = realloc(builder->subsets, subset_word_count * sizeof *builder->subsets);

	if (subsets == NULL) {
		return DFA_ERR;
	}

	builder->subsets = subsets;

	unsigned char *accepting = realloc(
		builder->accepting,
		new_cap * sizeof *builder->accepting
	);

	if (accepting == NULL) {
		return DFA_ERR;
	}

	builder->accepting = accepting;

	if (builder->source->alphabet_count != 0) {
		if (new_cap > SIZE_MAX / builder->source->alphabet_count) {
			return DFA_ERR;
		}

		size_t transition_count = new_cap * builder->source->alphabet_count;

		if (transition_count > SIZE_MAX / sizeof *builder->transitions) {
			return DFA_ERR;
		}

		StateId *transitions = realloc(
			builder->transitions,
			transition_count * sizeof *builder->transitions
		);

		if (transitions == NULL) {
			return DFA_ERR;
		}

		builder->transitions = transitions;
	}

	builder->state_cap = new_cap;
	return DFA_OK;
}

static DfaResult builder_intern(ReverseBuilder *builder, const StateSet *subset, StateId *state) {
	for (size_t i = 0; i < builder->state_count; ++i) {
		StateSet existing = {
			.words      = builder_subset(builder, i),
			.word_count = builder->word_count,
		};

		if (stateset_equal(&existing, subset)) {
			*state = i;
			return DFA_OK;
		}
	}

	if (builder->state_count == builder->state_cap && builder_grow(builder) != DFA_OK) {
		return DFA_ERR;
	}

	size_t id = builder->state_count;

	memcpy(
		&builder->subsets[id * builder->word_count],
		subset->words,
		builder->word_count * sizeof *subset->words
	);

	builder->accepting[id] = (unsigned char)stateset_contains(subset, builder->source->start);
	builder->state_count++;

	*state = id;
	return DFA_OK;
}

static void builder_set_transition(
	ReverseBuilder *builder,
	StateId from,
	size_t symbol_index,
	StateId to
) {
	assert(from < builder->state_count);
	assert(to   < builder->state_count);
	assert(symbol_index < builder->source->alphabet_count);

	builder->transitions[from * builder->source->alphabet_count + symbol_index] = to;
}

static void reverse_move(
	const Dfa *source,
	const StateSet *from,
	size_t symbol_index,
	StateSet *to
) {
	stateset_clear(to);

	for (StateId state = 0; state < source->state_count; ++state) {
		StateId target = source->transitions[
			state * source->alphabet_count
			+ symbol_index
		];

		assert(target < source->state_count);

		if (stateset_contains(from, target)) {
			stateset_add(to, state);
		}
	}
}

static DfaResult reverse_determinize(Dfa *dfa, const Dfa *source) {
	assert(dfa    != NULL);
	assert(source != NULL);
	assert(dfa    != source);

	assert(source->state_count > 0);
	assert(source->start < source->state_count);

	assert(dfa->state_count    == 0);
	assert(dfa->alphabet       == NULL);
	assert(dfa->alphabet_count == 0);
	assert(dfa->accepting      == NULL);
	assert(dfa->transitions    == NULL);

	ReverseBuilder builder = {.source = source};

	builder.word_count = source->state_count / 64 + (size_t)(source->state_count % 64 != 0);

	if (builder.word_count > SIZE_MAX / sizeof(uint64_t)) {
		return DFA_ERR;
	}

	uint64_t *initial_words = calloc(builder.word_count, sizeof *initial_words);

	if (initial_words == NULL) {
		return DFA_ERR;
	}

	uint64_t *next_words = calloc(builder.word_count, sizeof *next_words);

	if (next_words == NULL) {
		free(initial_words);
		return DFA_ERR;
	}

	StateSet initial = {
		.words      = initial_words,
		.word_count = builder.word_count,
	};

	StateSet next = {
		.words      = next_words,
		.word_count = builder.word_count,
	};

	for (StateId state = 0; state < source->state_count; ++state) {
		if (source->accepting[state]) {
			stateset_add(&initial, state);
		}
	}

	StateId initial_state;

	if (builder_intern(&builder, &initial, &initial_state) != DFA_OK) {
		builder_free(&builder);
		free(next_words);
		free(initial_words);

		return DFA_ERR;
	}

	for (StateId state = 0; state < builder.state_count; ++state) {
		for (size_t sym_index = 0; sym_index < source->alphabet_count; ++sym_index) {
			StateSet current = {
				.words      = builder_subset(&builder, state),
				.word_count = builder.word_count,
			};

			reverse_move(source, &current, sym_index, &next);

			StateId target;

			if (builder_intern(&builder, &next, &target) != DFA_OK) {
				builder_free(&builder);
				free(next_words);
				free(initial_words);

				return DFA_ERR;
			}

			builder_set_transition(&builder, state, sym_index, target);
		}
	}

	unsigned char *alphabet = NULL;

	if (source->alphabet_count != 0) {
		alphabet = malloc(source->alphabet_count * sizeof *alphabet);

		if (alphabet == NULL) {
			builder_free(&builder);
			free(next_words);
			free(initial_words);

			return DFA_ERR;
		}

		memcpy(alphabet, source->alphabet, source->alphabet_count * sizeof *alphabet);
	}

	free(builder.subsets);
	builder.subsets = NULL;

	*dfa = (Dfa){
		.state_count    = builder.state_count,
		.start          = initial_state,
		.alphabet       = alphabet,
		.alphabet_count = source->alphabet_count,
		.accepting      = builder.accepting,
		.transitions    = builder.transitions,
	};

	builder.accepting   = NULL;
	builder.transitions = NULL;

	free(next_words);
	free(initial_words);

	return DFA_OK;
}

DfaResult dfa_minimize(Dfa *dfa, const Dfa *source) {
	assert(dfa    != NULL);
	assert(source != NULL);
	assert(dfa    != source);

	assert(dfa->state_count    == 0);
	assert(dfa->alphabet       == NULL);
	assert(dfa->alphabet_count == 0);
	assert(dfa->accepting      == NULL);
	assert(dfa->transitions    == NULL);

	Dfa reversed = {0};

	/* det(reverse(source)) */
	if (reverse_determinize(&reversed, source) != DFA_OK) {
		return DFA_ERR;
	}

	/* det(reverse(det(reverse(source)))) */
	if (reverse_determinize(dfa, &reversed) != DFA_OK) {
		dfa_free(&reversed);
		return DFA_ERR;
	}

	dfa_free(&reversed);

	return DFA_OK;
}
