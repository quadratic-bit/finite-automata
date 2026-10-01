#include <automata/ops.h>

#include "helpers/automata_test.h"

#include <criterion/criterion.h>

Test(ops, dfa_to_nfa_preserves_language) {
	Dfa dfa = test_build_dfa("(a+b)*abb");
	Nfa nfa = {0};

	cr_assert_eq(nfa_from_dfa(&nfa, &dfa), NFA_OK);

	static const char *words[] = {
		"",
		"a",
		"b",
		"ab",
		"abb",
		"aabb",
		"babb",
		"abba",
		"abababb",
	};

	for (size_t i = 0; i < sizeof words / sizeof words[0]; ++i) {
		cr_assert_eq(
			test_dfa_accepts(&dfa, words[i]),
			test_nfa_accepts(&nfa, words[i]),
			"DFA and converted NFA disagree on \"%s\"",
			words[i]
		);
	}

	nfa_free(&nfa);
	dfa_free(&dfa);
}

Test(ops, nfa_union) {
	Nfa left   = test_build_nfa("ab");
	Nfa right  = test_build_nfa("ba");
	Nfa result = {0};

	cr_assert_eq(nfa_union(&result, &left, &right), NFA_OK);

	test_assert_nfa_accepts(&result, "ab");
	test_assert_nfa_accepts(&result, "ba");

	test_assert_nfa_rejects(&result, "");
	test_assert_nfa_rejects(&result, "a");
	test_assert_nfa_rejects(&result, "b");
	test_assert_nfa_rejects(&result, "aa");
	test_assert_nfa_rejects(&result, "aba");

	nfa_free(&result);
	nfa_free(&right);
	nfa_free(&left);
}

Test(ops, nfa_union_with_empty_language) {
	Nfa left   = test_build_nfa("0");
	Nfa right  = test_build_nfa("a");
	Nfa result = {0};

	cr_assert_eq(nfa_union(&result, &left, &right), NFA_OK);

	test_assert_nfa_accepts(&result, "a");

	test_assert_nfa_rejects(&result, "");
	test_assert_nfa_rejects(&result, "aa");

	nfa_free(&result);
	nfa_free(&right);
	nfa_free(&left);
}

Test(ops, nfa_concat) {
	Nfa left   = test_build_nfa("a+b");
	Nfa right  = test_build_nfa("c");
	Nfa result = {0};

	cr_assert_eq(nfa_concat(&result, &left, &right), NFA_OK);

	test_assert_nfa_accepts(&result, "ac");
	test_assert_nfa_accepts(&result, "bc");

	test_assert_nfa_rejects(&result, "");
	test_assert_nfa_rejects(&result, "a");
	test_assert_nfa_rejects(&result, "b");
	test_assert_nfa_rejects(&result, "c");
	test_assert_nfa_rejects(&result, "abc");

	nfa_free(&result);
	nfa_free(&right);
	nfa_free(&left);
}

Test(ops, nfa_concat_epsilon_identity) {
	Nfa left   = test_build_nfa("1");
	Nfa right  = test_build_nfa("a");
	Nfa result = {0};

	cr_assert_eq(nfa_concat(&result, &left, &right), NFA_OK);

	test_assert_nfa_accepts(&result, "a");

	test_assert_nfa_rejects(&result, "");
	test_assert_nfa_rejects(&result, "aa");

	nfa_free(&result);
	nfa_free(&right);
	nfa_free(&left);
}

Test(ops, nfa_star) {
	Nfa source = test_build_nfa("ab");
	Nfa result = {0};

	cr_assert_eq(nfa_star(&result, &source), NFA_OK);

	test_assert_nfa_accepts(&result, "");
	test_assert_nfa_accepts(&result, "ab");
	test_assert_nfa_accepts(&result, "abab");
	test_assert_nfa_accepts(&result, "ababab");

	test_assert_nfa_rejects(&result, "a");
	test_assert_nfa_rejects(&result, "b");
	test_assert_nfa_rejects(&result, "aba");

	nfa_free(&result);
	nfa_free(&source);
}

Test(ops, nfa_star_empty_language_is_epsilon) {
	Nfa source = test_build_nfa("0");
	Nfa result = {0};

	cr_assert_eq(nfa_star(&result, &source), NFA_OK);

	test_assert_nfa_accepts(&result, "");

	test_assert_nfa_rejects(&result, "a");

	nfa_free(&result);
	nfa_free(&source);
}

Test(ops, dfa_union) {
	Dfa left   = test_build_dfa("a");
	Dfa right  = test_build_dfa("b");
	Dfa result = {0};

	cr_assert_eq(
		dfa_union(&result, &left, &right),
		DFA_OK
	);

	test_assert_dfa_accepts(&result, "a");
	test_assert_dfa_accepts(&result, "b");

	test_assert_dfa_rejects(&result, "");
	test_assert_dfa_rejects(&result, "ab");
	test_assert_dfa_rejects(&result, "aa");
	test_assert_dfa_rejects(&result, "bb");

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}

Test(ops, dfa_intersection) {
	Dfa left   = test_build_dfa("(a+b)*a");
	Dfa right  = test_build_dfa("a*");
	Dfa result = {0};

	cr_assert_eq(dfa_inter(&result, &left, &right), DFA_OK);

	test_assert_dfa_accepts(&result, "a");
	test_assert_dfa_accepts(&result, "aa");
	test_assert_dfa_accepts(&result, "aaa");

	test_assert_dfa_rejects(&result, "");
	test_assert_dfa_rejects(&result, "b");
	test_assert_dfa_rejects(&result, "ba");
	test_assert_dfa_rejects(&result, "ab");

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}

Test(ops, dfa_intersection_disjoint_alphabets_is_empty) {
	Dfa left   = test_build_dfa("a");
	Dfa right  = test_build_dfa("b");
	Dfa result = {0};

	cr_assert_eq(dfa_inter(&result, &left, &right), DFA_OK);

	test_assert_dfa_rejects(&result, "");
	test_assert_dfa_rejects(&result, "a");
	test_assert_dfa_rejects(&result, "b");
	test_assert_dfa_rejects(&result, "ab");

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}

Test(ops, dfa_difference) {
	Dfa left   = test_build_dfa("a+b");
	Dfa right  = test_build_dfa("b");
	Dfa result = {0};

	cr_assert_eq(dfa_diff(&result, &left, &right), DFA_OK);

	test_assert_dfa_accepts(&result, "a");

	test_assert_dfa_rejects(&result, "");
	test_assert_dfa_rejects(&result, "b");
	test_assert_dfa_rejects(&result, "ab");

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}

Test(ops, dfa_difference_is_directional) {
	Dfa left   = test_build_dfa("a");
	Dfa right  = test_build_dfa("a+b");
	Dfa result = {0};

	cr_assert_eq(dfa_diff(&result, &left, &right), DFA_OK);

	test_assert_dfa_rejects(&result, "");
	test_assert_dfa_rejects(&result, "a");
	test_assert_dfa_rejects(&result, "b");

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}

Test(ops, dfa_symmetric_difference) {
	Dfa left   = test_build_dfa("a+b");
	Dfa right  = test_build_dfa("b+c");
	Dfa result = {0};

	cr_assert_eq(dfa_sym_diff(&result, &left, &right), DFA_OK);

	test_assert_dfa_accepts(&result, "a");
	test_assert_dfa_accepts(&result, "c");

	test_assert_dfa_rejects(&result, "");
	test_assert_dfa_rejects(&result, "b");
	test_assert_dfa_rejects(&result, "ac");

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}

Test(ops, dfa_product_uses_union_alphabet) {
	Dfa left   = test_build_dfa("a");
	Dfa right  = test_build_dfa("c+b");
	Dfa result = {0};

	cr_assert_eq(dfa_union(&result, &left, &right), DFA_OK);

	cr_assert_eq(result.alphabet_count, 3);

	cr_assert_eq(result.alphabet[0], 'a');
	cr_assert_eq(result.alphabet[1], 'b');
	cr_assert_eq(result.alphabet[2], 'c');

	dfa_free(&result);
	dfa_free(&right);
	dfa_free(&left);
}
