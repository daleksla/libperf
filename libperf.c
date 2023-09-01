#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <syslog.h> 

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#include "libperf.h"

/**
 * @brief Definitions of libperf API and inner functionality
 * @author Salih MSA, Wolfgang Richter, Vincent Bernardoff 
 */

#define LIBPERF_MAX_COUNTERS 32 // number of hardware counters we are even able to utilise

struct libperf_tracker { /* lib struct */
	int group; // who's the group leader (or -1 if you are)
	struct perf_event_attr *attrs; // list of events & their attributes. we will also use this to keep track of configuration information
	pid_t id; // process or thread ID
	int cpu; // CPU (or CPUs) to track
	int fds[LIBPERF_MAX_COUNTERS]; // set of counters
	unsigned long long wall_start; // time profiling
};

static inline unsigned long long rdclock(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000000000ULL + (unsigned long long)ts.tv_nsec;
}

struct stats { /* stats section */
	double n;
	double mean;
	double M2;
};

static void update_stats(struct stats *stats, const uint64_t val)
{
	++stats->n;
	const double delta = (double)val - stats->mean;
	stats->mean += delta / stats->n;
	stats->M2 += delta * ((double)val - stats->mean);
}

static inline double avg_stats(struct stats *stats)
{
	return stats->mean;
}

static inline int sys_perf_event_open(struct perf_event_attr *const hw_event, const pid_t id, const int cpu, const int group_fd, const unsigned long flags)
{
	return (int)syscall(__NR_perf_event_open, hw_event, id, cpu, group_fd, flags);
}

static struct perf_event_attr default_attrs[LIBPERF_MAX_COUNTERS] = { // detailed configuration information for the event being created
	// type = type of event,      config = type-specific configuration
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_CPU_CLOCK },
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_TASK_CLOCK },
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_CONTEXT_SWITCHES },
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_CPU_MIGRATIONS },
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_PAGE_FAULTS },
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_PAGE_FAULTS_MIN },
	{ .type = PERF_TYPE_SOFTWARE, .config = PERF_COUNT_SW_PAGE_FAULTS_MAJ },

	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_CPU_CYCLES },
	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_INSTRUCTIONS },
	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_CACHE_REFERENCES },
	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_CACHE_MISSES },
	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_BRANCH_MISSES },
	{ .type = PERF_TYPE_HARDWARE, .config = PERF_COUNT_HW_BUS_CYCLES },

	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_L1I | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_DTLB | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_ITLB | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)) },
	{ .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16)) },
	/* { .type = PERF_TYPE_HW_CACHE, .config = (PERF_COUNT_HW_CACHE_BPU | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16))}, */
};

libperf_tracker *libperf_init(const pid_t id, const int cpu)
{
	libperf_tracker *pd = malloc(sizeof(libperf_tracker));
	if (pd == NULL) {
		syslog(LOG_ERR, "libperf (in %s): unable to allocate memory for handle", __func__);
		return NULL;
	}

	pd->group = -1;

	for (size_t i = 0; i < LIBPERF_MAX_COUNTERS; ++i) {
		pd->fds[i] = -1;
	}

	pd->id = id;
	pd->cpu = cpu;

	pd->attrs = malloc(LIBPERF_MAX_COUNTERS * sizeof(struct perf_event_attr)); // create a space for local, configurable copy of the attributes of our counters
	if (pd->attrs == NULL) {
		syslog(LOG_ERR, "libperf (in %s): unable to allocate memory for perf events attributes", __func__);
		return NULL;
	}

	for (size_t i = 0; i < LIBPERF_MAX_COUNTERS; ++i) {
		/* firstly, we are going to initialise fields, for every single counter perf offers
		 * we will do so by copying over general data (counter stuff), then specifying additional fields
		 * TODO at some point, we need to allow the extra fields (ie those which don't pertain to what events to track, but rather HOW the features are tracked) to be customised
		 * this as it stands could be a default function, and then the described could be libperf_init_explicit or whatever where they give their own perf attributes
		 * but that's for a later date, and would most likely require an additional struct or enum parameters (or some combination of the two)
		 */
		if (
			trackers & (enum libperf_event)i != (enum libperf_event)i
			&&
			trackers != -1
		) { // check if user wants to track this specific flag / all flags
			pd->fds[i] = -1; // then we set to it an invalid fd immediately so we can't assume it's initialised down the line
			continue;
		}

		pd->attrs[i] = default_attrs[i]; // copy over general data
		pd->attrs[i].size = sizeof(struct perf_event_attr); // specifics: we include this due to kernel backcompatibility issues
		pd->attrs[i].inherit = 1; // specifics: children inherit being tracked
		pd->attrs[i].disabled = 1; // specifics: disable counters by default
		pd->attrs[i].enable_on_exec = 0; // specifics: do not enable counters due to exec* call
		if (pd->id != -1) { // if we aren't doing system wide analysis ...
			// then disable measuring statistics of the linux kernel - this will allow operation on more restrictive system
			// if we are, it's misleading to disable this information, and you'll simply need the correct permissions
			pd->attrs[i].exclude_kernel = 1;
			pd->attrs[i].exclude_hv = 1;
		}

		/* secondly, create event */
		pd->fds[i] = sys_perf_event_open(&pd->attrs[i], pd->id, pd->cpu, pd->group, 0); // open event, albeit with no additional flags
		if (pd->fds[i] < 0) {
			if (errno == E2BIG || errno == EACCES || errno == EBADF || errno == EBUSY || errno == EFAULT || errno == EINTR || errno == EMFILE || errno == ENOSPC || errno == EOVERFLOW || errno == EPERM || errno == ESRCH) { // error types listed are those which are due to programmer error or runtime issues with system
				syslog(LOG_ERR, "libperf (in %s): specified event #%lu is invalid thus aborting; refer to documentation & manual pages", __func__, i);
				return NULL;
			} else { // for others, we print a warning and that's it
				syslog(LOG_WARNING, "libperf (in %s): Event #%lu unsupported but continuing; refer to documentation & manual pages", __func__, i);
			}
		}
	}

	pd->wall_start = rdclock();

	syslog(LOG_INFO, "libperf (in %s): library initialised", __func__);
	return pd;
}

enum libperf_exit libperf_toggle_counter(libperf_tracker *const pd, const enum libperf_event counter, const enum libperf_event_toggle toggle_type, ...)
{
	if (pd == NULL) {
		syslog(LOG_ERR, "libperf (in %s): invalid handle", __func__);
		return LIBPERF_EXIT_HANDLE_INVALID;
	}

	if (counter < 0 || counter > LIBPERF_MAX_COUNTERS) {
		syslog(LOG_ERR, "libperf (in %s): invalid counter '%d' supplied\n", __func__, counter);
		return LIBPERF_EXIT_COUNTER_INVALID;
	}

	if (pd->fds[counter] < 0) { // if a specific counter isn't even active (due to failing in libperf_init)
		syslog(LOG_ERR, "libperf (in %s): counter '%d' not initialised", __func__, counter);
		return LIBPERF_EXIT_COUNTER_UNINITIALISABLE;
	}

	va_list args;
	va_start(args, toggle_type);
	switch (toggle_type) {
		case LIBPERF_EVENT_TOGGLE_ON:;
			if (ioctl(pd->fds[counter], PERF_EVENT_IOC_ENABLE) != 0) { // 0 is good, non-zero is bad 
				syslog(LOG_ERR, "libperf (in %s): unable to configure counter '%d'", __func__, counter);
				return LIBPERF_EXIT_SYSTEM_ERROR;
			}
			pd->attrs[counter].disabled = 0;
			break;
		case LIBPERF_EVENT_TOGGLE_OFF:;
			if (ioctl(pd->fds[counter], PERF_EVENT_IOC_DISABLE) != 0) {
				syslog(LOG_ERR, "libperf (in %s): unable to configure counter '%d'", __func__, counter);
				return LIBPERF_EXIT_SYSTEM_ERROR;
			}
			pd->attrs[counter].disabled = 1;
			break;
		case LIBPERF_EVENT_TOGGLE_RESET:;
			if (ioctl(pd->fds[counter], PERF_EVENT_IOC_RESET) != 0) {
				syslog(LOG_ERR, "libperf (in %s): unable to configure counter '%d'", __func__, counter);
				return LIBPERF_EXIT_SYSTEM_ERROR;
			}
			break;
		case LIBPERF_EVENT_TOGGLE_OVERFLOW_REFRESH:;
			const uint64_t overflows = va_arg(args, uint64_t);
			if (ioctl(pd->fds[counter], PERF_EVENT_IOC_REFRESH, overflows) != 0) {
				syslog(LOG_ERR, "libperf (in %s): unable to configure counter '%d'", __func__, counter);
				return LIBPERF_EXIT_SYSTEM_ERROR;
			}
			break;
		case LIBPERF_EVENT_TOGGLE_OVERFLOW_PERIOD:;
			uint64_t *const period = va_arg(args, uint64_t *);
			if (ioctl(pd->fds[counter], PERF_EVENT_IOC_PERIOD, period) != 0) {
				syslog(LOG_ERR, "libperf (in %s): unable to configure counter '%d'", __func__, counter);
				return LIBPERF_EXIT_SYSTEM_ERROR;
			}
			break;
		case LIBPERF_EVENT_TOGGLE_OUTPUT:;
			const uint32_t pause = va_arg(args, uint32_t);
			if (ioctl(pd->fds[counter], PERF_EVENT_IOC_PAUSE_OUTPUT, pause) != 0) {
				syslog(LOG_ERR, "libperf (in %s): unable to configure counter '%d'", __func__, counter);
				return LIBPERF_EXIT_SYSTEM_ERROR;
			}
			break;
		default:;
			syslog(LOG_ERR, "libperf (in %s): unsupported configuration supplied", __func__);
			return LIBPERF_EXIT_COUNTER_CONFIGURATION_UNSUPPORTED;
	}
	va_end(args);

	syslog(LOG_INFO, "libperf (in %s): counter '%d' manipulated successfully", __func__, counter);
	return LIBPERF_EXIT_SUCCESS;
}

enum libperf_exit libperf_read_counter(libperf_tracker *const pd, const enum libperf_event counter, uint64_t *const value)
{
	if (pd == NULL) {
		syslog(LOG_ERR, "libperf (in %s): invalid handle", __func__);
		return LIBPERF_EXIT_HANDLE_INVALID;
	}

	if (counter < 0 || counter > LIBPERF_MAX_COUNTERS) {
		syslog(LOG_ERR, "libperf (in %s): invalid counter '%d' supplied", __func__, counter);
		return LIBPERF_EXIT_COUNTER_INVALID;
	}

	if (counter == 32) { // act for a custom instruction
		*value = (uint64_t)(rdclock() - pd->wall_start);
	}
	else { // all other instructions
		if (pd->fds[counter] < 0) { // ie we weren't able to initialise it in the first place
			syslog(LOG_ERR, "libperf (in %s): counter '%d' not initialised", __func__, counter);
			return LIBPERF_EXIT_COUNTER_UNINITIALISABLE;
		}

		if (pd->attrs[counter].disabled == 1) {
			syslog(LOG_ERR, "libperf (in %s): counter '%d' disabled", __func__, counter);
			return LIBPERF_EXIT_COUNTER_DISABLED;
		}

		if (read(pd->fds[counter], value, sizeof(uint64_t)) != sizeof(uint64_t)) { // if there was an error reading the counter
			syslog(LOG_ERR, "libperf (in %s): unable to read event for counter '%d'", __func__, counter);
			return LIBPERF_EXIT_SYSTEM_ERROR;
		}
	}

	return LIBPERF_EXIT_SUCCESS;
}

enum libperf_exit libperf_log(libperf_tracker *const pd, FILE *const stream, const size_t tag)
{
	if (pd == NULL) {
		syslog(LOG_ERR, "libperf (in %s): invalid handle", __func__);
		return LIBPERF_EXIT_HANDLE_INVALID;
	}

	uint64_t count[3]; /* potentially 3 values */

	struct stats event_stats[LIBPERF_MAX_COUNTERS];

	struct stats walltime_nsecs_stats;

	for (size_t i = 0; i < LIBPERF_MAX_COUNTERS; ++i) {
		if (pd->fds[i] < 0) { // if event isn't monitorable on this system (failure to init)
			syslog(LOG_WARNING, "libperf (in %s): counter '%d' not initialised", __func__, i);
			continue; // then we move onto next stat
		}

		if (pd->attrs[i].disabled == 1) { // if event is currently disabled
			continue; // then we move onto next stat, we don't print error as people don't need to initialise everything
		}

		if (read(pd->fds[i], count, sizeof(uint64_t)) != sizeof(uint64_t)) { // if there was an error READING any of the values
			syslog(LOG_ERR, "libperf (in %s): unable to read event for counter '%d'", __func__, i);
			return LIBPERF_EXIT_SYSTEM_ERROR; // then we completely stop
		}

		update_stats(&event_stats[i], count[0]);

		fprintf(stream, "Stats[%lu, %lu]: %14.0f\n", tag, i, avg_stats(&event_stats[i]));
	}

	update_stats(&walltime_nsecs_stats, rdclock() - pd->wall_start);
	fprintf(stream, "Stats[%lu, %lu]: %14.9f\n", tag, LIBPERF_MAX_COUNTERS, avg_stats(&walltime_nsecs_stats) / 1e9);

	return LIBPERF_EXIT_SUCCESS;
}

void libperf_fini(libperf_tracker *const pd)
{
	if (pd == NULL) {
		syslog(LOG_ERR, "libperf (in %s): invalid handle", __func__);
		return;
	}

	for (size_t i = 0; i <  LIBPERF_MAX_COUNTERS; ++i) {
		if (pd->fds[i] >= 0) {
			close(pd->fds[i]);
		}
	}

	free(pd->attrs);
	free(pd);

	syslog(LOG_NOTICE, "libperf (in %s): library shut down", __func__);
}
