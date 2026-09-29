#include <automata/regex.h>

#include <criterion/criterion.h>
#include <criterion/parameterized.h>

static Regex parse(const char *source) {
	Regex      regex = {0};
	RegexError error = {0};

	RegexResult result = regex_parse(&regex, source, &error);

	cr_assert_eq(
		result,
		REGEX_OK,
		"failed to parse \"%s\" at %zu: %s",
		source,
		error.position,
		error.message != NULL ? error.message : "<no error>"
	);

	return regex;
}

static void assert_kind(const RegexNode *node, RegexNodeKind kind) {
	cr_assert_not_null(node);
	cr_assert_eq(node->kind, kind);
}

static void assert_literal(const RegexNode *node, unsigned char literal) {
	assert_kind(node, REGEX_LITERAL);
	cr_assert_eq(node->as.literal, literal);
}

Test(regex, literal) {
	Regex regex = parse("a");

	assert_literal(regex.root, 'a');

	regex_free(&regex);
}

Test(regex, empty_language) {
	Regex regex = parse("0");

	assert_kind(regex.root, REGEX_EMPTY);

	regex_free(&regex);
}

Test(regex, epsilon) {
	Regex regex = parse("1");

	assert_kind(regex.root, REGEX_EPSILON);

	regex_free(&regex);
}

Test(regex, concatenation) {
	Regex regex = parse("ab");

	assert_kind(regex.root, REGEX_CONCAT);

	assert_literal(regex.root->as.binary.left,  'a');
	assert_literal(regex.root->as.binary.right, 'b');

	regex_free(&regex);
}

Test(regex, alternation) {
	Regex regex = parse("a+b");

	assert_kind(regex.root, REGEX_ALT);

	assert_literal(regex.root->as.binary.left,  'a');
	assert_literal(regex.root->as.binary.right, 'b');

	regex_free(&regex);
}

Test(regex, kleene_star) {
	Regex regex = parse("a*");

	assert_kind(regex.root, REGEX_STAR);
	assert_literal(regex.root->as.child, 'a');

	regex_free(&regex);
}

Test(regex, star_has_highest_precedence) {
	Regex regex = parse("ab*");

	assert_kind(regex.root, REGEX_CONCAT);

	assert_literal(regex.root->as.binary.left, 'a');

	const RegexNode *right = regex.root->as.binary.right;

	assert_kind(right, REGEX_STAR);
	assert_literal(right->as.child, 'b');

	regex_free(&regex);
}

Test(regex, concat_has_higher_precedence_than_alt) {
	Regex regex = parse("a+bc");

	assert_kind(regex.root, REGEX_ALT);

	assert_literal(regex.root->as.binary.left, 'a');

	const RegexNode *right = regex.root->as.binary.right;

	assert_kind(right, REGEX_CONCAT);
	assert_literal(right->as.binary.left , 'b');
	assert_literal(right->as.binary.right, 'c');

	regex_free(&regex);
}

Test(regex, parentheses_override_precedence) {
	Regex regex = parse("(a+b)c");

	assert_kind(regex.root, REGEX_CONCAT);

	const RegexNode *left = regex.root->as.binary.left;

	assert_kind(left, REGEX_ALT);
	assert_literal(left->as.binary.left,  'a');
	assert_literal(left->as.binary.right, 'b');

	assert_literal(regex.root->as.binary.right, 'c');

	regex_free(&regex);
}

Test(regex, alternation_is_left_associative) {
	Regex regex = parse("a+b+c");

	assert_kind(regex.root, REGEX_ALT);

	const RegexNode *left = regex.root->as.binary.left;

	assert_kind(left, REGEX_ALT);
	assert_literal(left->as.binary.left,  'a');
	assert_literal(left->as.binary.right, 'b');

	assert_literal(regex.root->as.binary.right, 'c');

	regex_free(&regex);
}

Test(regex, concatenation_is_left_associative) {
	Regex regex = parse("abc");

	assert_kind(regex.root, REGEX_CONCAT);

	const RegexNode *left = regex.root->as.binary.left;

	assert_kind(left, REGEX_CONCAT);
	assert_literal(left->as.binary.left,  'a');
	assert_literal(left->as.binary.right, 'b');

	assert_literal(regex.root->as.binary.right, 'c');

	regex_free(&regex);
}

Test(regex, repeated_star) {
	Regex regex = parse("a**");

	assert_kind(regex.root, REGEX_STAR);
	assert_kind(regex.root->as.child, REGEX_STAR);
	assert_literal(regex.root->as.child->as.child, 'a');

	regex_free(&regex);
}

Test(regex, zero_and_one_are_reserved) {
	Regex regex = parse("01");

	assert_kind(regex.root, REGEX_CONCAT);
	assert_kind(regex.root->as.binary.left,  REGEX_EMPTY);
	assert_kind(regex.root->as.binary.right, REGEX_EPSILON);

	regex_free(&regex);
}

Test(regex, grouped_expression_under_star) {
	Regex regex = parse("(a+b)*");

	assert_kind(regex.root, REGEX_STAR);

	const RegexNode *inner = regex.root->as.child;

	assert_kind(inner, REGEX_ALT);
	assert_literal(inner->as.binary.left,  'a');
	assert_literal(inner->as.binary.right, 'b');

	regex_free(&regex);
}

typedef struct {
	char source[8];
} InvalidRegexCase;

ParameterizedTestParameters(regex, invalid) {
	static InvalidRegexCase cases[] = {
		{""},
		{"+a"},
		{"a+"},
		{"()"},
		{"(a"},
		{"a)"},
		{"*a"},
		{"a++b"},
	};

	return cr_make_param_array(
		InvalidRegexCase,
		cases,
		sizeof cases / sizeof cases[0]
	);
}

ParameterizedTest(InvalidRegexCase *param, regex, invalid) {
	Regex      regex = {0};
	RegexError error = {0};

	RegexResult result = regex_parse(
		&regex,
		param->source,
		&error
	);

	cr_assert_eq(
		result,
		REGEX_ERR,
		"expected \"%s\" to be rejected",
		param->source
	);

	cr_assert_not_null(error.message);
	cr_assert_null(regex.root);
	cr_assert_null(regex.arena.cur_block);
}
