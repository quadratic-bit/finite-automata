#include <automata/dfa.h>
#include <automata/dot.h>
#include <automata/nfa.h>
#include <automata/ops.h>
#include <automata/regex.h>
#include <automata/reverse.h>

#include <stdio.h>
#include <stdlib.h>
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

static const char *automaton_kind_name(AutomatonKind kind) {
	switch (kind) {
	case AUTOMATON_NFA: return "NFA";
	case AUTOMATON_DFA: return "DFA";
	}

	return "unknown";
}

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
	if (automaton->kind != AUTOMATON_NFA) {
		fprintf(
			stderr,
			"operation 'det' requires NFA, got %s\n",
			automaton_kind_name(automaton->kind)
		);
		return 0;
	}

	Dfa dfa = {0};

	if (dfa_from_nfa(&dfa, &automaton->as.nfa) != DFA_OK) {
		fprintf(stderr, "failed to determinize NFA\n");
		return 0;
	}

	nfa_free(&automaton->as.nfa);

	automaton->kind   = AUTOMATON_DFA;
	automaton->as.dfa = dfa;

	return 1;
}

static int automaton_minimize(Automaton *automaton) {
	if (automaton->kind != AUTOMATON_DFA) {
		fprintf(
			stderr,
			"operation 'min' requires DFA, got %s\n",
			automaton_kind_name(automaton->kind)
		);
		return 0;
	}

	Dfa minimized = {0};

	if (dfa_minimize(&minimized, &automaton->as.dfa) != DFA_OK) {
		fprintf(stderr, "failed to minimize DFA\n");
		return 0;
	}

	dfa_free(&automaton->as.dfa);
	automaton->as.dfa = minimized;

	return 1;
}

static int automaton_reverse(Automaton *automaton) {
	Nfa reversed = {0};

	switch (automaton->kind) {
	case AUTOMATON_NFA:
		if (nfa_reverse(&reversed, &automaton->as.nfa) != NFA_OK) {
			fprintf(stderr, "failed to reverse NFA\n");
			return 0;
		}

		nfa_free(&automaton->as.nfa);
		automaton->as.nfa = reversed;
		return 1;

	case AUTOMATON_DFA:
		if (nfa_reverse_dfa(&reversed, &automaton->as.dfa) != NFA_OK) {
			fprintf(stderr, "failed to reverse DFA\n");
			return 0;
		}

		dfa_free(&automaton->as.dfa);

		automaton->kind   = AUTOMATON_NFA;
		automaton->as.nfa = reversed;
		return 1;
	}

	return 0;
}

static int automaton_complement(Automaton *automaton) {
	if (automaton->kind != AUTOMATON_DFA) {
		fprintf(
			stderr,
			"operation 'compl' requires DFA, got %s\n",
			automaton_kind_name(automaton->kind)
		);
		return 0;
	}

	dfa_complement(&automaton->as.dfa);
	return 1;
}

static int automaton_to_nfa(Automaton *automaton) {
	if (automaton->kind != AUTOMATON_DFA) {
		fprintf(stderr, "operation 'nfa' requires DFA, got NFA\n");
		return 0;
	}

	Nfa nfa = {0};

	if (nfa_from_dfa(&nfa, &automaton->as.dfa) != NFA_OK) {
		fprintf(stderr, "failed to convert DFA to NFA\n");
		return 0;
	}

	dfa_free(&automaton->as.dfa);

	automaton->kind   = AUTOMATON_NFA;
	automaton->as.nfa = nfa;

	return 1;
}

static int automaton_union(Automaton *left, Automaton *right) {
	if (left->kind != AUTOMATON_NFA || right->kind != AUTOMATON_NFA) {
		fprintf(stderr, "operation 'union' requires two NFAs\n");
		return 0;
	}

	Nfa result = {0};

	if (nfa_union(&result, &left->as.nfa, &right->as.nfa) != NFA_OK) {
		fprintf(stderr, "failed to construct NFA union\n");
		return 0;
	}

	nfa_free(&left ->as.nfa);
	nfa_free(&right->as.nfa);

	left->kind   = AUTOMATON_NFA;
	left->as.nfa = result;

	return 1;
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
		fprintf(stderr, "usage: %s regex <expression> [det|min|rev|compl ...]\n", argv[0]);
		return 1;
	}

	Automaton *stack = calloc((size_t)argc, sizeof *stack);
	if (stack == NULL) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}

	size_t stack_len = 0;

	for (int i = 1; i < argc;) {
		const char *operation = argv[i];

		if (strcmp(operation, "regex") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "regex requires an expression\n");
				goto fail;
			}

			Regex      regex = {0};
			RegexError error = {0};

			if (regex_parse(&regex, argv[i + 1], &error) != REGEX_OK) {
				fprintf(stderr, "regex error at %zu: %s\n",
				        error.position, error.message);
				goto fail;
			}

			Nfa nfa = {0};

			if (nfa_from_regex(&nfa, &regex) != NFA_OK) {
				fprintf(stderr, "failed to construct NFA\n");
				regex_free(&regex);
				goto fail;
			}

			regex_free(&regex);

			stack[stack_len++] = (Automaton){
				.kind   = AUTOMATON_NFA,
				.as.nfa = nfa,
			};

			i += 2;
			continue;
		}

		if (stack_len == 0) {
			fprintf(stderr, "operation '%s' has no operand\n", operation);
			goto fail;
		}

		Automaton *top = &stack[stack_len - 1];

		if (strcmp(operation, "det") == 0) {
			if (!automaton_determinize(top)) goto fail;

		} else if (strcmp(operation, "nfa") == 0) {
			if (!automaton_to_nfa(top)) goto fail;

		} else if (strcmp(operation, "min") == 0) {
			if (!automaton_minimize(top)) goto fail;

		} else if (strcmp(operation, "rev") == 0) {
			if (!automaton_reverse(top)) goto fail;

		} else if (strcmp(operation, "compl") == 0) {
			if (!automaton_complement(top)) goto fail;

		} else if (strcmp(operation, "union") == 0) {
			if (stack_len < 2) {
				fprintf(stderr, "operation 'union' requires two operands\n");
				goto fail;
			}

			Automaton *left  = &stack[stack_len - 2];
			Automaton *right = &stack[stack_len - 1];

			if (!automaton_union(left, right)) goto fail;

			stack_len--;
		} else {
			fprintf(stderr, "unknown operation: %s\n", operation);
			goto fail;
		}

		i++;
	}

	if (stack_len != 1) {
		fprintf(stderr, "expected one resulting automaton, got %zu\n", stack_len);
		goto fail;
	}

	if (!automaton_write_dot(&stack[0])) {
		fprintf(stderr, "failed to write DOT\n");
		goto fail;
	}

	automaton_free(&stack[0]);
	free(stack);

	return 0;

fail:
	for (size_t i = 0; i < stack_len; ++i) {
		automaton_free(&stack[i]);
	}

	free(stack);
	return 1;
}
