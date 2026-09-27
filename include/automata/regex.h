#ifndef AUTOMATA_REGEX_H
#define AUTOMATA_REGEX_H

#include <automata/arena.h>

#include <stddef.h>

typedef enum {
	REGEX_EMPTY,
	REGEX_EPSILON,
	REGEX_LITERAL,
	REGEX_CONCAT,
	REGEX_ALT,
	REGEX_STAR,
} RegexNodeKind;

typedef struct RegexNode RegexNode;
struct RegexNode {
	RegexNodeKind kind;

	union {
		unsigned char literal;

		struct {
			RegexNode *left;
			RegexNode *right;
		} binary;

		RegexNode *child;
	} as;
};

typedef struct {
	Arena      arena;
	RegexNode *root;
} Regex;

typedef struct {
	size_t      position;
	const char *message;
} RegexError;

typedef enum {
	REGEX_OK,
	REGEX_ERR,
} RegexResult;

RegexResult regex_parse(Regex *regex, const char *source, RegexError *error);

void regex_free(Regex *regex);

#endif
