#include <automata/dfa.h>

#include "dfa_builder.h"
#include "stateset.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void reverse_move(
	const Dfa *source,
	const StateSet *from,
	size_t symbol_index,
	StateSet *to
) {
	stateset_clear(to);

	for (StateId state = 0; state < source->state_count; ++state) {
		StateId target = source->transitions[state * source->alphabet_count + symbol_index];
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

	size_t word_count = source->state_count / 64 + (size_t)(source->state_count % 64 != 0);

	DfaBuilder builder;
	dfa_builder_init(&builder, source->alphabet, source->alphabet_count, word_count);

	uint64_t *initial_words = NULL;
	uint64_t *next_words    = NULL;

	if (builder.word_count > SIZE_MAX / sizeof(uint64_t)) {
		goto fail;
	}

	initial_words = calloc(builder.word_count, sizeof *initial_words);
	if (initial_words == NULL) {
		goto fail;
	}

	next_words = calloc(builder.word_count, sizeof *next_words);
	if (next_words == NULL) {
		goto fail;
	}

	StateSet initial = {.words = initial_words, .word_count = builder.word_count};
	StateSet next    = {.words = next_words,    .word_count = builder.word_count};

	for (StateId state = 0; state < source->state_count; ++state) {
		if (source->accepting[state]) {
			stateset_add(&initial, state);
		}
	}

	StateId initial_state;

	if (dfa_builder_intern(&builder, &initial, stateset_contains(&initial, source->start),
	                       &initial_state) != DFA_OK) {
		goto fail;
	}

	for (StateId state = 0; state < builder.state_count; ++state) {
		for (size_t sym_index = 0; sym_index < source->alphabet_count; ++sym_index) {
			StateSet current = dfa_builder_subset(&builder, state);

			reverse_move(source, &current, sym_index, &next);

			StateId target;

			if (dfa_builder_intern(&builder, &next,
			                       stateset_contains(&next, source->start),
			                       &target) != DFA_OK) {
				goto fail;
			}

			dfa_builder_set_transition(&builder, state, sym_index, target);
		}
	}

	if (dfa_builder_finish(&builder, initial_state, dfa) != DFA_OK) {
		goto fail;
	}

	free(next_words);
	free(initial_words);

	return DFA_OK;

fail:
	dfa_builder_free(&builder);
	free(next_words);
	free(initial_words);

	return DFA_ERR;
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
