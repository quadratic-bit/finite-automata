#include <automata/nfa.h>
#include <automata/regex.h>

#include "helpers/automata_test.h"

#include <criterion/criterion.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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

Test(nfa, empty_language_shape) {
	Nfa nfa = test_build_nfa("0");

	cr_assert_eq(nfa.state_count, 2);
	cr_assert_eq(nfa.transition_count, 0);

	cr_assert_lt(nfa.start, nfa.state_count);
	cr_assert_lt(nfa.accept, nfa.state_count);
	cr_assert_neq(nfa.start, nfa.accept);

	nfa_free(&nfa);
}

Test(nfa, epsilon_shape) {
	Nfa nfa = test_build_nfa("1");

	cr_assert_eq(nfa.state_count, 2);
	cr_assert_eq(nfa.transition_count, 1);

	cr_assert(has_transition(&nfa, nfa.start, nfa.accept, NFA_TRANSITION_EPSILON, 0));

	nfa_free(&nfa);
}

Test(nfa, literal_shape) {
	Nfa nfa = test_build_nfa("a");

	cr_assert_eq(nfa.state_count, 2);
	cr_assert_eq(nfa.transition_count, 1);

	cr_assert(has_transition(&nfa, nfa.start, nfa.accept, NFA_TRANSITION_SYMBOL, 'a'));

	nfa_free(&nfa);
}

Test(nfa, empty_language_semantics) {
	Nfa nfa = test_build_nfa("0");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "a");
	test_assert_nfa_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, epsilon_semantics) {
	Nfa nfa = test_build_nfa("1");

	test_assert_nfa_accepts(&nfa, "");

	test_assert_nfa_rejects(&nfa, "a");
	test_assert_nfa_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, literal_semantics) {
	Nfa nfa = test_build_nfa("a");

	test_assert_nfa_accepts(&nfa, "a");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "b");
	test_assert_nfa_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, concatenation) {
	Nfa nfa = test_build_nfa("ab");

	test_assert_nfa_accepts(&nfa, "ab");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "a");
	test_assert_nfa_rejects(&nfa, "b");
	test_assert_nfa_rejects(&nfa, "aba");

	nfa_free(&nfa);
}

Test(nfa, alternation) {
	Nfa nfa = test_build_nfa("a+b");

	test_assert_nfa_accepts(&nfa, "a");
	test_assert_nfa_accepts(&nfa, "b");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "ab");
	test_assert_nfa_rejects(&nfa, "c");

	nfa_free(&nfa);
}

Test(nfa, kleene_star) {
	Nfa nfa = test_build_nfa("a*");

	test_assert_nfa_accepts(&nfa, "");
	test_assert_nfa_accepts(&nfa, "a");
	test_assert_nfa_accepts(&nfa, "aa");
	test_assert_nfa_accepts(&nfa, "aaaaa");

	test_assert_nfa_rejects(&nfa, "b");
	test_assert_nfa_rejects(&nfa, "ab");

	nfa_free(&nfa);
}

Test(nfa, empty_star_is_epsilon) {
	Nfa nfa = test_build_nfa("0*");

	test_assert_nfa_accepts(&nfa, "");

	test_assert_nfa_rejects(&nfa, "a");
	test_assert_nfa_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, epsilon_is_concat_identity) {
	Nfa nfa = test_build_nfa("1a");

	test_assert_nfa_accepts(&nfa, "a");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "aa");

	nfa_free(&nfa);
}

Test(nfa, empty_is_alt_identity) {
	Nfa nfa = test_build_nfa("0+a");

	test_assert_nfa_accepts(&nfa, "a");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "b");

	nfa_free(&nfa);
}

Test(nfa, composed_expression) {
	Nfa nfa = test_build_nfa("(a+b)*abb");

	test_assert_nfa_accepts(&nfa, "abb");
	test_assert_nfa_accepts(&nfa, "aabb");
	test_assert_nfa_accepts(&nfa, "babb");
	test_assert_nfa_accepts(&nfa, "abababb");

	test_assert_nfa_rejects(&nfa, "");
	test_assert_nfa_rejects(&nfa, "ab");
	test_assert_nfa_rejects(&nfa, "aba");
	test_assert_nfa_rejects(&nfa, "abba");

	nfa_free(&nfa);
}
