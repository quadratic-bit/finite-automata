#include "dfa_product.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
	ALPHABET_SIZE = 256,
};

typedef struct {
	StateId left;
	StateId right;
} StatePair;

static size_t product_alphabet(
	const Dfa *left,
	const Dfa *right,
	unsigned char alphabet[ALPHABET_SIZE]
) {
	unsigned char seen[ALPHABET_SIZE] = {0};

	for (size_t i = 0; i < left->alphabet_count; ++i) {
		seen[left->alphabet[i]] = 1;
	}

	for (size_t i = 0; i < right->alphabet_count; ++i) {
		seen[right->alphabet[i]] = 1;
	}

	size_t count = 0;

	for (size_t symbol = 0; symbol < ALPHABET_SIZE; ++symbol) {
		if (!seen[symbol]) {
			continue;
		}

		alphabet[count++] = (unsigned char)symbol;
	}

	return count;
}

static void alphabet_index(const Dfa *dfa, size_t index[ALPHABET_SIZE]) {
	for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
		index[i] = SIZE_MAX;
	}

	for (size_t i = 0; i < dfa->alphabet_count; ++i) {
		index[dfa->alphabet[i]] = i;
	}
}

static StateId product_step(
	const Dfa *dfa,
	const size_t index[ALPHABET_SIZE],
	StateId state,
	unsigned char symbol
) {
	if (state == dfa->state_count) {
		return state;
	}

	size_t sym_index = index[symbol];

	if (sym_index == SIZE_MAX) {
		return dfa->state_count;
	}

	StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];
	assert(target < dfa->state_count);

	return target;
}

static int product_state_accepting(const Dfa *dfa, StateId state) {
	if (state == dfa->state_count) {
		return 0;
	}

	return dfa->accepting[state] != 0;
}

static size_t pair_index(StatePair pair, size_t right_state_count) {
	return pair.left * right_state_count + pair.right;
}

DfaResult dfa_product(Dfa *dfa, const Dfa *left, const Dfa *right, DfaProductAccept accept) {
	assert(dfa    != NULL);
	assert(left   != NULL);
	assert(right  != NULL);
	assert(accept != NULL);

	assert(dfa != left);
	assert(dfa != right);

	assert(left ->state_count > 0);
	assert(right->state_count > 0);
	assert(left ->start < left ->state_count);
	assert(right->start < right->state_count);

	assert(dfa->state_count    == 0);
	assert(dfa->alphabet       == NULL);
	assert(dfa->alphabet_count == 0);
	assert(dfa->accepting      == NULL);
	assert(dfa->transitions    == NULL);

	if (left->state_count == SIZE_MAX || right->state_count == SIZE_MAX) {
		return DFA_ERR;
	}

	size_t left_count  = left ->state_count + 1;
	size_t right_count = right->state_count + 1;

	if (left_count > SIZE_MAX / right_count) {
		return DFA_ERR;
	}

	size_t pair_count = left_count * right_count;

	unsigned char alphabet_buf[ALPHABET_SIZE];
	size_t alphabet_count = product_alphabet(left, right, alphabet_buf);

	if (alphabet_count != 0 && pair_count > SIZE_MAX / alphabet_count) {
		return DFA_ERR;
	}

	size_t transition_cap = pair_count * alphabet_count;

	if (pair_count > SIZE_MAX / sizeof(StatePair)) {
		return DFA_ERR;
	}

	if (pair_count > SIZE_MAX / sizeof(StateId)) {
		return DFA_ERR;
	}

	if (transition_cap != 0 && transition_cap > SIZE_MAX / sizeof(StateId)) {
		return DFA_ERR;
	}

	StatePair *pairs = malloc(pair_count * sizeof *pairs);
	StateId   *ids   = malloc(pair_count * sizeof *ids);
	unsigned char *accepting = malloc(pair_count * sizeof *accepting);

	StateId *transitions    = NULL;
	unsigned char *alphabet = NULL;

	if (pairs == NULL || ids == NULL || accepting == NULL) {
		goto fail;
	}

	if (transition_cap != 0) {
		transitions = malloc(transition_cap * sizeof *transitions);

		if (transitions == NULL) {
			goto fail;
		}
	}

	if (alphabet_count != 0) {
		alphabet = malloc(alphabet_count * sizeof *alphabet);

		if (alphabet == NULL) {
			goto fail;
		}

		for (size_t i = 0; i < alphabet_count; ++i) {
			alphabet[i] = alphabet_buf[i];
		}
	}

	for (size_t i = 0; i < pair_count; ++i) {
		ids[i] = SIZE_MAX;
	}

	size_t left_index [ALPHABET_SIZE];
	size_t right_index[ALPHABET_SIZE];

	alphabet_index(left,  left_index);
	alphabet_index(right, right_index);

	StatePair start = {
		.left  = left ->start,
		.right = right->start,
	};

	pairs[0] = start;
	ids[pair_index(start, right_count)] = 0;

	accepting[0] = (unsigned char)accept(
		product_state_accepting(left,  start.left),
		product_state_accepting(right, start.right)
	);

	size_t state_count = 1;

	for (StateId state = 0; state < state_count; ++state) {
		StatePair current = pairs[state];

		for (size_t sym_index = 0; sym_index < alphabet_count; ++sym_index) {
			unsigned char symbol = alphabet[sym_index];

			StatePair next = {
				.left  = product_step(left,  left_index,  current.left,  symbol),
				.right = product_step(right, right_index, current.right, symbol),
			};

			size_t index = pair_index(next, right_count);
			StateId target = ids[index];

			if (target == SIZE_MAX) {
				assert(state_count < pair_count);

				target = state_count;
				state_count++;

				pairs[target] = next;
				ids[index] = target;

				accepting[target] = (unsigned char)accept(
					product_state_accepting(left,  next.left),
					product_state_accepting(right, next.right)
				);
			}

			transitions[state * alphabet_count + sym_index] = target;
		}
	}

	free(ids);
	free(pairs);

	*dfa = (Dfa){
		.state_count    = state_count,
		.start          = 0,
		.alphabet       = alphabet,
		.alphabet_count = alphabet_count,
		.accepting      = accepting,
		.transitions    = transitions,
	};

	return DFA_OK;

fail:
	free(alphabet);
	free(transitions);
	free(accepting);
	free(ids);
	free(pairs);

	return DFA_ERR;
}
