#include <automata/dfa.h>
#include <automata/dot.h>
#include <automata/nfa.h>
#include <automata/regex.h>
#include <automata/reverse.h>

#include <stdio.h>
#include <string.h>

typedef enum {
	AUTOMATON_NFA,
	AUTOMATON_DFA,
} AutomatonKind;

typedef struct {
	AutomatonKind kind;

	union {
		Nfa nfa;
		Dfa dfa;
	} as;
} Automaton;

static void automaton_free(Automaton *automaton) {
	switch (automaton->kind) {
	case AUTOMATON_NFA:
		nfa_free(&automaton->as.nfa);
		break;

	case AUTOMATON_DFA:
		dfa_free(&automaton->as.dfa);
		break;
	}
}

static int automaton_determinize(Automaton *automaton) {
	if (automaton->kind == AUTOMATON_DFA) {
		return 1;
	}

	Dfa dfa = {0};

	if (dfa_from_nfa(&dfa, &automaton->as.nfa) != DFA_OK) {
		return 0;
	}

	nfa_free(&automaton->as.nfa);

	automaton->kind   = AUTOMATON_DFA;
	automaton->as.dfa = dfa;

	return 1;
}

static int automaton_minimize(Automaton *automaton) {
	if (!automaton_determinize(automaton)) {
		return 0;
	}

	Dfa minimized = {0};

	if (dfa_minimize(&minimized, &automaton->as.dfa) != DFA_OK) {
		return 0;
	}

	dfa_free(&automaton->as.dfa);
	automaton->as.dfa = minimized;

	return 1;
}

static int automaton_reverse(Automaton *automaton) {
	if (!automaton_determinize(automaton)) {
		return 0;
	}

	Nfa reversed = {0};

	if (nfa_reverse_dfa(&reversed, &automaton->as.dfa) != NFA_OK) {
		return 0;
	}

	dfa_free(&automaton->as.dfa);

	automaton->kind   = AUTOMATON_NFA;
	automaton->as.nfa = reversed;

	return 1;
}

static int automaton_complement(Automaton *automaton) {
	if (!automaton_determinize(automaton)) {
		return 0;
	}

	dfa_complement(&automaton->as.dfa);
	return 1;
}

static int apply_operation(Automaton *automaton, const char *operation) {
	if (strcmp(operation, "dfa") == 0) {
		return automaton_determinize(automaton);
	}

	if (strcmp(operation, "min") == 0) {
		return automaton_minimize(automaton);
	}

	if (strcmp(operation, "reverse") == 0) {
		return automaton_reverse(automaton);
	}

	if (strcmp(operation, "complement") == 0) {
		return automaton_complement(automaton);
	}

	fprintf(stderr, "unknown operation: %s\n", operation);
	return 0;
}

static int automaton_write_dot(const Automaton *automaton) {
	switch (automaton->kind) {
	case AUTOMATON_NFA:
		return nfa_write_dot(&automaton->as.nfa, stdout) == DOT_OK;

	case AUTOMATON_DFA:
		return dfa_write_dot(&automaton->as.dfa, stdout) == DOT_OK;
	}

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s regex <expression> [operation...]\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "regex") != 0) {
		fprintf(stderr, "expected 'regex'\n");
		return 1;
	}

	Regex      regex = {0};
	RegexError error = {0};

	if (regex_parse(&regex, argv[2], &error) != REGEX_OK) {
		fprintf(stderr, "regex error at %zu: %s\n", error.position, error.message);
		return 1;
	}

	Nfa nfa = {0};

	if (nfa_from_regex(&nfa, &regex) != NFA_OK) {
		fprintf(stderr, "failed to construct NFA\n");
		regex_free(&regex);
		return 1;
	}

	regex_free(&regex);

	Automaton automaton = {
		.kind   = AUTOMATON_NFA,
		.as.nfa = nfa,
	};

	for (int i = 3; i < argc; ++i) {
		if (!apply_operation(&automaton, argv[i])) {
			automaton_free(&automaton);
			return 1;
		}
	}

	if (!automaton_write_dot(&automaton)) {
		fprintf(stderr, "failed to write DOT\n");
		automaton_free(&automaton);
		return 1;
	}

	automaton_free(&automaton);

	return 0;
}
