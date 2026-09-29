#include <automata/reverse.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

NfaResult nfa_reverse_dfa(Nfa *nfa, const Dfa *dfa) {
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

	size_t transition_index = 0;

	for (StateId state = 0; state < dfa->state_count; ++state) {
		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];

			assert(target < dfa->state_count);

			transitions[transition_index] = (NfaTransition){
				.from   = target,
				.to     = state,
				.kind   = NFA_TRANSITION_SYMBOL,
				.symbol = dfa->alphabet[sym_index],
			};

			transition_index++;
		}
	}

	StateId start = dfa->state_count;

	for (StateId state = 0; state < dfa->state_count; ++state) {
		if (!dfa->accepting[state]) {
			continue;
		}

		transitions[transition_index] = (NfaTransition){
			.from = start,
			.to   = state,
			.kind = NFA_TRANSITION_EPSILON,
		};

		transition_index++;
	}

	assert(transition_index == transition_count);

	*nfa = (Nfa){
		.state_count      = dfa->state_count + 1,
		.start            = start,
		.accept           = dfa->start,
		.transitions      = transitions,
		.transition_count = transition_count,
	};

	return NFA_OK;
}

NfaResult nfa_reverse(Nfa *nfa, const Nfa *source) {
	assert(nfa    != NULL);
	assert(source != NULL);
	assert(nfa    != source);

	assert(source->state_count > 0);
	assert(source->start  < source->state_count);
	assert(source->accept < source->state_count);

	assert(nfa->state_count      == 0);
	assert(nfa->transitions      == NULL);
	assert(nfa->transition_count == 0);

	NfaTransition *transitions = NULL;

	if (source->transition_count != 0) {
		if (source->transition_count > SIZE_MAX / sizeof *transitions) {
			return NFA_ERR;
		}

		transitions = malloc(source->transition_count * sizeof *transitions);

		if (transitions == NULL) {
			return NFA_ERR;
		}
	}

	for (size_t i = 0; i < source->transition_count; ++i) {
		const NfaTransition *old = &source->transitions[i];

		transitions[i] = (NfaTransition){
			.from   = old->to,
			.to     = old->from,
			.kind   = old->kind,
			.symbol = old->symbol,
		};
	}

	*nfa = (Nfa){
		.state_count      = source->state_count,
		.start            = source->accept,
		.accept           = source->start,
		.transitions      = transitions,
		.transition_count = source->transition_count,
	};

	return NFA_OK;
}
