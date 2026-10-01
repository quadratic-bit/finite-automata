#include <automata/query.h>

#include "helpers/automata_test.h"

#include <criterion/criterion.h>

#include <string.h>

static int query_nfa_accepts(const Nfa *nfa, const char *word) {
	int accepts = 0;

	cr_assert_eq(
		nfa_accepts(nfa, (const unsigned char *)word, strlen(word), &accepts),
		QUERY_OK
	);

	return accepts;
}

static int query_dfa_accepts(const Dfa *dfa, const char *word) {
	int accepts = 0;

	cr_assert_eq(
		dfa_accepts(dfa, (const unsigned char *)word, strlen(word), &accepts),
		QUERY_OK
	);

	return accepts;
}

Test(query, nfa_accepts_word) {
	Nfa nfa = test_build_nfa("(a+b)*abb");

	cr_assert(query_nfa_accepts(&nfa, "abb"));
	cr_assert(query_nfa_accepts(&nfa, "aabb"));
	cr_assert(query_nfa_accepts(&nfa, "babb"));

	cr_assert(!query_nfa_accepts(&nfa, ""));
	cr_assert(!query_nfa_accepts(&nfa, "ab"));
	cr_assert(!query_nfa_accepts(&nfa, "abba"));

	nfa_free(&nfa);
}

Test(query, dfa_accepts_word) {
	Dfa dfa = test_build_dfa("(a+b)*abb");

	cr_assert(query_dfa_accepts(&dfa, "abb"));
	cr_assert(query_dfa_accepts(&dfa, "aabb"));

	cr_assert(!query_dfa_accepts(&dfa, ""));
	cr_assert(!query_dfa_accepts(&dfa, "aba"));

	dfa_free(&dfa);
}

Test(query, accepts_empty_word) {
	Nfa nfa = test_build_nfa("1");
	Dfa dfa = test_build_dfa("1");

	cr_assert(query_nfa_accepts(&nfa, ""));
	cr_assert(query_dfa_accepts(&dfa, ""));

	dfa_free(&dfa);
	nfa_free(&nfa);
}

Test(query, nfa_empty_language) {
	Nfa nfa = test_build_nfa("0");

	int empty;

	cr_assert_eq(nfa_is_empty(&nfa, &empty), QUERY_OK);

	cr_assert(empty);

	nfa_free(&nfa);
}

Test(query, nfa_nonempty_language) {
	Nfa nfa = test_build_nfa("(0+a)b");

	int empty;

	cr_assert_eq(nfa_is_empty(&nfa, &empty), QUERY_OK);

	cr_assert(!empty);

	nfa_free(&nfa);
}

Test(query, dfa_empty_language) {
	Dfa dfa = test_build_dfa("0");

	int empty;

	cr_assert_eq(dfa_is_empty(&dfa, &empty), QUERY_OK);

	cr_assert(empty);

	dfa_free(&dfa);
}

Test(query, dfa_nonempty_language) {
	Dfa dfa = test_build_dfa("a*");

	int empty;

	cr_assert_eq(dfa_is_empty(&dfa, &empty), QUERY_OK);

	cr_assert(!empty);

	dfa_free(&dfa);
}

Test(query, equivalent_languages) {
	Dfa left  = test_build_dfa("a+b");
	Dfa right = test_build_dfa("b+a");

	int equivalent;

	cr_assert_eq(dfa_equivalent(&left, &right, &equivalent), QUERY_OK);

	cr_assert(equivalent);

	dfa_free(&right);
	dfa_free(&left);
}

Test(query, different_languages) {
	Dfa left  = test_build_dfa("a*");
	Dfa right = test_build_dfa("(aa)*");

	int equivalent;

	cr_assert_eq(dfa_equivalent(&left, &right, &equivalent), QUERY_OK);

	cr_assert(!equivalent);

	dfa_free(&right);
	dfa_free(&left);
}

Test(query, subset) {
	Dfa left  = test_build_dfa("a");
	Dfa right = test_build_dfa("a+b");

	int subset;

	cr_assert_eq(dfa_is_subset(&left, &right, &subset), QUERY_OK);

	cr_assert(subset);

	dfa_free(&right);
	dfa_free(&left);
}

Test(query, not_subset) {
	Dfa left  = test_build_dfa("a+b");
	Dfa right = test_build_dfa("a");

	int subset;

	cr_assert_eq(dfa_is_subset(&left, &right, &subset), QUERY_OK);

	cr_assert(!subset);

	dfa_free(&right);
	dfa_free(&left);
}

Test(query, equivalence_handles_different_alphabets) {
	Dfa left  = test_build_dfa("a");
	Dfa right = test_build_dfa("a+0b");

	int equivalent;

	cr_assert_eq(dfa_equivalent(&left, &right, &equivalent), QUERY_OK);

	cr_assert(equivalent);

	dfa_free(&right);
	dfa_free(&left);
}
