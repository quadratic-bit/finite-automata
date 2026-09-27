#ifndef AUTOMATA_DOT_H
#define AUTOMATA_DOT_H

#include <automata/nfa.h>

#include <stdio.h>

typedef enum {
	DOT_OK,
	DOT_ERR,
} DotResult;

DotResult nfa_write_dot(const Nfa *nfa, FILE *out);

#endif
