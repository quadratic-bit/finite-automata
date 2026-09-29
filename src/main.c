#include <automata/dfa.h>
#include <automata/dot.h>
#include <automata/nfa.h>
#include <automata/regex.h>

#include <stdio.h>
#include <string.h>

typedef struct {
	int dfa;
	int minimize;
} Options;

static int command_regex(const char *source, const Options *options) {
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

	regex_free(&regex);

	if (!options->dfa && !options->minimize) {
		if (nfa_write_dot(&nfa, stdout) != DOT_OK) {
			fprintf(stderr, "failed to write DOT\n");
			nfa_free(&nfa);

			return 1;
		}

		nfa_free(&nfa);
		return 0;
	}

	Dfa dfa = {0};

	if (dfa_from_nfa(&dfa, &nfa) != DFA_OK) {
		fprintf(stderr, "failed to determinize NFA\n");
		nfa_free(&nfa);

		return 1;
	}

	nfa_free(&nfa);

	if (!options->minimize) {
		if (dfa_write_dot(&dfa, stdout) != DOT_OK) {
			fprintf(stderr, "failed to write DOT\n");
			dfa_free(&dfa);

			return 1;
		}

		dfa_free(&dfa);
		return 0;
	}

	Dfa minimized = {0};

	if (dfa_minimize(&minimized, &dfa) != DFA_OK) {
		fprintf(stderr, "failed to minimize DFA\n");
		dfa_free(&dfa);

		return 1;
	}

	dfa_free(&dfa);

	if (dfa_write_dot(&minimized, stdout) != DOT_OK) {
		fprintf(stderr, "failed to write DOT\n");
		dfa_free(&minimized);

		return 1;
	}

	dfa_free(&minimized);
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s regex <expression> [--dfa] [--min]\n", argv[0]);

		return 1;
	}

	if (strcmp(argv[1], "regex") != 0) {
		fprintf(stderr, "unknown command: %s\n", argv[1]);
		return 1;
	}

	Options options = {0};

	for (int i = 3; i < argc; ++i) {
		if (strcmp(argv[i], "--dfa") == 0) {
			options.dfa = 1;
			continue;
		}

		if (strcmp(argv[i], "--min") == 0) {
			options.minimize = 1;
			continue;
		}

		fprintf(stderr, "unknown option: %s\n", argv[i]);
		return 1;
	}

	return command_regex(argv[2], &options);
}
