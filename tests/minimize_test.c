#include <automata/dfa.h>

#include "helpers/automata_test.h"

#include <criterion/criterion.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void assert_equivalent_words_rec(
	const Dfa *left,
	const Dfa *right,
	char      *word,
	size_t     pos,
	size_t     len
) {
	if (pos == len) {
		word[len] = '\0';

		cr_assert_eq(
			test_dfa_accepts(left, word),
			test_dfa_accepts(right, word),
			"DFAs disagree on \"%s\"",
			word
		);

		return;
	}

	for (size_t i = 0; i < left->alphabet_count; ++i) {
		word[pos] = (char)left->alphabet[i];

		assert_equivalent_words_rec(left, right, word, pos + 1, len);
	}
}

static void assert_equivalent_words(const Dfa *left, const Dfa *right, size_t max_len) {
	cr_assert_eq(left->alphabet_count, right->alphabet_count);

	for (size_t i = 0; i < left->alphabet_count; ++i) {
		cr_assert_eq(left->alphabet[i], right->alphabet[i]);
	}

	char *word = malloc(max_len + 1);
	cr_assert_not_null(word);

	for (size_t len = 0; len <= max_len; ++len) {
		assert_equivalent_words_rec(left, right, word, 0, len);
	}

	free(word);
}

static void assert_minimal(const Dfa *dfa) {
	size_t n = dfa->state_count;

	cr_assert_gt(n, 0);

	cr_assert(n <= SIZE_MAX / n, "state pair table size overflow");

	unsigned char *distinct = calloc(n * n, sizeof *distinct);

	cr_assert_not_null(distinct);

	for (StateId left = 0; left < n; ++left) {
		for (StateId right = left + 1; right < n; ++right) {
			if ((dfa->accepting[left] != 0) != (dfa->accepting[right] != 0)) {
				distinct[left  * n + right] = 1;
				distinct[right * n + left ] = 1;
			}
		}
	}

	int changed;

	do {
		changed = 0;

		for (StateId left = 0; left < n; ++left) {
			for (StateId right = left + 1; right < n; ++right) {
				if (distinct[left * n + right]) {
					continue;
				}

				for (size_t sym = 0; sym < dfa->alphabet_count; ++sym) {
					StateId left_target = dfa->transitions[
						left * dfa->alphabet_count + sym
					];

					StateId right_target = dfa->transitions[
						right * dfa->alphabet_count + sym
					];

					if (distinct[left_target * n + right_target]) {
						distinct[left  * n + right] = 1;
						distinct[right * n + left ] = 1;

						changed = 1;
						break;
					}
				}
			}
		}
	} while (changed);

	for (StateId left = 0; left < n; ++left) {
		for (StateId right = left + 1; right < n; ++right) {
			cr_assert(
				distinct[left * n + right],
				"states %zu and %zu are equivalent",
				left,
				right
			);
		}
	}

	free(distinct);
}

static void assert_minimization(const char *regex, size_t max_word_len) {
	Dfa source    = test_build_dfa(regex);
	Dfa minimized = {0};

	cr_assert_eq(dfa_minimize(&minimized, &source), DFA_OK, "failed to minimize \"%s\"", regex);

	cr_assert_leq(minimized.state_count, source.state_count);

	assert_equivalent_words(&source, &minimized, max_word_len);

	assert_minimal(&minimized);

	dfa_free(&minimized);
	dfa_free(&source);
}

Test(minimize, empty_language) {
	Dfa dfa = test_build_minimized_dfa("0");

	cr_assert_eq(dfa.state_count, 1);
	cr_assert(!dfa.accepting[dfa.start]);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, epsilon) {
	Dfa dfa = test_build_minimized_dfa("1");

	cr_assert_eq(dfa.state_count, 1);
	cr_assert(dfa.accepting[dfa.start]);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, literal) {
	Dfa dfa = test_build_minimized_dfa("a");

	cr_assert_eq(dfa.state_count, 3);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, kleene_star) {
	Dfa dfa = test_build_minimized_dfa("a*");

	cr_assert_eq(dfa.state_count, 1);
	cr_assert(dfa.accepting[dfa.start]);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, simple_alternation) {
	Dfa dfa = test_build_minimized_dfa("a+b");

	cr_assert_eq(dfa.state_count, 3);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, concatenation) {
	Dfa dfa = test_build_minimized_dfa("ab");

	cr_assert_eq(dfa.state_count, 4);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, suffix_language) {
	Dfa dfa = test_build_minimized_dfa("(a+b)*abb");

	cr_assert_eq(dfa.state_count, 4);

	assert_minimal(&dfa);

	dfa_free(&dfa);
}

Test(minimize, preserves_empty_language) {
	assert_minimization("0", 4);
}

Test(minimize, preserves_epsilon) {
	assert_minimization("1", 4);
}

Test(minimize, preserves_literal) {
	assert_minimization("a", 5);
}

Test(minimize, preserves_alternation) {
	assert_minimization("a+b", 5);
}

Test(minimize, preserves_concatenation) {
	assert_minimization("ab", 5);
}

Test(minimize, preserves_star) {
	assert_minimization("a*", 5);
}

Test(minimize, preserves_composed_language) {
	assert_minimization("(a+b)*abb", 6);
}

Test(minimize, preserves_epsilon_interactions) {
	assert_minimization("(1+a)b*", 6);
}

Test(minimize, preserves_empty_interactions) {
	assert_minimization("(0+a)*b", 6);
}
