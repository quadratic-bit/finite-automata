#ifndef AUTOMATA_QUERY_H
#define AUTOMATA_QUERY_H

#include <automata/dfa.h>
#include <automata/nfa.h>

#include <stddef.h>

typedef enum {
	QUERY_OK,
	QUERY_ERR,
} QueryResult;

QueryResult nfa_accepts(const Nfa *nfa, const unsigned char *word, size_t length, int *accepts);
QueryResult dfa_accepts(const Dfa *dfa, const unsigned char *word, size_t length, int *accepts);

QueryResult nfa_is_empty(const Nfa *nfa, int *empty);
QueryResult dfa_is_empty(const Dfa *dfa, int *empty);

QueryResult dfa_equivalent(const Dfa *left, const Dfa *right, int *equivalent);
QueryResult dfa_is_subset (const Dfa *left, const Dfa *right, int *subset);

#endif
