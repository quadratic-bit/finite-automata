#include "stateset.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

void stateset_clear(StateSet *set) {
	memset(set->words, 0, set->word_count * sizeof *set->words);
}

void stateset_add(StateSet *set, StateId state) {
	size_t word = state / 64;
	size_t bit  = state % 64;

	assert(word < set->word_count);

	set->words[word] |= UINT64_C(1) << bit;
}

int stateset_contains(const StateSet *set, StateId state) {
	size_t word = state / 64;
	size_t bit  = state % 64;

	assert(word < set->word_count);

	return (set->words[word] & (UINT64_C(1) << bit)) != 0;
}

int stateset_equal(const StateSet *left, const StateSet *right) {
	assert(left->word_count == right->word_count);

	return memcmp(left->words, right->words, left->word_count * sizeof *left->words) == 0;
}
