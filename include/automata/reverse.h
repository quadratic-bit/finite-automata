#ifndef AUTOMATA_REVERSE_H
#define AUTOMATA_REVERSE_H

#include <automata/dfa.h>
#include <automata/nfa.h>

NfaResult nfa_reverse_dfa(Nfa *nfa, const Dfa *dfa);

#endif
