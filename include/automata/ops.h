#ifndef AUTOMATA_OPS_H
#define AUTOMATA_OPS_H

#include <automata/dfa.h>
#include <automata/nfa.h>

NfaResult nfa_from_dfa(Nfa *nfa, const Dfa *dfa);
NfaResult nfa_star    (Nfa *nfa, const Nfa *source);
NfaResult nfa_union   (Nfa *nfa, const Nfa *left, const Nfa *right);
NfaResult nfa_concat  (Nfa *nfa, const Nfa *left, const Nfa *right);
DfaResult dfa_union   (Dfa *dfa, const Dfa *left, const Dfa *right);
DfaResult dfa_inter   (Dfa *dfa, const Dfa *left, const Dfa *right);
DfaResult dfa_diff    (Dfa *dfa, const Dfa *left, const Dfa *right);
DfaResult dfa_sym_diff(Dfa *dfa, const Dfa *left, const Dfa *right);

#endif
