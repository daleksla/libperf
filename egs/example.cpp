#include <inttypes.h> // for PRIu64 definition
#include <cstdio> // for printf family
#include <cstdlib> // for EXIT_SUCCESS definition
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include "libperf.hpp"

/**
 * @brief Example using libperf in C++
 * @author Salih MSA, Wolfgang Richter, Vincent Bernardoff 
 */

int main(void)
{
	try {
		/* Initialise library */
		libperf::Tracker pd(getpid(), -1) ; // initialise libperf

		/* Enable counters */
		pd.toggle_counter(LIBPERF_COUNT_HW_CPU_CYCLES, true) ; // enable HW CPU cycles counter
		pd.toggle_counter(LIBPERF_COUNT_HW_INSTRUCTIONS, true) ;

		/* Read singular counter */
		std::uint64_t counter = pd.read_counter(LIBPERF_COUNT_HW_INSTRUCTIONS) ;
		std::fprintf(stdout, "counter read: %" PRIu64 "\n", counter);

		/* Logging */
		char logname[256] = {'\0'};
		if(std::snprintf(logname, sizeof(logname), "%d.data", getpid()) < 0)
		{ // if there was an error creating our logfile
			fprintf(stderr, "Unable to create log name\n");
			abort();
		}

		const int fd = open(logname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (fd < 0) { // if there was an error opening FD (with certain flags)
			fprintf(stderr, "Unable to create underlying file descriptor\n");
			abort();
		}

		std::FILE *const log = fdopen(fd, "a");
		if (log == NULL) { // if there was an error opening FD (with certain flags)
			fprintf(stderr, "Unable to wrap stream object\n");
			abort();
		}

		pd.log(log, 0); // log current counter values
		sleep(4) ; // sleep for a bit
		for (size_t i = 0; i < 10000; ++i); // do some busy work
		pd.log(log, 1); // log new counter values
	}
	catch(const std::system_error& err)
	{
		std::cerr << "Caught system_error with code " << err.code() << " meaning " << err.what() << std::endl ;
	}

	return 0;
}

