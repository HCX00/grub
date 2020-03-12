#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <mrsh/shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include "builtin.h"
#include "mrsh_getopt.h"

static const char ulimit_usage[] = "usage: ulimit [-f] [blocks]\n";

int builtin_ulimit(struct mrsh_state *state, int argc, char *argv[]) {
	_mrsh_optind = 0;
	int opt;
	while ((opt = _mrsh_getopt(argc, argv, ":f")) != -1) {
		if (opt == 'f') {
			// Nothing here
		} else {
			fprintf(stderr, "%s", ulimit_usage);
			return 1;
		}
	}

	if (_mrsh_optind == argc - 1) {
		char *arg = argv[_mrsh_optind];
		char *end;
		long int new_limit = strtol(arg, &end, 10);
		if (end == arg || end[0] != '\0') {
			fprintf(stderr, "ulimit: invalid argument: %s\n", arg);
			return 1;
		}
		struct rlimit new = {
			.rlim_cur = new_limit * 512,
			.rlim_max = new_limit * 512
		};
		if (setrlimit(RLIMIT_FSIZE, &new) != 0) {
			perror("setrlimit");
			return 1;
		}
	} else if (_mrsh_optind == argc) {
		struct rlimit old = { 0 };
		if (getrlimit(RLIMIT_FSIZE, &old) != 0) {
			perror("getrlimit");
			return 1;
		}
		if (old.rlim_max == RLIM_INFINITY) {
			printf("unlimited\n");
		} else {
			printf("%" PRIuMAX "\n", (uintmax_t)(old.rlim_max / 512));
		}
	} else {
		fprintf(stderr, "%s", ulimit_usage);
		return 1;
	}

	return 0;
}
