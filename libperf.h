#ifndef LIBPERF_H
#define LIBPERF_H
#pragma once

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h> // needed for boolean support
#endif

#include <stdint.h>

/**
 * @brief Declarations of libperf API
 * @author Salih MSA, Wolfgang Richter, Vincent Bernardoff 
 */

struct libperf_tracker;
typedef struct libperf_tracker libperf_tracker;

/* lib constants */
enum libperf_tracepoint {
	/* sw tracepoints */
	LIBPERF_COUNT_SW_CPU_CLOCK = 0,
	LIBPERF_COUNT_SW_TASK_CLOCK = 1,
	LIBPERF_COUNT_SW_CONTEXT_SWITCHES = 2,
	LIBPERF_COUNT_SW_CPU_MIGRATIONS = 3,
	LIBPERF_COUNT_SW_PAGE_FAULTS = 4,
	LIBPERF_COUNT_SW_PAGE_FAULTS_MIN = 5,
	LIBPERF_COUNT_SW_PAGE_FAULTS_MAJ = 6,

	/* hw counters */
	LIBPERF_COUNT_HW_CPU_CYCLES = 7,
	LIBPERF_COUNT_HW_INSTRUCTIONS = 8,
	LIBPERF_COUNT_HW_CACHE_REFERENCES = 9,
	LIBPERF_COUNT_HW_CACHE_MISSES = 10,
	LIBPERF_COUNT_HW_BRANCH_INSTRUCTIONS = 11,
	LIBPERF_COUNT_HW_BRANCH_MISSES = 12,
	LIBPERF_COUNT_HW_BUS_CYCLES = 13,

	/* cache counters */

	/* L1D - data cache */
	LIBPERF_COUNT_HW_CACHE_L1D_LOADS = 14,
	LIBPERF_COUNT_HW_CACHE_L1D_LOADS_MISSES = 15,
	LIBPERF_COUNT_HW_CACHE_L1D_STORES = 16,
	LIBPERF_COUNT_HW_CACHE_L1D_STORES_MISSES = 17,
	LIBPERF_COUNT_HW_CACHE_L1D_PREFETCHES = 18,

	/* L1I - instruction cache */
	LIBPERF_COUNT_HW_CACHE_L1I_LOADS = 19,
	LIBPERF_COUNT_HW_CACHE_L1I_LOADS_MISSES = 20,

	/* LL - last level cache */
	LIBPERF_COUNT_HW_CACHE_LL_LOADS = 21,
	LIBPERF_COUNT_HW_CACHE_LL_LOADS_MISSES = 22,
	LIBPERF_COUNT_HW_CACHE_LL_STORES  = 23,
	LIBPERF_COUNT_HW_CACHE_LL_STORES_MISSES = 24,
	
	/* DTLB - data translation lookaside buffer */
	LIBPERF_COUNT_HW_CACHE_DTLB_LOADS = 25,
	LIBPERF_COUNT_HW_CACHE_DTLB_LOADS_MISSES = 26,
	LIBPERF_COUNT_HW_CACHE_DTLB_STORES = 27,
	LIBPERF_COUNT_HW_CACHE_DTLB_STORES_MISSES = 28,

	/* ITLB - instructiont translation lookaside buffer */
	LIBPERF_COUNT_HW_CACHE_ITLB_LOADS = 29,
	LIBPERF_COUNT_HW_CACHE_ITLB_LOADS_MISSES = 30,

	/* BPU - branch prediction unit */
	LIBPERF_COUNT_HW_CACHE_BPU_LOADS = 31,
	LIBPERF_COUNT_HW_CACHE_BPU_LOADS_MISSES = 32,

	/* Special internally defined "counter" */
	/* this is the _only_ floating point value */
	LIBPERF_LIB_SW_WALL_TIME = 33
};

/**
 * @brief libperf_init - function initialises the libperf library
 * @note See https://man7.org/linux/man-pages/man2/perf_event_open.2.html for valid combination for arguments below
 * @note TODO Initialises counters using some default & library specific attributes (latter e.g. initially disabled, enable on exec., etc.). An explicit init allows you to specify these
 * @param const pid_t id - process ID *or* thread ID to monitor
 * @note Set -1 for system wide readings
 * @brief const int cpu - pass in specific cpuid to track
 * @note Set -1 for aggregate readings (of all CPUs)
 * @return libperf_tracker* - handle for use in future library calls
 * @note return NULL if failure occurs. We deem failure in cases of runtime errors (i.e. bad arguments, bad user permissions, lack of system resources, etc.). If a system has permanent, fixed issues that we are not capable of fixing (ie missing hardware), then there's nothing we can do so we run what we can but we print a warning to let user know
 */
libperf_tracker *libperf_init(const pid_t id, const int cpu);

/**
 * @brief libperf_readcounter - funtion reads a specified counter
 * @note You might want to use this, instead of the logging function, if you a) want to see the numbers real-time, b) only plan on recording one or two values
 * @pre libperf_toggle_counter(..., counter, true) - enable counter
 * @param libperf_tracker *const pd - library structure obtained from libperf_initialise()
 * @param const enum libperf_tracepoint counter - counter type
 * @param uint64_t *const value - value to write value out to
 * @return int - exit code
 */
int libperf_read_counter(libperf_tracker *const pd, const enum libperf_tracepoint counter, uint64_t *const value);

/**
 * @brief libperf_toggle_counter - this function enables, or disables an individual counter
 * @note Needed to configure logging, for use in `libperf_log` function 
 * @param libperf_tracker *const pd - library structure obtained from libperf_initialise()
 * @param const enum libperf_tracepoint counter - counter type
 * @param const bool toggle_type - true to enable, false to disable
 * @return int - exit code
 */
int libperf_toggle_counter(libperf_tracker *const pd, const enum libperf_tracepoint counter, const bool toggle_type);

/**
 * @brief libperf_log - logs values of all counters for debugging/logging purposes
 * @note Log files are named after the `id` argument that was used when calling libperf_initialise
 * @note Log is saved in the same location in the current working directory
 * @param libperf_tracker *const pd - library structure obtained from libperf_initialise()
 * @param FILE *const stream - output stream for logging
 * @param const size_t tag - a unique identifier to tag log messages (you might want to log periodically, so i = 0 is first tag, etc.)
 */
int libperf_log(libperf_tracker *const pd, FILE *const stream, const size_t tag);

/**
 * @brief libperf_fini - function shuts down the library, performing cleanup
 * @note It should always be called when you're finished tracing to avoid memory leaks
 * @param libperf_tracker *const pd - library structure obtained from libperf_initialise()
 */
void libperf_fini(libperf_tracker *const pd);

#ifdef __cplusplus
}
#endif

#endif // LIBPERF_H
