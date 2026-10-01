#ifndef AUTOMATA_OPS_H
#define AUTOMATA_OPS_H

#include <automata/dfa.h>
#include <automata/nfa.h>

NfaResult nfa_from_dfa(Nfa *nfa, const Dfa *dfa);
NfaResult nfa_union   (Nfa *nfa, const Nfa *left, const Nfa *right);

#endif
