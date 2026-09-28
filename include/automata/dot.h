#ifndef AUTOMATA_DOT_H
#define AUTOMATA_DOT_H

#include <automata/dfa.h>
#include <automata/nfa.h>

#include <stdio.h>

typedef enum {
	DOT_OK,
	DOT_ERR,
} DotResult;

DotResult nfa_write_dot(const Nfa *nfa, FILE *out);
DotResult dfa_write_dot(const Dfa *dfa, FILE *out);

#endif
