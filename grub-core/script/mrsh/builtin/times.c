#include <errno.h>
#include <mrsh/builtin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <unistd.h>
#include "builtin.h"

static const char times_usage[] = "usage: times\n";

int builtin_times(struct mrsh_state *state, int argc, char *argv[]) {
	if (argc > 1) {
		fprintf(stderr, times_usage);
		return 1;
	}

	struct tms buf;
	long clk_tck = sysconf(_SC_CLK_TCK);
	if (clk_tck == -1) {
		perror("sysconf");
		return 1;
	}

	if (times(&buf) == (clock_t)-1) {
		perror("times");
		return 1;
	}

	printf("%dm%fs %dm%fs\n%dm%fs %dm%fs\n",
			(int)(buf.tms_utime / clk_tck / 60),
			((double) buf.tms_utime) / clk_tck,
			(int)(buf.tms_stime / clk_tck / 60),
			((double) buf.tms_stime) / clk_tck,
			(int)(buf.tms_cutime / clk_tck / 60),
			((double) buf.tms_cutime) / clk_tck,
			(int)(buf.tms_cstime / clk_tck / 60),
			((double)buf.tms_cstime) / clk_tck);

	return 0;
}
