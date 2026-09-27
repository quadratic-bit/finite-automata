#include <automata/dot.h>
#include <automata/nfa.h>
#include <automata/regex.h>

#include <stdio.h>
#include <string.h>

static int command_regex(const char *source) {
	Regex      regex = {0};
	RegexError error = {0};

	if (regex_parse(&regex, source, &error) != REGEX_OK) {
		fprintf(stderr, "regex:%zu: %s\n", error.position, error.message);

		return 1;
	}

	Nfa nfa = {0};

	if (nfa_from_regex(&nfa, &regex) != NFA_OK) {
		fprintf(stderr, "failed to construct NFA\n");
		regex_free(&regex);

		return 1;
	}

	if (nfa_write_dot(&nfa, stdout) != DOT_OK) {
		fprintf(stderr, "failed to write DOT\n");

		nfa_free(&nfa);
		regex_free(&regex);

		return 1;
	}

	nfa_free(&nfa);
	regex_free(&regex);

	return 0;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "usage: %s regex <expression>\n", argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "regex") == 0) {
		return command_regex(argv[2]);
	}

	fprintf(stderr, "unknown command: %s\n", argv[1]);
	return 1;
}
