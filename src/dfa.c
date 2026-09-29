#include <automata/dfa.h>

#include "stateset.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
	ALPHABET_SIZE = 256,
};

typedef struct {
	const Nfa *nfa;

	unsigned char alphabet[ALPHABET_SIZE];
	size_t        alphabet_count;

	size_t word_count;

	uint64_t      *subsets;
	unsigned char *accepting;
	StateId       *transitions;

	size_t state_count;
	size_t state_cap;
} DfaBuilder;

static uint64_t *builder_subset(DfaBuilder *builder, StateId state) {
	assert(state < builder->state_count);

	return &builder->subsets[state * builder->word_count];
}

static void builder_extract_alphabet(DfaBuilder *builder) {
	unsigned char seen[ALPHABET_SIZE] = {0};

	for (size_t i = 0; i < builder->nfa->transition_count; ++i) {
		const NfaTransition *transition = &builder->nfa->transitions[i];

		if (transition->kind == NFA_TRANSITION_SYMBOL) {
			seen[transition->symbol] = 1;
		}
	}

	for (size_t symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
		if (!seen[symbol]) continue;

		builder->alphabet[builder->alphabet_count] = (unsigned char)symbol;
		builder->alphabet_count++;
	}
}

static DfaResult builder_grow_states(DfaBuilder *builder) {
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

	if (builder->alphabet_count != 0) {
		if (new_cap > SIZE_MAX / builder->alphabet_count) {
			return DFA_ERR;
		}

		size_t transition_count = new_cap * builder->alphabet_count;

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

static DfaResult builder_intern_subset(
	DfaBuilder     *builder,
	const StateSet *subset,
	StateId        *state
) {
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

	if (builder->state_count == builder->state_cap) {
		if (builder_grow_states(builder) != DFA_OK) {
			return DFA_ERR;
		}
	}

	size_t id = builder->state_count;

	memcpy(
		&builder->subsets[id * builder->word_count],
		subset->words,
		builder->word_count * sizeof *subset->words
	);

	builder->accepting[id] = (unsigned char)stateset_contains(subset, builder->nfa->accept);
	builder->state_count++;

	*state = id;
	return DFA_OK;
}

static void builder_set_transition(
	DfaBuilder *builder,
	StateId     from,
	size_t      symbol_index,
	StateId     to
) {
	assert(from         < builder->state_count);
	assert(to           < builder->state_count);
	assert(symbol_index < builder->alphabet_count);

	builder->transitions[from * builder->alphabet_count + symbol_index] = to;
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

static void builder_free(DfaBuilder *builder) {
	free(builder->subsets);
	free(builder->accepting);
	free(builder->transitions);

	*builder = (DfaBuilder){0};
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

	DfaBuilder builder = {.nfa = nfa};

	builder.word_count = nfa->state_count / 64 + (size_t)(nfa->state_count % 64 != 0);

	builder_extract_alphabet(&builder);

	if (builder.word_count > SIZE_MAX / sizeof(uint64_t)) {
		return DFA_ERR;
	}

	uint64_t *start_words = calloc(builder.word_count, sizeof *start_words);

	if (start_words == NULL) {
		return DFA_ERR;
	}

	uint64_t *next_words = calloc(builder.word_count, sizeof *next_words);

	if (next_words == NULL) {
		free(start_words);
		return DFA_ERR;
	}

	if (nfa->state_count > SIZE_MAX / sizeof(StateId)) {
		free(next_words);
		free(start_words);
		return DFA_ERR;
	}

	StateId *stack = malloc(nfa->state_count * sizeof *stack);

	if (stack == NULL) {
		free(next_words);
		free(start_words);
		return DFA_ERR;
	}

	StateSet start = {.words = start_words, .word_count = builder.word_count};
	StateSet next  = {.words = next_words,  .word_count = builder.word_count};

	stateset_add(&start, nfa->start);
	nfa_epsilon_closure(nfa, &start, stack);

	StateId start_state;

	if (builder_intern_subset(&builder, &start, &start_state) != DFA_OK) {
		builder_free(&builder);
		free(stack);
		free(next_words);
		free(start_words);

		return DFA_ERR;
	}

	for (StateId state = 0; state < builder.state_count; ++state) {
		for (size_t sym_index = 0; sym_index < builder.alphabet_count; ++sym_index) {
			StateSet current = {
				.words      = builder_subset(&builder, state),
				.word_count = builder.word_count,
			};

			nfa_move(nfa, &current, builder.alphabet[sym_index], &next);
			nfa_epsilon_closure(nfa, &next, stack);

			StateId target;

			if (builder_intern_subset(&builder, &next, &target) != DFA_OK) {
				builder_free(&builder);
				free(stack);
				free(next_words);
				free(start_words);

				return DFA_ERR;
			}

			builder_set_transition(&builder, state, sym_index, target);
		}
	}

	unsigned char *alphabet = NULL;

	if (builder.alphabet_count != 0) {
		alphabet = malloc(builder.alphabet_count * sizeof *alphabet);

		if (alphabet == NULL) {
			builder_free(&builder);
			free(stack);
			free(next_words);
			free(start_words);

			return DFA_ERR;
		}

		memcpy(alphabet, builder.alphabet, builder.alphabet_count * sizeof *alphabet);
	}

	free(builder.subsets);
	builder.subsets = NULL;

	*dfa = (Dfa){
		.state_count    = builder.state_count,
		.start          = start_state,
		.alphabet       = alphabet,
		.alphabet_count = builder.alphabet_count,
		.accepting      = builder.accepting,
		.transitions    = builder.transitions,
	};

	builder.accepting   = NULL;
	builder.transitions = NULL;

	free(stack);
	free(next_words);
	free(start_words);

	return DFA_OK;
}

void dfa_free(Dfa *dfa) {
	assert(dfa != NULL);

	free(dfa->alphabet);
	free(dfa->accepting);
	free(dfa->transitions);

	*dfa = (Dfa){0};
}
