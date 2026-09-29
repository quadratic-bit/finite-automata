#include <automata/dfa.h>

#include "dfa_builder.h"
#include "stateset.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
	ALPHABET_SIZE = 256,
};

static size_t nfa_extract_alphabet(const Nfa *nfa, unsigned char alphabet[ALPHABET_SIZE]) {
	unsigned char seen[ALPHABET_SIZE] = {0};

	for (size_t i = 0; i < nfa->transition_count; ++i) {
		const NfaTransition *transition = &nfa->transitions[i];

		if (transition->kind == NFA_TRANSITION_SYMBOL) {
			seen[transition->symbol] = 1;
		}
	}

	size_t count = 0;

	for (size_t symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
		if (!seen[symbol]) continue;

		alphabet[count] = (unsigned char)symbol;
		count++;
	}

	return count;
}

static void nfa_move(const Nfa *nfa, const StateSet *from, unsigned char symbol, StateSet *to) {
	stateset_clear(to);

	for (size_t i = 0; i < nfa->transition_count; ++i) {
		const NfaTransition *transition = &nfa->transitions[i];

		if (transition->kind != NFA_TRANSITION_SYMBOL) {
			continue;
		}

		if (transition->symbol != symbol) {
			continue;
		}

		if (!stateset_contains(from, transition->from)) {
			continue;
		}

		stateset_add(to, transition->to);
	}
}

static void nfa_epsilon_closure(const Nfa *nfa, StateSet *set, StateId *stack) {
	size_t stack_len = 0;

	for (StateId state = 0; state < nfa->state_count; ++state) {
		if (stateset_contains(set, state)) {
			stack[stack_len] = state;
			stack_len++;
		}
	}

	while (stack_len > 0) {
		stack_len--;

		StateId state = stack[stack_len];

		for (size_t i = 0; i < nfa->transition_count; ++i) {
			const NfaTransition *transition = &nfa->transitions[i];

			if (transition->kind != NFA_TRANSITION_EPSILON) {
				continue;
			}

			if (transition->from != state) {
				continue;
			}

			if (stateset_contains(set, transition->to)) {
				continue;
			}

			stateset_add(set, transition->to);

			stack[stack_len] = transition->to;
			stack_len++;
		}
	}
}

DfaResult dfa_from_nfa(Dfa *dfa, const Nfa *nfa) {
	assert(dfa != NULL);
	assert(nfa != NULL);

	assert(nfa->state_count > 0);
	assert(nfa->start  < nfa->state_count);
	assert(nfa->accept < nfa->state_count);

	assert(dfa->state_count    == 0);
	assert(dfa->alphabet       == NULL);
	assert(dfa->alphabet_count == 0);
	assert(dfa->accepting      == NULL);
	assert(dfa->transitions    == NULL);

	unsigned char alphabet[ALPHABET_SIZE];
	size_t alphabet_count = nfa_extract_alphabet(nfa, alphabet);

	size_t word_count = nfa->state_count / 64 + (size_t)(nfa->state_count % 64 != 0);

	DfaBuilder builder;
	dfa_builder_init(&builder, alphabet, alphabet_count, word_count);

	uint64_t *start_words = NULL;
	uint64_t *next_words  = NULL;
	StateId  *stack       = NULL;

	if (builder.word_count > SIZE_MAX / sizeof(uint64_t)) {
		goto fail;
	}

	start_words = calloc(builder.word_count, sizeof *start_words);
	if (start_words == NULL) {
		goto fail;
	}

	next_words = calloc(builder.word_count, sizeof *next_words);
	if (next_words == NULL) {
		goto fail;
	}

	if (nfa->state_count > SIZE_MAX / sizeof(StateId)) {
		goto fail;
	}

	stack = malloc(nfa->state_count * sizeof *stack);
	if (stack == NULL) {
		goto fail;
	}

	StateSet start = {.words = start_words, .word_count = builder.word_count};
	StateSet next  = {.words = next_words,  .word_count = builder.word_count};

	stateset_add(&start, nfa->start);
	nfa_epsilon_closure(nfa, &start, stack);

	StateId start_state;

	if (dfa_builder_intern(&builder, &start, stateset_contains(&start, nfa->accept),
	                       &start_state) != DFA_OK) {
		goto fail;
	}

	for (StateId state = 0; state < builder.state_count; ++state) {
		for (size_t sym_index = 0; sym_index < builder.alphabet_count; ++sym_index) {
			StateSet current = dfa_builder_subset(&builder, state);

			nfa_move(nfa, &current, alphabet[sym_index], &next);
			nfa_epsilon_closure(nfa, &next, stack);

			StateId target;

			if (dfa_builder_intern(&builder, &next,
			                       stateset_contains(&next, nfa->accept),
			                       &target) != DFA_OK) {
				goto fail;
			}

			dfa_builder_set_transition(&builder, state, sym_index, target);
		}
	}

	if (dfa_builder_finish(&builder, start_state, dfa) != DFA_OK) {
		goto fail;
	}

	free(stack);
	free(next_words);
	free(start_words);

	return DFA_OK;

fail:
	dfa_builder_free(&builder);
	free(stack);
	free(next_words);
	free(start_words);

	return DFA_ERR;
}

void dfa_free(Dfa *dfa) {
	assert(dfa != NULL);

	free(dfa->alphabet);
	free(dfa->accepting);
	free(dfa->transitions);

	*dfa = (Dfa){0};
}
