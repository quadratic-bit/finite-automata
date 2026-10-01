#ifndef DFA_PRODUCT_H
#define DFA_PRODUCT_H

#include <automata/dfa.h>

typedef int (*DfaProductAccept)(int left, int right);

DfaResult dfa_product(Dfa *dfa, const Dfa *left, const Dfa *right, DfaProductAccept accept);

#endif
