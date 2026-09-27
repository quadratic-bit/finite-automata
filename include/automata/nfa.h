#ifndef AUTOMATA_NFA_H
#define AUTOMATA_NFA_H

#include <automata/regex.h>

#include <stddef.h>

typedef size_t StateId;

typedef enum {
	NFA_TRANSITION_EPSILON,
	NFA_TRANSITION_SYMBOL,
} NfaTransitionKind;

typedef struct {
	StateId           from;
	StateId           to;
	NfaTransitionKind kind;
	unsigned char     symbol;
} NfaTransition;

typedef struct {
	size_t state_count;

	StateId start;
	StateId accept;

	NfaTransition *transitions;
	size_t         transition_count;
} Nfa;

typedef enum {
	NFA_OK,
	NFA_ERR,
} NfaResult;

NfaResult nfa_from_regex(Nfa *nfa, const Regex *regex);

void nfa_free(Nfa *nfa);

#endif
