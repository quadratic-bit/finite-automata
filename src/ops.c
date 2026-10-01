#include <automata/ops.h>

#include "dfa_product.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

NfaResult nfa_from_dfa(Nfa *nfa, const Dfa *dfa) {
	assert(nfa != NULL);
	assert(dfa != NULL);

	assert(dfa->state_count > 0);
	assert(dfa->start < dfa->state_count);

	assert(nfa->state_count      == 0);
	assert(nfa->transitions      == NULL);
	assert(nfa->transition_count == 0);

	if (dfa->state_count == SIZE_MAX) {
		return NFA_ERR;
	}

	size_t epsilon_count = 0;

	for (StateId state = 0; state < dfa->state_count; ++state) {
		if (dfa->accepting[state]) {
			epsilon_count++;
		}
	}

	if (dfa->alphabet_count != 0 && dfa->state_count > SIZE_MAX / dfa->alphabet_count) {
		return NFA_ERR;
	}

	size_t symbol_count = dfa->state_count * dfa->alphabet_count;
	if (symbol_count > SIZE_MAX - epsilon_count) {
		return NFA_ERR;
	}

	size_t transition_count = symbol_count + epsilon_count;
	if (transition_count != 0 && transition_count > SIZE_MAX / sizeof(NfaTransition)) {
		return NFA_ERR;
	}

	NfaTransition *transitions = NULL;

	if (transition_count != 0) {
		transitions = malloc(transition_count * sizeof *transitions);

		if (transitions == NULL) {
			return NFA_ERR;
		}
	}

	size_t index = 0;

	for (StateId state = 0; state < dfa->state_count; ++state) {
		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];

			assert(target < dfa->state_count);

			transitions[index++] = (NfaTransition){
				.from   = state,
				.to     = target,
				.kind   = NFA_TRANSITION_SYMBOL,
				.symbol = dfa->alphabet[sym_index],
			};
		}
	}

	StateId accept = dfa->state_count;

	for (StateId state = 0; state < dfa->state_count; ++state) {
		if (!dfa->accepting[state]) {
			continue;
		}

		transitions[index++] = (NfaTransition){
			.from = state,
			.to   = accept,
			.kind = NFA_TRANSITION_EPSILON,
		};
	}

	assert(index == transition_count);

	*nfa = (Nfa){
		.state_count      = dfa->state_count + 1,
		.start            = dfa->start,
		.accept           = accept,
		.transitions      = transitions,
		.transition_count = transition_count,
	};

	return NFA_OK;
}

NfaResult nfa_union(Nfa *nfa, const Nfa *left, const Nfa *right) {
	assert(nfa   != NULL);
	assert(left  != NULL);
	assert(right != NULL);

	assert(nfa != left);
	assert(nfa != right);

	assert(left ->state_count > 0);
	assert(right->state_count > 0);

	if (left->state_count > SIZE_MAX - right->state_count) {
		return NFA_ERR;
	}

	size_t inner_state_count = left->state_count + right->state_count;
	if (inner_state_count > SIZE_MAX - 2) {
		return NFA_ERR;
	}

	if (left->transition_count > SIZE_MAX - right->transition_count) {
		return NFA_ERR;
	}

	size_t transition_count = left->transition_count + right->transition_count;
	if (transition_count > SIZE_MAX - 4) {
		return NFA_ERR;
	}

	transition_count += 4;
	if (transition_count > SIZE_MAX / sizeof(NfaTransition)) {
		return NFA_ERR;
	}

	NfaTransition *transitions = malloc(transition_count * sizeof *transitions);
	if (transitions == NULL) {
		return NFA_ERR;
	}

	StateId right_offset = left->state_count;
	StateId start        = inner_state_count;
	StateId accept       = start + 1;

	size_t index = 0;

	for (size_t i = 0; i < left->transition_count; ++i) {
		const NfaTransition *old = &left->transitions[i];

		assert(old->from < left->state_count);
		assert(old->to   < left->state_count);

		transitions[index++] = *old;
	}

	for (size_t i = 0; i < right->transition_count; ++i) {
		const NfaTransition *old = &right->transitions[i];

		assert(old->from < right->state_count);
		assert(old->to   < right->state_count);

		transitions[index++] = (NfaTransition){
			.from   = old->from + right_offset,
			.to     = old->to + right_offset,
			.kind   = old->kind,
			.symbol = old->symbol,
		};
	}

	transitions[index++] = (NfaTransition){
		.from = start,
		.to   = left->start,
		.kind = NFA_TRANSITION_EPSILON,
	};

	transitions[index++] = (NfaTransition){
		.from = start,
		.to   = right->start + right_offset,
		.kind = NFA_TRANSITION_EPSILON,
	};

	transitions[index++] = (NfaTransition){
		.from = left->accept,
		.to   = accept,
		.kind = NFA_TRANSITION_EPSILON,
	};

	transitions[index++] = (NfaTransition){
		.from = right->accept + right_offset,
		.to   = accept,
		.kind = NFA_TRANSITION_EPSILON,
	};

	assert(index == transition_count);

	*nfa = (Nfa){
		.state_count      = inner_state_count + 2,
		.start            = start,
		.accept           = accept,
		.transitions      = transitions,
		.transition_count = transition_count,
	};

	return NFA_OK;
}

NfaResult nfa_concat(Nfa *nfa, const Nfa *left, const Nfa *right) {
	assert(nfa   != NULL);
	assert(left  != NULL);
	assert(right != NULL);

	assert(nfa != left);
	assert(nfa != right);

	assert(left ->state_count > 0);
	assert(right->state_count > 0);

	if (left->state_count > SIZE_MAX - right->state_count) {
		return NFA_ERR;
	}

	size_t state_count = left->state_count + right->state_count;

	if (left->transition_count > SIZE_MAX - right->transition_count) {
		return NFA_ERR;
	}

	size_t transition_count = left->transition_count + right->transition_count;
	if (transition_count == SIZE_MAX) {
		return NFA_ERR;
	}

	transition_count++;
	if (transition_count > SIZE_MAX / sizeof(NfaTransition)) {
		return NFA_ERR;
	}

	NfaTransition *transitions = malloc(transition_count * sizeof *transitions);
	if (transitions == NULL) {
		return NFA_ERR;
	}

	StateId right_offset = left->state_count;
	size_t index = 0;

	for (size_t i = 0; i < left->transition_count; ++i) {
		const NfaTransition *old = &left->transitions[i];

		assert(old->from < left->state_count);
		assert(old->to   < left->state_count);

		transitions[index++] = *old;
	}

	for (size_t i = 0; i < right->transition_count; ++i) {
		const NfaTransition *old = &right->transitions[i];

		assert(old->from < right->state_count);
		assert(old->to   < right->state_count);

		transitions[index++] = (NfaTransition){
			.from   = old->from + right_offset,
			.to     = old->to + right_offset,
			.kind   = old->kind,
			.symbol = old->symbol,
		};
	}

	transitions[index++] = (NfaTransition){
		.from = left->accept,
		.to   = right->start + right_offset,
		.kind = NFA_TRANSITION_EPSILON,
	};

	assert(index == transition_count);

	*nfa = (Nfa){
		.state_count      = state_count,
		.start            = left->start,
		.accept           = right->accept + right_offset,
		.transitions      = transitions,
		.transition_count = transition_count,
	};

	return NFA_OK;
}

NfaResult nfa_star(Nfa *nfa, const Nfa *source) {
	assert(nfa    != NULL);
	assert(source != NULL);
	assert(nfa    != source);

	assert(source->state_count > 0);

	if (source->state_count > SIZE_MAX - 2) {
		return NFA_ERR;
	}

	if (source->transition_count > SIZE_MAX - 4) {
		return NFA_ERR;
	}

	size_t state_count      = source->state_count + 2;
	size_t transition_count = source->transition_count + 4;

	if (transition_count > SIZE_MAX / sizeof(NfaTransition)) {
		return NFA_ERR;
	}

	NfaTransition *transitions = malloc(transition_count * sizeof *transitions);
	if (transitions == NULL) {
		return NFA_ERR;
	}

	size_t index = 0;

	for (size_t i = 0; i < source->transition_count; ++i) {
		const NfaTransition *old = &source->transitions[i];

		assert(old->from < source->state_count);
		assert(old->to   < source->state_count);

		transitions[index++] = *old;
	}

	StateId start  = source->state_count;
	StateId accept = start + 1;

	transitions[index++] = (NfaTransition){
		.from = start,
		.to   = source->start,
		.kind = NFA_TRANSITION_EPSILON,
	};

	transitions[index++] = (NfaTransition){
		.from = start,
		.to   = accept,
		.kind = NFA_TRANSITION_EPSILON,
	};

	transitions[index++] = (NfaTransition){
		.from = source->accept,
		.to   = source->start,
		.kind = NFA_TRANSITION_EPSILON,
	};

	transitions[index++] = (NfaTransition){
		.from = source->accept,
		.to   = accept,
		.kind = NFA_TRANSITION_EPSILON,
	};

	assert(index == transition_count);

	*nfa = (Nfa){
		.state_count      = state_count,
		.start            = start,
		.accept           = accept,
		.transitions      = transitions,
		.transition_count = transition_count,
	};

	return NFA_OK;
}

static int accept_union(int left, int right) {
	return left || right;
}

static int accept_intersection(int left, int right) {
	return left && right;
}

static int accept_difference(int left, int right) {
	return left && !right;
}

static int accept_xor(int left, int right) {
	return left != right;
}

DfaResult dfa_union(Dfa *dfa, const Dfa *left, const Dfa *right) {
	return dfa_product(dfa, left, right, accept_union);
}

DfaResult dfa_inter(Dfa *dfa, const Dfa *left, const Dfa *right) {
	return dfa_product(dfa, left, right, accept_intersection);
}

DfaResult dfa_diff(Dfa *dfa, const Dfa *left, const Dfa *right) {
	return dfa_product(dfa, left, right, accept_difference);
}

DfaResult dfa_sym_diff(Dfa *dfa, const Dfa *left, const Dfa *right) {
	return dfa_product(dfa, left, right, accept_xor);
}
