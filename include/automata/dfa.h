#ifndef AUTOMATA_DFA_H
#define AUTOMATA_DFA_H

#include <automata/nfa.h>

#include <stddef.h>

typedef struct {
	size_t  state_count;
	StateId start;

	unsigned char *alphabet;
	size_t         alphabet_count;

	unsigned char *accepting;
	StateId       *transitions;
} Dfa;

typedef enum {
	DFA_OK,
	DFA_ERR,
} DfaResult;

DfaResult dfa_from_nfa(Dfa *dfa, const Nfa *nfa);
DfaResult dfa_minimize(Dfa *dfa, const Dfa *source);

void dfa_free(Dfa *dfa);

#endif
