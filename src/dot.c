#include <automata/dot.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

static DotResult write_symbol(FILE *out, unsigned char symbol) {
	if (symbol == '"') {
		if (fputs("\\\"", out) == EOF) {
			return DOT_ERR;
		}

		return DOT_OK;
	}

	if (symbol == '\\') {
		if (fputs("\\\\", out) == EOF) {
			return DOT_ERR;
		}

		return DOT_OK;
	}

	if (symbol >= 0x20 && symbol <= 0x7e) {
		if (fputc((int)symbol, out) == EOF) {
			return DOT_ERR;
		}

		return DOT_OK;
	}

	if (fprintf(out, "0x%02X", (unsigned int)symbol) < 0) {
		return DOT_ERR;
	}

	return DOT_OK;
}

static DotResult write_transition(FILE *out, const NfaTransition *transition) {
	if (fprintf(out, "\tq%zu -> q%zu [label=\"", transition->from, transition->to) < 0) {
		return DOT_ERR;
	}

	switch (transition->kind) {
	case NFA_TRANSITION_EPSILON:
		if (fputs("ε", out) == EOF) {
			return DOT_ERR;
		}
		break;

	case NFA_TRANSITION_SYMBOL:
		if (write_symbol(out, transition->symbol) != DOT_OK) {
			return DOT_ERR;
		}
		break;
	}

	if (fputs("\"];\n", out) == EOF) {
		return DOT_ERR;
	}

	return DOT_OK;
}

DotResult nfa_write_dot(const Nfa *nfa, FILE *out) {
	assert(nfa != NULL);
	assert(out != NULL);

	assert(nfa->state_count > 0);
	assert(nfa->start  < nfa->state_count);
	assert(nfa->accept < nfa->state_count);

	if (fputs("digraph NFA {\n", out) == EOF) {
		return DOT_ERR;
	}

	if (fputs("\trankdir=LR;\n\n", out) == EOF) {
		return DOT_ERR;
	}

	if (fputs("\tstart [shape=point, label=\"\"];\n", out) == EOF) {
		return DOT_ERR;
	}

	if (fprintf(out, "\tstart -> q%zu;\n\n", nfa->start) < 0) {
		return DOT_ERR;
	}

	for (size_t state = 0; state < nfa->state_count; ++state) {
		const char *shape = state == nfa->accept ? "doublecircle" : "circle";

		if (fprintf(out, "\tq%zu [shape=%s];\n", state, shape) < 0) {
			return DOT_ERR;
		}
	}

	if (nfa->transition_count > 0) {
		if (fputc('\n', out) == EOF) {
			return DOT_ERR;
		}
	}

	for (size_t i = 0; i < nfa->transition_count; ++i) {
		const NfaTransition *transition = &nfa->transitions[i];

		assert(transition->from < nfa->state_count);
		assert(transition->to   < nfa->state_count);

		if (write_transition(out, transition) != DOT_OK) {
			return DOT_ERR;
		}
	}

	if (fputs("}\n", out) == EOF) {
		return DOT_ERR;
	}

	if (ferror(out)) {
		return DOT_ERR;
	}

	return DOT_OK;
}

DotResult dfa_write_dot(const Dfa *dfa, FILE *out) {
	assert(dfa != NULL);
	assert(out != NULL);

	assert(dfa->state_count > 0);
	assert(dfa->start < dfa->state_count);

	if (fputs("digraph DFA {\n", out) == EOF) {
		return DOT_ERR;
	}

	if (fputs("\trankdir=LR;\n\n", out) == EOF) {
		return DOT_ERR;
	}

	if (fputs("\tstart [shape=point, label=\"\"];\n", out) == EOF) {
		return DOT_ERR;
	}

	if (fprintf(out, "\tstart -> q%zu;\n\n", dfa->start) < 0) {
		return DOT_ERR;
	}

	for (size_t state = 0; state < dfa->state_count; ++state) {
		const char *shape = dfa->accepting[state] ? "doublecircle" : "circle";

		if (fprintf(out, "\tq%zu [shape=%s];\n", state, shape) < 0) {
			return DOT_ERR;
		}
	}

	if (dfa->state_count > 0 && dfa->alphabet_count > 0 && fputc('\n', out) == EOF) {
		return DOT_ERR;
	}

	for (StateId state = 0; state < dfa->state_count; ++state) {
		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];

			assert(target < dfa->state_count);

			if (fprintf(out, "\tq%zu -> q%zu [label=\"", state, target) < 0) {
				return DOT_ERR;
			}

			if (write_symbol(out, dfa->alphabet[sym_index]) != DOT_OK) {
				return DOT_ERR;
			}

			if (fputs("\"];\n", out) == EOF) {
				return DOT_ERR;
			}
		}
	}

	if (fputs("}\n", out) == EOF) {
		return DOT_ERR;
	}

	if (ferror(out)) {
		return DOT_ERR;
	}

	return DOT_OK;
}
