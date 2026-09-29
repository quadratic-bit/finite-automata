#include <automata/dfa.h>
#include <automata/nfa.h>

#include "helpers/automata_test.h"

#include <criterion/criterion.h>

#include <stddef.h>

static size_t symbol_index(const Dfa *dfa, unsigned char symbol) {
	for (size_t i = 0; i < dfa->alphabet_count; ++i) {
		if (dfa->alphabet[i] == symbol) {
			return i;
		}
	}

	return dfa->alphabet_count;
}

static void assert_valid_dfa(const Dfa *dfa) {
	cr_assert_gt(dfa->state_count, 0);
	cr_assert_lt(dfa->start, dfa->state_count);

	if (dfa->alphabet_count != 0) {
		cr_assert_not_null(dfa->alphabet);
		cr_assert_not_null(dfa->transitions);
	}

	cr_assert_not_null(dfa->accepting);

	for (StateId state = 0; state < dfa->state_count; ++state) {
		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];

			cr_assert_lt(
				target,
				dfa->state_count,
				"invalid transition from state %zu",
				state
			);
		}
	}
}

Test(dfa, literal) {
	Dfa dfa = test_build_dfa("a");

	assert_valid_dfa(&dfa);

	cr_assert_eq(dfa.alphabet_count, 1);
	cr_assert_eq(dfa.alphabet[0], 'a');

	test_assert_dfa_accepts(&dfa, "a");

	test_assert_dfa_rejects(&dfa, "");
	test_assert_dfa_rejects(&dfa, "aa");
	test_assert_dfa_rejects(&dfa, "b");

	dfa_free(&dfa);
}

Test(dfa, epsilon) {
	Dfa dfa = test_build_dfa("1");

	assert_valid_dfa(&dfa);

	cr_assert_eq(dfa.alphabet_count, 0);

	test_assert_dfa_accepts(&dfa, "");

	test_assert_dfa_rejects(&dfa, "a");

	dfa_free(&dfa);
}

Test(dfa, empty_language) {
	Dfa dfa = test_build_dfa("0");

	assert_valid_dfa(&dfa);

	cr_assert_eq(dfa.alphabet_count, 0);

	test_assert_dfa_rejects(&dfa, "");
	test_assert_dfa_rejects(&dfa, "a");

	dfa_free(&dfa);
}

Test(dfa, alphabet_is_sorted) {
	Dfa dfa = test_build_dfa("z+a+m+b");

	cr_assert_eq(dfa.alphabet_count, 4);

	cr_assert_eq(dfa.alphabet[0], 'a');
	cr_assert_eq(dfa.alphabet[1], 'b');
	cr_assert_eq(dfa.alphabet[2], 'm');
	cr_assert_eq(dfa.alphabet[3], 'z');

	dfa_free(&dfa);
}

Test(dfa, is_complete) {
	Dfa dfa = test_build_dfa("ab");

	assert_valid_dfa(&dfa);

	cr_assert_eq(dfa.alphabet_count, 2);

	for (StateId state = 0; state < dfa.state_count; ++state) {
		for (size_t sym_index = 0; sym_index < dfa.alphabet_count; ++sym_index) {
			StateId target = dfa.transitions[state * dfa.alphabet_count + sym_index];

			cr_assert_lt(target, dfa.state_count);
		}
	}

	dfa_free(&dfa);
}

Test(dfa, creates_dead_state) {
	Dfa dfa = test_build_dfa("a");

	size_t a = symbol_index(&dfa, 'a');

	cr_assert_neq(a, dfa.alphabet_count);

	StateId after_a = dfa.transitions[dfa.start * dfa.alphabet_count + a];
	StateId dead    = dfa.transitions[after_a   * dfa.alphabet_count + a];

	cr_assert(!dfa.accepting[dead]);

	cr_assert_eq(dfa.transitions[dead * dfa.alphabet_count + a], dead);

	dfa_free(&dfa);
}

Test(dfa, alternation_semantics) {
	Dfa dfa = test_build_dfa("a+b");

	test_assert_dfa_accepts(&dfa, "a");
	test_assert_dfa_accepts(&dfa, "b");

	test_assert_dfa_rejects(&dfa, "");
	test_assert_dfa_rejects(&dfa, "ab");
	test_assert_dfa_rejects(&dfa, "ba");

	dfa_free(&dfa);
}

Test(dfa, concatenation_semantics) {
	Dfa dfa = test_build_dfa("ab");

	test_assert_dfa_accepts(&dfa, "ab");

	test_assert_dfa_rejects(&dfa, "");
	test_assert_dfa_rejects(&dfa, "a");
	test_assert_dfa_rejects(&dfa, "b");
	test_assert_dfa_rejects(&dfa, "aba");

	dfa_free(&dfa);
}

Test(dfa, kleene_star_semantics) {
	Dfa dfa = test_build_dfa("a*");

	test_assert_dfa_accepts(&dfa, "");
	test_assert_dfa_accepts(&dfa, "a");
	test_assert_dfa_accepts(&dfa, "aa");
	test_assert_dfa_accepts(&dfa, "aaaa");

	test_assert_dfa_rejects(&dfa, "b");
	test_assert_dfa_rejects(&dfa, "ab");

	dfa_free(&dfa);
}

Test(dfa, composed_expression) {
	Dfa dfa = test_build_dfa("(a+b)*abb");

	test_assert_dfa_accepts(&dfa, "abb");
	test_assert_dfa_accepts(&dfa, "aabb");
	test_assert_dfa_accepts(&dfa, "babb");
	test_assert_dfa_accepts(&dfa, "abababb");

	test_assert_dfa_rejects(&dfa, "");
	test_assert_dfa_rejects(&dfa, "ab");
	test_assert_dfa_rejects(&dfa, "aba");
	test_assert_dfa_rejects(&dfa, "abba");

	dfa_free(&dfa);
}

Test(dfa, complement) {
	Dfa dfa = test_build_dfa("a");

	dfa_complement(&dfa);

	test_assert_dfa_accepts(&dfa, "");
	test_assert_dfa_rejects(&dfa, "a");
	test_assert_dfa_accepts(&dfa, "aa");
	test_assert_dfa_accepts(&dfa, "aaa");

	dfa_free(&dfa);
}

Test(dfa, complement_twice_is_identity) {
	Dfa dfa = test_build_dfa("(a+b)*abb");

	static const char *words[] = {
		"",
		"a",
		"b",
		"abb",
		"aabb",
		"abba",
		"abababb",
	};

	int before[sizeof words / sizeof words[0]];

	for (size_t i = 0; i < sizeof words / sizeof words[0]; ++i) {
		before[i] = test_dfa_accepts(&dfa, words[i]);
	}

	dfa_complement(&dfa);
	dfa_complement(&dfa);

	for (size_t i = 0; i < sizeof words / sizeof words[0]; ++i) {
		cr_assert_eq(
			test_dfa_accepts(&dfa, words[i]),
			before[i],
			"double complement changed \"%s\"",
			words[i]
		);
	}

	dfa_free(&dfa);
}

Test(dfa, complement_empty_language) {
	Dfa dfa = test_build_dfa("0");

	dfa_complement(&dfa);

	test_assert_dfa_accepts(&dfa, "");

	dfa_free(&dfa);
}
