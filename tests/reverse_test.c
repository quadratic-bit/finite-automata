#include <automata/reverse.h>

#include "helpers/automata_test.h"

#include <criterion/criterion.h>

Test(reverse, nfa_literal) {
	Nfa source   = test_build_nfa("ab");
	Nfa reversed = {0};

	cr_assert_eq(nfa_reverse(&reversed, &source), NFA_OK);

	test_assert_nfa_accepts(&reversed, "ba");

	test_assert_nfa_rejects(&reversed, "");
	test_assert_nfa_rejects(&reversed, "ab");
	test_assert_nfa_rejects(&reversed, "b");

	nfa_free(&reversed);
	nfa_free(&source);
}

Test(reverse, nfa_alternation) {
	Nfa source   = test_build_nfa("ab+cd");
	Nfa reversed = {0};

	cr_assert_eq(nfa_reverse(&reversed, &source), NFA_OK);

	test_assert_nfa_accepts(&reversed, "ba");
	test_assert_nfa_accepts(&reversed, "dc");

	test_assert_nfa_rejects(&reversed, "ab");
	test_assert_nfa_rejects(&reversed, "cd");

	nfa_free(&reversed);
	nfa_free(&source);
}

Test(reverse, nfa_epsilon) {
	Nfa source   = test_build_nfa("1");
	Nfa reversed = {0};

	cr_assert_eq(nfa_reverse(&reversed, &source), NFA_OK);

	test_assert_nfa_accepts(&reversed, "");
	test_assert_nfa_rejects(&reversed, "a");

	nfa_free(&reversed);
	nfa_free(&source);
}

Test(reverse, nfa_twice_preserves_language) {
	Nfa source = test_build_nfa("(a+b)*abb");
	Nfa first  = {0};
	Nfa second = {0};

	cr_assert_eq(nfa_reverse(&first, &source), NFA_OK);
	cr_assert_eq(nfa_reverse(&second, &first), NFA_OK);

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
			test_nfa_accepts(&source, words[i]),
			test_nfa_accepts(&second, words[i]),
			"double reversal changed \"%s\"",
			words[i]
		);
	}

	nfa_free(&second);
	nfa_free(&first);
	nfa_free(&source);
}
