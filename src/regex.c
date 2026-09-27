#include <automata/regex.h>

#include <assert.h>
#include <stddef.h>

typedef struct {
	const char *source;
	size_t      pos;

	Regex      *regex;
	RegexError *error;
} Parser;

static RegexNode *parse_alt   (Parser *parser);
static RegexNode *parse_concat(Parser *parser);
static RegexNode *parse_repeat(Parser *parser);
static RegexNode *parse_atom  (Parser *parser);

static char parser_peek(const Parser *parser) {
	return parser->source[parser->pos];
}

static char parser_advance(Parser *parser) {
	char c = parser_peek(parser);

	if (c != '\0') {
		parser->pos++;
	}

	return c;
}

static void parser_set_error(Parser *parser, const char *message) {
	if (parser->error == NULL || parser->error->message != NULL) {
		return;
	}

	parser->error->position = parser->pos;
	parser->error->message  = message;
}

static int parser_starts_atom(char c) {
	switch (c) {
	case '\0':
	case ')':
	case '+':
	case '*':
		return 0;

	default:
		return 1;
	}
}

static int parser_is_literal(char c) {
	switch (c) {
	case '\0':
	case '0':
	case '1':
	case '(':
	case ')':
	case '+':
	case '*':
		return 0;

	default:
		return 1;
	}
}

static RegexNode *node_new(Parser *parser, RegexNodeKind kind) {
	RegexNode *node = arena_alloc(
		&parser->regex->arena,
		sizeof *node,
		_Alignof(RegexNode)
	);

	if (node == NULL) {
		parser_set_error(parser, "out of memory");
		return NULL;
	}

	node->kind = kind;
	return node;
}

static RegexNode *node_literal(Parser *parser, unsigned char literal) {
	RegexNode *node = node_new(parser, REGEX_LITERAL);
	if (node == NULL) {
		return NULL;
	}

	node->as.literal = literal;
	return node;
}

static RegexNode *node_binary(
	Parser       *parser,
	RegexNodeKind kind,
	RegexNode    *left,
	RegexNode    *right
) {
	RegexNode *node = node_new(parser, kind);
	if (node == NULL) {
		return NULL;
	}

	node->as.binary.left  = left;
	node->as.binary.right = right;

	return node;
}

static RegexNode *node_unary(Parser *parser, RegexNodeKind kind, RegexNode *child) {
	RegexNode *node = node_new(parser, kind);
	if (node == NULL) {
		return NULL;
	}

	node->as.child = child;
	return node;
}

static RegexNode *parse_atom(Parser *parser) {
	char c = parser_peek(parser);

	switch (c) {
	case '0':
		parser_advance(parser);
		return node_new(parser, REGEX_EMPTY);

	case '1':
		parser_advance(parser);
		return node_new(parser, REGEX_EPSILON);

	case '(':
		parser_advance(parser);

		RegexNode *node = parse_alt(parser);
		if (node == NULL) {
			return NULL;
		}

		if (parser_peek(parser) != ')') {
			parser_set_error(parser, "expected ')'");
			return NULL;
		}

		parser_advance(parser);
		return node;

	default:
		break;
	}

	if (!parser_is_literal(c)) {
		parser_set_error(parser, "expected expression");
		return NULL;
	}

	parser_advance(parser);
	return node_literal(parser, (unsigned char)c);
}

static RegexNode *parse_repeat(Parser *parser) {
	RegexNode *node = parse_atom(parser);
	if (node == NULL) {
		return NULL;
	}

	while (parser_peek(parser) == '*') {
		parser_advance(parser);

		node = node_unary(parser, REGEX_STAR, node);
		if (node == NULL) {
			return NULL;
		}
	}

	return node;
}

static RegexNode *parse_concat(Parser *parser) {
	if (!parser_starts_atom(parser_peek(parser))) {
		parser_set_error(parser, "expected expression");
		return NULL;
	}

	RegexNode *left = parse_repeat(parser);
	if (left == NULL) {
		return NULL;
	}

	while (parser_starts_atom(parser_peek(parser))) {
		RegexNode *right = parse_repeat(parser);
		if (right == NULL) {
			return NULL;
		}

		left = node_binary(parser, REGEX_CONCAT, left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

static RegexNode *parse_alt(Parser *parser) {
	RegexNode *left = parse_concat(parser);
	if (left == NULL) {
		return NULL;
	}

	while (parser_peek(parser) == '+') {
		parser_advance(parser);

		RegexNode *right = parse_concat(parser);
		if (right == NULL) {
			return NULL;
		}

		left = node_binary(parser, REGEX_ALT, left, right);
		if (left == NULL) {
			return NULL;
		}
	}

	return left;
}

RegexResult regex_parse(Regex *regex, const char *source, RegexError *error) {
	assert(regex  != NULL);
	assert(source != NULL);
	assert(regex->arena.cur_block == NULL);
	assert(regex->root == NULL);

	if (error != NULL) {
		error->position = 0;
		error->message  = NULL;
	}

	if (arena_init(&regex->arena) != ARENA_OK) {
		if (error != NULL) {
			error->position = 0;
			error->message  = "out of memory";
		}

		return REGEX_ERR;
	}

	Parser parser = {
		.source = source,
		.pos    = 0,
		.regex  = regex,
		.error  = error,
	};

	RegexNode *root = parse_alt(&parser);
	if (root == NULL) {
		arena_free(&regex->arena);
		return REGEX_ERR;
	}

	if (parser_peek(&parser) != '\0') {
		parser_set_error(&parser, "unexpected character");
		arena_free(&regex->arena);
		return REGEX_ERR;
	}

	regex->root = root;
	return REGEX_OK;
}

void regex_free(Regex *regex) {
	assert(regex != NULL);

	arena_free(&regex->arena);
	regex->root = NULL;
}
