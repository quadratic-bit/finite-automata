#ifndef STATESET_H
#define STATESET_H

#include <automata/nfa.h>

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint64_t *words;
	size_t    word_count;
} StateSet;

void stateset_clear   (StateSet *set);
void stateset_add     (StateSet *set, StateId state);
int  stateset_contains(const StateSet *set, StateId state);
int  stateset_equal   (const StateSet *left, const StateSet *right);

#endif
