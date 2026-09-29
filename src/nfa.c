#include <automata/nfa.h>

#include "vec.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static const size_t DEFAULT_BUILDER_CAP = 16;

typedef struct {
	StateId start;
	StateId accept;
} NfaFragment;

typedef struct {
	size_t state_count;

	NfaTransition *transitions;
	size_t         transition_count;
	size_t         transition_cap;
} NfaBuilder;

static NfaResult builder_new_state(NfaBuilder *builder, StateId *state) {
	if (builder->state_count == SIZE_MAX) {
		return NFA_ERR;
	}

	*state = builder->state_count;
	builder->state_count++;

	return NFA_OK;
}

static NfaResult builder_grow_transitions(NfaBuilder *builder) {
	size_t new_cap;

	if (!vec_next_cap(builder->transition_cap, DEFAULT_BUILDER_CAP, &new_cap)) {
		return NFA_ERR;
	}

	NfaTransition *transitions = vec_realloc(
		builder->transitions,
		new_cap,
		sizeof *builder->transitions
	);

	if (transitions == NULL) {
		return NFA_ERR;
	}

	builder->transitions    = transitions;
	builder->transition_cap = new_cap;

	return NFA_OK;
}

static NfaResult builder_add_transition(
	NfaBuilder *builder,
	StateId from,
	StateId to,
	NfaTransitionKind kind,
	unsigned char symbol
) {
	assert(from < builder->state_count);
	assert(to   < builder->state_count);

	if (builder->transition_count == builder->transition_cap &&
	    builder_grow_transitions(builder) != NFA_OK
	) {
		return NFA_ERR;
	}

	builder->transitions[builder->transition_count] = (NfaTransition){
		.from   = from,
		.to     = to,
		.kind   = kind,
		.symbol = symbol,
	};

	builder->transition_count++;
	return NFA_OK;
}

static NfaResult builder_add_epsilon(NfaBuilder *builder, StateId from, StateId to) {
	return builder_add_transition(builder, from, to, NFA_TRANSITION_EPSILON, 0);
}

static NfaResult builder_add_symbol(
	NfaBuilder *builder,
	StateId from,
	StateId to,
	unsigned char symbol
) {
	return builder_add_transition(builder, from, to, NFA_TRANSITION_SYMBOL, symbol);
}

static NfaResult build_node(NfaBuilder *builder, const RegexNode *node, NfaFragment *fragment) {
	assert(builder  != NULL);
	assert(node     != NULL);
	assert(fragment != NULL);

	switch (node->kind) {
	case REGEX_EMPTY: {
		StateId start;
		StateId accept;

		if (builder_new_state(builder, &start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_new_state(builder, &accept) != NFA_OK) {
			return NFA_ERR;
		}

		*fragment = (NfaFragment){.start = start, .accept = accept};
		return NFA_OK;
	}

	case REGEX_EPSILON: {
		StateId start;
		StateId accept;

		if (builder_new_state(builder, &start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_new_state(builder, &accept) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, start, accept) != NFA_OK) {
			return NFA_ERR;
		}

		*fragment = (NfaFragment){.start = start, .accept = accept};

		return NFA_OK;
	}

	case REGEX_LITERAL: {
		StateId start;
		StateId accept;

		if (builder_new_state(builder, &start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_new_state(builder, &accept) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_symbol(builder, start, accept, node->as.literal) != NFA_OK) {
			return NFA_ERR;
		}

		*fragment = (NfaFragment){.start = start, .accept = accept};

		return NFA_OK;
	}

	case REGEX_CONCAT: {
		NfaFragment left;
		NfaFragment right;

		if (build_node(builder, node->as.binary.left, &left) != NFA_OK) {
			return NFA_ERR;
		}

		if (build_node(builder, node->as.binary.right, &right) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, left.accept, right.start) != NFA_OK) {
			return NFA_ERR;
		}

		*fragment = (NfaFragment){.start  = left.start, .accept = right.accept};

		return NFA_OK;
	}

	case REGEX_ALT: {
		StateId start;
		StateId accept;

		NfaFragment left;
		NfaFragment right;

		if (builder_new_state(builder, &start) != NFA_OK) {
			return NFA_ERR;
		}

		if (build_node(builder, node->as.binary.left, &left) != NFA_OK) {
			return NFA_ERR;
		}

		if (build_node(builder, node->as.binary.right, &right) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_new_state(builder, &accept) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, start, left.start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, start, right.start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, left.accept, accept) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, right.accept, accept) != NFA_OK) {
			return NFA_ERR;
		}

		*fragment = (NfaFragment){.start  = start, .accept = accept};

		return NFA_OK;
	}

	case REGEX_STAR: {
		StateId start;
		StateId accept;

		NfaFragment inner;

		if (builder_new_state(builder, &start) != NFA_OK) {
			return NFA_ERR;
		}

		if (build_node(builder, node->as.child, &inner) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_new_state(builder, &accept) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, start, inner.start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, start, accept) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, inner.accept, inner.start) != NFA_OK) {
			return NFA_ERR;
		}

		if (builder_add_epsilon(builder, inner.accept, accept) != NFA_OK) {
			return NFA_ERR;
		}

		*fragment = (NfaFragment){.start  = start, .accept = accept};

		return NFA_OK;
	}
	}

	assert(0 && "Unknown regex node kind");
	return NFA_ERR;
}

NfaResult nfa_from_regex(Nfa *nfa, const Regex *regex) {
	assert(nfa         != NULL);
	assert(regex       != NULL);
	assert(regex->root != NULL);

	assert(nfa->state_count      == 0);
	assert(nfa->transition_count == 0);
	assert(nfa->transitions      == NULL);

	NfaBuilder  builder = {0};
	NfaFragment fragment;

	if (build_node(&builder, regex->root, &fragment) != NFA_OK) {
		free(builder.transitions);
		return NFA_ERR;
	}

	*nfa = (Nfa){
		.state_count      = builder .state_count,
		.start            = fragment.start,
		.accept           = fragment.accept,
		.transitions      = builder .transitions,
		.transition_count = builder .transition_count,
	};

	return NFA_OK;
}

void nfa_free(Nfa *nfa) {
	assert(nfa != NULL);

	free(nfa->transitions);
	*nfa = (Nfa){0};
}
