#include <automata/query.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void epsilon_closure(const Nfa *nfa, unsigned char *states, StateId *stack) {
	size_t stack_len = 0;

	for (StateId state = 0; state < nfa->state_count; ++state) {
		if (states[state]) {
			stack[stack_len++] = state;
		}
	}

	while (stack_len > 0) {
		StateId state = stack[--stack_len];

		for (size_t i = 0; i < nfa->transition_count; ++i) {
			const NfaTransition *transition = &nfa->transitions[i];

			if (transition->kind != NFA_TRANSITION_EPSILON) {
				continue;
			}

			if (transition->from != state) {
				continue;
			}

			if (states[transition->to]) {
				continue;
			}

			states[transition->to] = 1;
			stack[stack_len++] = transition->to;
		}
	}
}

QueryResult nfa_accepts(const Nfa *nfa, const unsigned char *word, size_t length, int *accepts) {
	assert(nfa     != NULL);
	assert(word    != NULL || length == 0);
	assert(accepts != NULL);

	if (nfa->state_count > SIZE_MAX / sizeof(StateId)) {
		return QUERY_ERR;
	}

	unsigned char *current = calloc(nfa->state_count,  sizeof *current);
	unsigned char *next    = calloc(nfa->state_count,  sizeof *next);
	StateId       *stack   = malloc(nfa->state_count * sizeof *stack);

	if (current == NULL || next == NULL || stack == NULL) {
		free(stack);
		free(next);
		free(current);
		return QUERY_ERR;
	}

	current[nfa->start] = 1;
	epsilon_closure(nfa, current, stack);

	for (size_t pos = 0; pos < length; ++pos) {
		memset(next, 0, nfa->state_count * sizeof *next);

		for (size_t i = 0; i < nfa->transition_count; ++i) {
			const NfaTransition *transition = &nfa->transitions[i];

			if (transition->kind != NFA_TRANSITION_SYMBOL) {
				continue;
			}

			if (!current[transition->from]) {
				continue;
			}

			if (transition->symbol != word[pos]) {
				continue;
			}

			next[transition->to] = 1;
		}

		epsilon_closure(nfa, next, stack);

		unsigned char *tmp = current;
		current = next;
		next = tmp;
	}

	*accepts = current[nfa->accept] != 0;

	free(stack);
	free(next);
	free(current);

	return QUERY_OK;
}

QueryResult dfa_accepts(const Dfa *dfa, const unsigned char *word, size_t length, int *accepts) {
	assert(dfa     != NULL);
	assert(word    != NULL || length == 0);
	assert(accepts != NULL);

	StateId state = dfa->start;

	for (size_t pos = 0; pos < length; ++pos) {
		size_t symbol_index = dfa->alphabet_count;

		for (size_t i = 0; i < dfa->alphabet_count; ++i) {
			if (dfa->alphabet[i] == word[pos]) {
				symbol_index = i;
				break;
			}
		}

		if (symbol_index == dfa->alphabet_count) {
			*accepts = 0;
			return QUERY_OK;
		}

		state = dfa->transitions[state * dfa->alphabet_count + symbol_index];

		assert(state < dfa->state_count);
	}

	*accepts = dfa->accepting[state] != 0;
	return QUERY_OK;
}

QueryResult nfa_is_empty(const Nfa *nfa, int *empty) {
	assert(nfa   != NULL);
	assert(empty != NULL);

	if (nfa->state_count > SIZE_MAX / sizeof(StateId)) {
		return QUERY_ERR;
	}

	unsigned char *visited = calloc(nfa->state_count,  sizeof *visited);
	StateId       *stack   = malloc(nfa->state_count * sizeof *stack);

	if (visited == NULL || stack == NULL) {
		free(stack);
		free(visited);
		return QUERY_ERR;
	}

	size_t stack_len = 0;

	visited[nfa->start] = 1;
	stack[stack_len++] = nfa->start;

	while (stack_len > 0) {
		StateId state = stack[--stack_len];

		if (state == nfa->accept) {
			*empty = 0;
			free(stack);
			free(visited);
			return QUERY_OK;
		}

		for (size_t i = 0; i < nfa->transition_count; ++i) {
			const NfaTransition *transition = &nfa->transitions[i];

			if (transition->from != state) {
				continue;
			}

			if (visited[transition->to]) {
				continue;
			}

			visited[transition->to] = 1;
			stack[stack_len++] = transition->to;
		}
	}

	*empty = 1;

	free(stack);
	free(visited);

	return QUERY_OK;
}

QueryResult dfa_is_empty(const Dfa *dfa, int *empty) {
	assert(dfa   != NULL);
	assert(empty != NULL);

	if (dfa->state_count > SIZE_MAX / sizeof(StateId)) {
		return QUERY_ERR;
	}

	unsigned char *visited = calloc(dfa->state_count,  sizeof *visited);
	StateId       *stack   = malloc(dfa->state_count * sizeof *stack);

	if (visited == NULL || stack == NULL) {
		free(stack);
		free(visited);
		return QUERY_ERR;
	}

	size_t stack_len = 0;

	visited[dfa->start] = 1;
	stack[stack_len++] = dfa->start;

	while (stack_len > 0) {
		StateId state = stack[--stack_len];

		if (dfa->accepting[state]) {
			*empty = 0;
			free(stack);
			free(visited);
			return QUERY_OK;
		}

		for (size_t sym_index = 0; sym_index < dfa->alphabet_count; ++sym_index) {
			StateId target = dfa->transitions[state * dfa->alphabet_count + sym_index];

			assert(target < dfa->state_count);

			if (visited[target]) {
				continue;
			}

			visited[target] = 1;
			stack[stack_len++] = target;
		}
	}

	*empty = 1;

	free(stack);
	free(visited);

	return QUERY_OK;
}
