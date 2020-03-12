#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <mrsh/buffer.h>
#include <mrsh/shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "mrsh_getopt.h"

static const char getopts_usage[] = "usage: getopts optstring name [arg...]\n";

int builtin_getopts(struct mrsh_state *state, int argc, char *argv[]) {
	_mrsh_optind = 0;
	if (_mrsh_getopt(argc, argv, ":") != -1) {
		fprintf(stderr, "getopts: unknown option -- %c\n", _mrsh_optopt);
		fprintf(stderr, getopts_usage);
		return 1;
	}
	if (_mrsh_optind + 2 < argc) {
		fprintf(stderr, getopts_usage);
		return 1;
	}

	int optc;
	char **optv;
	if (_mrsh_optind + 2 > argc) {
		optc = argc - _mrsh_optind - 2;
		optv = &argv[_mrsh_optind + 2];
	} else {
		optc = state->frame->argc;
		optv = state->frame->argv;
	}
	char *optstring = argv[_mrsh_optind];
	char *name = argv[_mrsh_optind + 1];

	const char *optind_str = mrsh_env_get(state, "OPTIND", NULL);
	if (optind_str == NULL) {
		fprintf(stderr, "getopts: OPTIND is not defined\n");
		return 1;
	}
	char *endptr;
	long optind_long = strtol(optind_str, &endptr, 10);
	if (endptr[0] != '\0' || optind_long <= 0 || optind_long > INT_MAX) {
		fprintf(stderr, "getopts: OPTIND is not a positive integer\n");
		return 1;
	}
	_mrsh_optind = (int)optind_long;

	_mrsh_optopt = 0;
	int opt = _mrsh_getopt(optc, optv, optstring);

	char optind_fmt[16];
	snprintf(optind_fmt, sizeof(optind_fmt), "%d", _mrsh_optind);
	mrsh_env_set(state, "OPTIND", optind_fmt, MRSH_VAR_ATTRIB_NONE);

	if (_mrsh_optopt != 0) {
		if (opt == ':') {
			char value[] = {(char)_mrsh_optopt, '\0'};
			mrsh_env_set(state, "OPTARG", value, MRSH_VAR_ATTRIB_NONE);
		} else if (optstring[0] != ':') {
			mrsh_env_unset(state, "OPTARG");
		} else {
			// either missing option-argument or unknown option character
			// in the former case, unset OPTARG
			// in the latter case, set OPTARG to _mrsh_optopt
			bool opt_exists = false;
			size_t len = strlen(optstring);
			for (size_t i = 0; i < len; ++i) {
				if (optstring[i] == _mrsh_optopt) {
					opt_exists = true;
					break;
				}
			}
			if (opt_exists) {
				mrsh_env_unset(state, "OPTARG");
			} else {
				char value[] = {(char)_mrsh_optopt, '\0'};
				mrsh_env_set(state, "OPTARG", value, MRSH_VAR_ATTRIB_NONE);
			}
		}
	} else if (_mrsh_optarg != NULL) {
		mrsh_env_set(state, "OPTARG", _mrsh_optarg, MRSH_VAR_ATTRIB_NONE);
	} else {
		mrsh_env_unset(state, "OPTARG");
	}

	char value[] = {opt == -1 ? (char)'?' : (char)opt, '\0'};
	mrsh_env_set(state, name, value, MRSH_VAR_ATTRIB_NONE);

	if (opt == -1) {
		return 1;
	}
	return 0;
}
