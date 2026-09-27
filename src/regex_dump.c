#include <automata/regex.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum {
	PRINT_MAX_DEPTH = 128
};

typedef struct {
	int has_next_sibling[PRINT_MAX_DEPTH];
} RegexPrinter;

typedef struct {
	RegexPrinter *printer;
	size_t        depth;
} PrintCtx;

static PrintCtx deep(PrintCtx old, size_t depth_delta) {
	assert(old.depth <= SIZE_MAX - depth_delta && "Depth overflow");

	return (PrintCtx){
		.printer = old.printer,
		.depth   = old.depth + depth_delta,
	};
}

static void set_next_sibling(PrintCtx ctx, int has_next) {
	assert(ctx.depth < PRINT_MAX_DEPTH && "Depth exceeded the limit");

	ctx.printer->has_next_sibling[ctx.depth] = has_next;
}

static void print_tab(PrintCtx ctx) {
	assert(ctx.depth < PRINT_MAX_DEPTH && "Depth exceeded the limit");

	if (ctx.depth == 0) return;

	for (size_t depth = 0; depth + 1 < ctx.depth; ++depth) {
		if (ctx.printer->has_next_sibling[depth]) printf("|  ");
		else                                      printf("   ");
	}

	printf("|--");
}

static void print_node(PrintCtx ctx, const RegexNode *node) {
	assert(node != NULL);

	print_tab(ctx);

	switch (node->kind) {
	case REGEX_EMPTY:
		printf("EMPTY 0\n");
		break;

	case REGEX_EPSILON:
		printf("EPSILON 1\n");
		break;

	case REGEX_LITERAL:
		printf("LITERAL %c\n", node->as.literal);
		break;

	case REGEX_CONCAT:
		printf("BINARY CONCAT\n");

		set_next_sibling(ctx, 1);
		print_node(deep(ctx, 1), node->as.binary.left);

		set_next_sibling(ctx, 0);
		print_node(deep(ctx, 1), node->as.binary.right);
		break;

	case REGEX_ALT:
		printf("BINARY +\n");

		set_next_sibling(ctx, 1);
		print_node(deep(ctx, 1), node->as.binary.left);

		set_next_sibling(ctx, 0);
		print_node(deep(ctx, 1), node->as.binary.right);
		break;

	case REGEX_STAR:
		printf("UNARY *\n");
		print_node(deep(ctx, 1), node->as.child);
		break;
	}
}

void regex_dump(const Regex *regex) {
	assert(regex != NULL);
	assert(regex->root != NULL);

	RegexPrinter printer = {
		.has_next_sibling = {0},
	};

	PrintCtx ctx = {
		.printer = &printer,
		.depth   = 0,
	};

	print_node(ctx, regex->root);
}
