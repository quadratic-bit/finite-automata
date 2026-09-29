#include <automata/dot.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void nfa_mark_finishable(const Nfa *nfa, unsigned char *finishable) {
	finishable[nfa->accept] = 1;

	int changed;

	do {
		changed = 0;

		for (size_t i = 0; i < nfa->transition_count; ++i) {
			const NfaTransition *transition = &nfa->transitions[i];

			if (finishable[transition->to] && !finishable[transition->from]) {
				finishable[transition->from] = 1;
				changed = 1;
			}
		}
	} while (changed);
}

static void dfa_mark_finishable(const Dfa *dfa, unsigned char *finishable) {
	for (StateId state = 0; state < dfa->state_count; ++state) {
		if (dfa->accepting[state]) {
			finishable[state] = 1;
		}
	}

	int changed;

	do {
		changed = 0;

		for (StateId state = 0; state < dfa->state_count; ++state) {
			if (finishable[state]) {
				continue;
			}

			for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
				StateId target = dfa->transitions[
					state * dfa->alphabet_count + sym_index
				];

				assert(target < dfa->state_count);

				if (finishable[target]) {
					finishable[state] = 1;
					changed = 1;
					break;
				}
			}
		}
	} while (changed);
}

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

	unsigned char *finishable = calloc(nfa->state_count, sizeof *finishable);
	if (finishable == NULL) {
		return DOT_ERR;
	}

	nfa_mark_finishable(nfa, finishable);

	if (fputs("digraph NFA {\n", out) == EOF) {
		goto fail;
	}

	if (fputs("\trankdir=LR;\n\n", out) == EOF) {
		goto fail;
	}

	if (fputs("\tstart [shape=point, label=\"\"];\n", out) == EOF) {
		goto fail;
	}

	if (finishable[nfa->start]) {
		if (fprintf(out, "\tstart -> q%zu;\n", nfa->start) < 0) {
			goto fail;
		}
	}

	if (fputc('\n', out) == EOF) {
		goto fail;
	}

	for (StateId state = 0; state < nfa->state_count; ++state) {
		if (!finishable[state]) {
			continue;
		}

		const char *shape = state == nfa->accept ? "doublecircle" : "circle";

		if (fprintf(out, "\tq%zu [shape=%s];\n", state, shape) < 0) {
			goto fail;
		}
	}

	if (fputc('\n', out) == EOF) {
		goto fail;
	}

	for (size_t i = 0; i < nfa->transition_count; ++i) {
		const NfaTransition *transition = &nfa->transitions[i];

		assert(transition->from < nfa->state_count);
		assert(transition->to   < nfa->state_count);

		if (!finishable[transition->from] || !finishable[transition->to]) {
			continue;
		}

		if (write_transition(out, transition) != DOT_OK) {
			goto fail;
		}
	}

	if (fputs("}\n", out) == EOF) {
		goto fail;
	}

	if (ferror(out)) {
		goto fail;
	}

	free(finishable);
	return DOT_OK;

fail:
	free(finishable);
	return DOT_ERR;
}

DotResult dfa_write_dot(const Dfa *dfa, FILE *out) {
	assert(dfa != NULL);
	assert(out != NULL);

	assert(dfa->state_count > 0);
	assert(dfa->start < dfa->state_count);

	unsigned char *finishable = calloc(dfa->state_count, sizeof *finishable);
	if (finishable == NULL) {
		return DOT_ERR;
	}

	dfa_mark_finishable(dfa, finishable);

	if (fputs("digraph DFA {\n", out) == EOF) {
		goto fail;
	}

	if (fputs("\trankdir=LR;\n\n", out) == EOF) {
		goto fail;
	}

	if (fputs("\tstart [shape=point, label=\"\"];\n", out) == EOF) {
		goto fail;
	}

	if (finishable[dfa->start]) {
		if (fprintf(out, "\tstart -> q%zu;\n", dfa->start) < 0) {
			goto fail;
		}
	}

	if (fputc('\n', out) == EOF) {
		goto fail;
	}

	for (StateId state = 0; state < dfa->state_count; ++state) {
		if (!finishable[state]) {
			continue;
		}

		const char *shape = dfa->accepting[state] ? "doublecircle" : "circle";

		if (fprintf(out, "\tq%zu [shape=%s];\n", state, shape) < 0) {
			goto fail;
		}
	}

	if (fputc('\n', out) == EOF) {
		goto fail;
	}

	for (StateId state = 0; state < dfa->state_count; ++state) {
		if (!finishable[state]) {
			continue;
		}

		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];

			assert(target < dfa->state_count);

			if (!finishable[target]) {
				continue;
			}

			if (fprintf(out, "\tq%zu -> q%zu [label=\"", state, target) < 0) {
				goto fail;
			}

			if (write_symbol(out, dfa->alphabet[sym_index]) != DOT_OK) {
				goto fail;
			}

			if (fputs("\"];\n", out) == EOF) {
				goto fail;
			}
		}
	}

	if (fputs("}\n", out) == EOF) {
		goto fail;
	}

	if (ferror(out)) {
		goto fail;
	}

	free(finishable);
	return DOT_OK;

fail:
	free(finishable);
	return DOT_ERR;
}
