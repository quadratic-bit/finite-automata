#include <automata/nfa.h>
#include <automata/regex.h>

#include <criterion/criterion.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static Nfa build_nfa(const char *source) {
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

static int has_transition(
	const Nfa *nfa,
	StateId from,
	StateId to,
	NfaTransitionKind kind,
	unsigned char symbol
) {
	for (size_t i = 0; i < nfa->transition_count; ++i) {
		const NfaTransition *transition = &nfa->transitions[i];

		if (transition->from != from) continue;
		if (transition->to   != to)   continue;
		if (transition->kind != kind) continue;

		if (kind == NFA_TRANSITION_SYMBOL && transition->symbol != symbol) {
			continue;
		}

		return 1;
	}

	return 0;
}

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

static int nfa_accepts(const Nfa *nfa, const char *word) {
	unsigned char *current = calloc(nfa->state_count, sizeof *current);

	unsigned char *next  = calloc(nfa->state_count,  sizeof *next);
	StateId       *stack = malloc(nfa->state_count * sizeof *stack);

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
		next = tmp;
	}

	int accepting = current[nfa->accept] != 0;

	free(stack);
	free(next);
	free(current);

	return accepting;
}

static void assert_accepts(const Nfa *nfa, const char *word) {
	cr_assert(nfa_accepts(nfa, word), "expected NFA to accept \"%s\"", word);
}

static void assert_rejects(const Nfa *nfa, const char *word) {
	cr_assert(!nfa_accepts(nfa, word), "expected NFA to reject \"%s\"", word);
}

Test(nfa, empty_language_shape) {
	Nfa nfa = build_nfa("0");

	cr_assert_eq(nfa.state_count, 2);
	cr_assert_eq(nfa.transition_count, 0);

	cr_assert_lt(nfa.start, nfa.state_count);
	cr_assert_lt(nfa.accept, nfa.state_count);
	cr_assert_neq(nfa.start, nfa.accept);

	nfa_free(&nfa);
}

Test(nfa, epsilon_shape) {
	Nfa nfa = build_nfa("1");

	cr_assert_eq(nfa.state_count, 2);
	cr_assert_eq(nfa.transition_count, 1);

	cr_assert(has_transition(&nfa, nfa.start, nfa.accept, NFA_TRANSITION_EPSILON, 0));

	nfa_free(&nfa);
}

Test(nfa, literal_shape) {
	Nfa nfa = build_nfa("a");

	cr_assert_eq(nfa.state_count, 2);
	cr_assert_eq(nfa.transition_count, 1);

	cr_assert(has_transition(&nfa, nfa.start, nfa.accept, NFA_TRANSITION_SYMBOL, 'a'));

	nfa_free(&nfa);
}

Test(nfa, empty_language_semantics) {
	Nfa nfa = build_nfa("0");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "a");
	assert_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, epsilon_semantics) {
	Nfa nfa = build_nfa("1");

	assert_accepts(&nfa, "");

	assert_rejects(&nfa, "a");
	assert_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, literal_semantics) {
	Nfa nfa = build_nfa("a");

	assert_accepts(&nfa, "a");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "b");
	assert_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, concatenation) {
	Nfa nfa = build_nfa("ab");

	assert_accepts(&nfa, "ab");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "a");
	assert_rejects(&nfa, "b");
	assert_rejects(&nfa, "aba");

	nfa_free(&nfa);
}

Test(nfa, alternation) {
	Nfa nfa = build_nfa("a+b");

	assert_accepts(&nfa, "a");
	assert_accepts(&nfa, "b");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "ab");
	assert_rejects(&nfa, "c");

	nfa_free(&nfa);
}

Test(nfa, kleene_star) {
	Nfa nfa = build_nfa("a*");

	assert_accepts(&nfa, "");
	assert_accepts(&nfa, "a");
	assert_accepts(&nfa, "aa");
	assert_accepts(&nfa, "aaaaa");

	assert_rejects(&nfa, "b");
	assert_rejects(&nfa, "ab");

	nfa_free(&nfa);
}

Test(nfa, empty_star_is_epsilon) {
	Nfa nfa = build_nfa("0*");

	assert_accepts(&nfa, "");

	assert_rejects(&nfa, "a");
	assert_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, epsilon_is_concat_identity) {
	Nfa nfa = build_nfa("1a");

	assert_accepts(&nfa, "a");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, empty_is_alt_identity) {
	Nfa nfa = build_nfa("0+a");

	assert_accepts(&nfa, "a");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "b");

	nfa_free(&nfa);
}

Test(nfa, composed_expression) {
	Nfa nfa = build_nfa("(a+b)*abb");

	assert_accepts(&nfa, "abb");
	assert_accepts(&nfa, "aabb");
	assert_accepts(&nfa, "babb");
	assert_accepts(&nfa, "abababb");

	assert_rejects(&nfa, "");
	assert_rejects(&nfa, "ab");
	assert_rejects(&nfa, "aba");
	assert_rejects(&nfa, "abba");

	nfa_free(&nfa);
}
