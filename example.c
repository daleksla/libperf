#include <inttypes.h> // for PRIu64 definition
#include <stdint.h> // for uint64_t and PRIu64
#include <stdio.h> // for printf family
#include <stdlib.h> // for EXIT_SUCCESS definition
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "libperf.h"

int main(void)
{
	/* Initialise library */
	libperf_tracker *pd = libperf_init(getpid(), -1); // initialise libperf
	if (pd == NULL) {
		fprintf(stderr, "didn't init. errno %s\n", strerror(errno));
		abort();
	}

	/* Enable counters */
	if (libperf_toggle_counter(pd, LIBPERF_COUNT_HW_CPU_CYCLES, true) != 0) { // enable HW CPU cycles counter
		fprintf(stderr, "unable to enable counter (perhaps it wasn't initialisable). errno %d\n", errno);
		abort();
	}

	if (libperf_toggle_counter(pd, LIBPERF_COUNT_HW_INSTRUCTIONS, true) != 0) { // enable HW instruction count counter
		fprintf(stderr, "unable to enable counter (perhaps it wasn't initialisable). errno %d\n", errno);
		abort();
	}

	/* Read singular counter */
	uint64_t counter;
	if (libperf_read_counter(pd, LIBPERF_COUNT_HW_INSTRUCTIONS, &counter) != 0) { // obtain single counter value
		fprintf(stderr, "unable to read counter (perhaps it wasn't initialisable in the first place, or most likely enabled). errno %d\n", errno);
		abort();
	} else {
		fprintf(stdout, "counter read: %" PRIu64 "\n", counter);
	}

	/* Logging */
	char logname[256] = {'\0'};
	if (snprintf(logname, sizeof(logname), "%d.data", getpid()) < 0) { // if there was an error creating our logfile
		fprintf(stderr, "Unable to create log name\n");
		abort();
	}

	const int fd = open(logname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) { // if there was an error opening FD (with certain flags)
		fprintf(stderr, "Unable to create underlying file descriptor\n");
		abort();
	}

	FILE *const log = fdopen(fd, "a");
	if (log == NULL) { // if there was an error opening FD (with certain flags)
		fprintf(stderr, "Unable to wrap stream object\n");
		abort();
	}

	libperf_log(pd, log, 0); // log current counter values
	sleep(4) ; // sleep for a bit
	for (size_t i = 0; i < 10000; ++i); // do some busy work
	libperf_log(pd, log, 1); // log new counter values

	/* Close library */
	libperf_fini(pd) ;

	return 0;
}

