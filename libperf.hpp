#ifndef LIBPERF_HPP
#define LIBPERF_HPP
#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <string>

#include "libperf.h"

/**
 * @brief Declarations of libperf API for C++
 * @note Access to libperf in C++ in via an RAII-complaint container
 * @author Salih MSA
 */

namespace libperf {

	class Error : public std::error_category {
		public:
			const char* name() const noexcept override ;

			std::string message(int condition) const override ;
	} ;

	class Tracker {
		private:
			libperf_tracker* _tracker ; // internal, opaque C API object

		public:
			/**
			 * @brief Tracker (constructor) - initialises the libperf tracker
			 * @note See https://man7.org/linux/man-pages/man2/perf_event_open.2.html for valid combination for arguments below
			 * @note Stub to libperf_init()
			 * @param const pid_t id - process ID *or* thread ID to monitor
			 * @note Set -1 for system wide readings
			 * @brief const int cpu - pass in specific cpuid to track
			 * @note Set -1 for aggregate readings (of all CPUs)
			 * @throws std::system_error - We deem failure in cases of runtime errors (i.e. bad arguments, bad user permissions, lack of system resources, etc.). If a system has permanent, fixed issues that we are not capable of fixing (ie missing hardware), then there's nothing we can do so we run what we can but we print a warning to let user know. We throw the errno which caused the specific error
			 * @note Category of std::system_error will either be only be std::generic_category, as strictly dictated by the underlying C API
			 */
			explicit Tracker(const pid_t id, const int cpu) noexcept(false) ;

			/**
			 * @note Copy constructor + assignment deleted - there are so few scenarios where copying either the file handle (to watch the exact same events) or accessing the attributes would be desirable
			 * So I've explicitly deleted it
			 */
			Tracker(const Tracker& tracker) noexcept(false) = delete ;
			Tracker& operator=(const Tracker& tracker) noexcept(false) = delete ;

			/**
			 * @brief Tracker (move constructor) - acquire existing libperf tracker
			 * @param Tracker&& tracker - tracker to acquire
			 */
			explicit Tracker(Tracker&& tracker) noexcept ;

			/**
			 * @brief Tracker (move constructor) - acquire existing libperf tracker
			 * @param Tracker&& tracker - tracker to acquire
			 * @return Tracker& - new object which acquired
			 */
			Tracker& operator=(Tracker&& tracker) noexcept ;

			/**
			 * @brief toggle_counter - method enables, or disables an individual counter
			 * @note Stub to libperf_toggle_counter()
			 * @note Needed to configure value extraction, for use in `libperf_log` function 
			 * @param const enum libperf_tracepoint counter - counter type
			 * @param const bool toggle_type - true to enable, false to disable
			 * @throws std::system_error - thrown if we can't manipulate counter
			 * @note Category of std::system_error will either be std::generic_category, or libperf::Error. The former is when a system error occured, and the latter when there was an issue with the library. Whichever one of them is assigned depends on the underlying C API
			 */
			void toggle_counter(const enum libperf_tracepoint counter, const bool toggle_type) noexcept(false) ;

			/**
			 * @brief read_counter - method reads a specified counter
			 * @note You might want to use this, instead of the logging function, if you a) want to see the numbers real-time, b) only plan on recording one or two values
			 * @note Stub to libperf_read_counter()
			 * @pre this->toggle_counter(counter, true) - enable counter
			 * @param const enum libperf_tracepoint counter - counter type
			 * @return std::uint64_t - value of counter
			 * @throws std::system_error - thrown if we can't read value
			 * @note Category of std::system_error will either be std::generic_category, or libperf::Error. The former is when a system error occured, and the latter when there was an issue with the library. Whichever one of them is assigned depends on the underlying C API
			 */
			std::uint64_t read_counter(const enum libperf_tracepoint counter) const noexcept(false) ;

			/**
			 * @brief log - logs values of all counters for debugging/logging purposes
			 * @note Stub to libperf_log()
			 * @param std::FILE *const stream - output stream for logging
			 * @param const size_t tag - a unique identifier to tag log messages (you might want to log periodically, so i = 0 is first tag, etc.)
			 * @throws std::system_error - thrown if we face unexpected issue reading 1+ values
			 * @note Category of std::system_error will either be std::generic_category, or libperf::Error. The former is when a system error occured, and the latter when there was an issue with the library. Whichever one of them is assigned depends on the underlying C API
			 */
			void log(std::FILE *const stream, const std::size_t tag) const noexcept(false) ;

			/**
			 * @brief ~Tracker - function shuts down the library, performing cleanup
			 * @note Stub to libperf_fini()
			 */
			~Tracker() noexcept ;

	} ;

} // libperf

#endif // LIBPERF_HPP
