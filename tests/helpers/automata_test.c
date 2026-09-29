#include "automata_test.h"

#include <automata/regex.h>

#include <criterion/criterion.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void epsilon_closure(const Nfa *nfa, unsigned char *states, StateId *stack) {
	size_t stack_len = 0;

	for (StateId state = 0; state < nfa->state_count; ++state) {
		if (states[state]) {
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

			if (states[transition->to]) {
				continue;
			}

			states[transition->to] = 1;

			stack[stack_len] = transition->to;
			stack_len++;
		}
	}
}

Nfa test_build_nfa(const char *source) {
	Regex      regex = {0};
	RegexError error = {0};

	cr_assert_eq(
		regex_parse(&regex, source, &error),
		REGEX_OK,
		"failed to parse \"%s\" at %zu: %s",
		source,
		error.position,
		error.message != NULL ? error.message : "<no error>"
	);

	Nfa nfa = {0};

	cr_assert_eq(
		nfa_from_regex(&nfa, &regex),
		NFA_OK,
		"failed to construct NFA from \"%s\"",
		source
	);

	regex_free(&regex);

	return nfa;
}

Dfa test_build_dfa(const char *source) {
	Nfa nfa = test_build_nfa(source);
	Dfa dfa = {0};

	cr_assert_eq(dfa_from_nfa(&dfa, &nfa), DFA_OK, "failed to determinize \"%s\"", source);

	nfa_free(&nfa);

	return dfa;
}

Dfa test_build_minimized_dfa(const char *source) {
	Dfa source_dfa = test_build_dfa(source);
	Dfa minimized  = {0};

	cr_assert_eq(
		dfa_minimize(&minimized, &source_dfa),
		DFA_OK,
		"failed to minimize \"%s\"",
		source
	);

	dfa_free(&source_dfa);

	return minimized;
}

int test_nfa_accepts(const Nfa *nfa, const char *word) {
	unsigned char *current = calloc(nfa->state_count, sizeof *current);
	unsigned char *next    = calloc(nfa->state_count, sizeof *next);

	StateId *stack = malloc(nfa->state_count * sizeof *stack);

	cr_assert_not_null(current);
	cr_assert_not_null(next);
	cr_assert_not_null(stack);

	current[nfa->start] = 1;
	epsilon_closure(nfa, current, stack);

	for (size_t i = 0; word[i] != '\0'; ++i) {
		memset(next, 0, nfa->state_count * sizeof *next);

		for (size_t j = 0; j < nfa->transition_count; ++j) {
			const NfaTransition *transition = &nfa->transitions[j];

			if (transition->kind != NFA_TRANSITION_SYMBOL) {
				continue;
			}

			if (!current[transition->from]) {
				continue;
			}

			if (transition->symbol != (unsigned char)word[i]) {
				continue;
			}

			next[transition->to] = 1;
		}

		epsilon_closure(nfa, next, stack);

		unsigned char *tmp = current;
		current = next;
		next    = tmp;
	}

	int accepting = current[nfa->accept] != 0;

	free(stack);
	free(next);
	free(current);

	return accepting;
}

int test_dfa_accepts(const Dfa *dfa, const char *word) {
	StateId state = dfa->start;

	for (size_t i = 0; word[i] != '\0'; ++i) {
		size_t symbol_index = dfa->alphabet_count;

		for (size_t j = 0; j < dfa->alphabet_count; ++j) {
			if (dfa->alphabet[j] == (unsigned char)word[i]) {
				symbol_index = j;
				break;
			}
		}

		if (symbol_index == dfa->alphabet_count) {
			return 0;
		}

		state = dfa->transitions[state * dfa->alphabet_count + symbol_index];
	}

	return dfa->accepting[state] != 0;
}

void test_assert_nfa_accepts(const Nfa *nfa, const char *word) {
	cr_assert(test_nfa_accepts(nfa, word), "expected NFA to accept \"%s\"", word);
}

void test_assert_nfa_rejects(const Nfa *nfa, const char *word) {
	cr_assert(!test_nfa_accepts(nfa, word), "expected NFA to reject \"%s\"", word);
}

void test_assert_dfa_accepts(const Dfa *dfa, const char *word) {
	cr_assert(test_dfa_accepts(dfa, word), "expected DFA to accept \"%s\"", word);
}

void test_assert_dfa_rejects(const Dfa *dfa, const char *word) {
	cr_assert(!test_dfa_accepts(dfa, word), "expected DFA to reject \"%s\"", word);
}
