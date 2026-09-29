#include <automata/dfa.h>
#include <automata/reverse.h>

#include <assert.h>
#include <stddef.h>

static int same_behavior(const Dfa *dfa, StateId left, StateId right) {
	if ((dfa->accepting[left] != 0) != (dfa->accepting[right] != 0)) {
		return 0;
	}

	for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
		StateId left_target  = dfa->transitions[left  * dfa->alphabet_count + sym_index];
		StateId right_target = dfa->transitions[right * dfa->alphabet_count + sym_index];

		if (left_target != right_target) {
			return 0;
		}
	}

	return 1;
}

static void remove_state(Dfa *dfa, StateId removed, StateId replacement) {
	assert(removed     < dfa->state_count);
	assert(replacement < dfa->state_count);

	assert(removed != replacement);

	size_t old_count = dfa->state_count;

	StateId new_start = dfa->start - (StateId)(dfa->start > removed);

	for (StateId old_state = 0, new_state = 0; old_state < old_count; ++old_state) {
		if (old_state == removed) {
			continue;
		}

		dfa->accepting[new_state] = dfa->accepting[old_state];

		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[
				old_state * dfa->alphabet_count + sym_index
			];

			if (target == removed) {
				target = replacement;
			}

			if (target > removed) {
				target--;
			}

			dfa->transitions[new_state * dfa->alphabet_count + sym_index ] = target;
		}

		new_state++;
	}

	dfa->state_count--;
	dfa->start = new_start;
}

static void remove_artificial_start_duplicates(Dfa *dfa) {
	for (;;) {
		StateId duplicate = dfa->state_count;

		for (StateId state = 0; state < dfa->state_count; ++state) {
			if (state == dfa->start) {
				continue;
			}

			if (same_behavior(dfa, dfa->start, state)) {
				duplicate = state;
				break;
			}
		}

		if (duplicate == dfa->state_count) {
			return;
		}

		remove_state(dfa, duplicate, dfa->start);
	}
}

static DfaResult reverse_determinize(Dfa *dfa, const Dfa *source) {
	Nfa reversed = {0};

	if (nfa_reverse_dfa(&reversed, source) != NFA_OK) {
		return DFA_ERR;
	}

	DfaResult result = dfa_from_nfa(dfa, &reversed);

	nfa_free(&reversed);

	if (result != DFA_OK) {
		return DFA_ERR;
	}

	remove_artificial_start_duplicates(dfa);

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

	if (reverse_determinize(&reversed, source) != DFA_OK) {
		return DFA_ERR;
	}

	if (reverse_determinize(dfa, &reversed) != DFA_OK) {
		dfa_free(&reversed);
		return DFA_ERR;
	}

	dfa_free(&reversed);

	return DFA_OK;
}
