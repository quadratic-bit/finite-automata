#ifndef AUTOMATA_TEST_H
#define AUTOMATA_TEST_H

#include <automata/dfa.h>
#include <automata/nfa.h>

Nfa test_build_nfa          (const char *source);
Dfa test_build_dfa          (const char *source);
Dfa test_build_minimized_dfa(const char *source);

int test_nfa_accepts(const Nfa *nfa, const char *word);
int test_dfa_accepts(const Dfa *dfa, const char *word);

void test_assert_nfa_accepts(const Nfa *nfa, const char *word);
void test_assert_nfa_rejects(const Nfa *nfa, const char *word);

void test_assert_dfa_accepts(const Dfa *dfa, const char *word);
void test_assert_dfa_rejects(const Dfa *dfa, const char *word);

#endif
