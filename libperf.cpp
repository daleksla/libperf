#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <system_error>
#include <cstdint>
#include <cerrno>
#include <cstdarg>

#include "libperf.h"

#include "libperf.hpp"

/**
 * @brief Definitions of libperf API in C++ and inner functionality
 * @author Salih MSA
 */

const char* libperf::Error::name() const noexcept
{
	return "libperf::Error" ;
}

std::string libperf::Error::message(int condition) const
{
	std::string message ;

	switch(condition)
	{
		case LIBPERF_EXIT_COUNTER_INVALID:
			message = "Invalid counter supplied" ;
			break ;
		case LIBPERF_EXIT_COUNTER_UNINITIALISABLE:
			message = "Counter specified uninitialisable" ;
			break ;
		case LIBPERF_EXIT_COUNTER_CONFIGURATION_UNSUPPORTED:
			message = "Configuration supplied is not supported" ;
			break ;
		case LIBPERF_EXIT_HANDLE_INVALID:
			message = "Invalid handle" ;
			break ;
		case LIBPERF_EXIT_COUNTER_DISABLED:
			message = "Counter currently disabled" ;
			break ;
		default:
			message = "Unknown error" ;
	}

	return message ;
}

libperf::Tracker::Tracker(const pid_t id, const int cpu, const int trackers) noexcept(false)
{
	this->_tracker = libperf_init(id, cpu, trackers) ;
	if(this->_tracker == nullptr)
	{
		throw std::system_error(errno, std::generic_category()) ;
	}
}

libperf::Tracker::Tracker(libperf::Tracker&& tracker) noexcept
{
	this->_tracker = tracker._tracker ;
	tracker._tracker = nullptr ;
}

libperf::Tracker& libperf::Tracker::operator=(libperf::Tracker&& tracker) noexcept
{
	this->_tracker = tracker._tracker ;
	tracker._tracker = nullptr ;

	return *this ;
}

std::uint64_t libperf::Tracker::read_counter(const libperf_event counter) const noexcept(false)
{
	std::uint64_t value ;

	const auto err = libperf_read_counter(this->_tracker, counter, &value) ;
	if(err != LIBPERF_EXIT_SUCCESS)
	{
		if(err == LIBPERF_EXIT_SYSTEM_ERROR)
		{
			throw std::system_error(errno, std::generic_category()) ;
		}
		else {
			throw std::system_error(err, libperf::Error()) ;
		}
	}

	return value ;
}

void libperf::Tracker::toggle_counter(const libperf_event counter, const libperf_event_toggle toggle_type, ...) noexcept(false)
{
	libperf_exit err ;

	va_list args ;
	va_start(args, toggle_type) ;
	switch(toggle_type)
	{
		case LIBPERF_EVENT_TOGGLE_OVERFLOW_REFRESH:
		{
			const std::uint64_t overflows = va_arg(args, std::uint64_t) ;
			err = libperf_toggle_counter(this->_tracker, counter, toggle_type, overflows) ;
			break ;
		}
		case LIBPERF_EVENT_TOGGLE_OVERFLOW_PERIOD:
		{
			std::uint64_t *const period = va_arg(args, std::uint64_t*) ;
			err = libperf_toggle_counter(this->_tracker, counter, toggle_type, period) ;
			break ;
		}
		case LIBPERF_EVENT_TOGGLE_OUTPUT:
		{
			const std::uint32_t pause = va_arg(args, std::uint32_t) ;
			err = libperf_toggle_counter(this->_tracker, counter, toggle_type, pause) ;
			break ;
		}
		default:
			err = libperf_toggle_counter(this->_tracker, counter, toggle_type) ;
	}
	va_end(args) ;

	if(err != LIBPERF_EXIT_SUCCESS)
	{
		if(err == LIBPERF_EXIT_SYSTEM_ERROR)
		{
			throw std::system_error(errno, std::generic_category()) ;
		}
		else {
			throw std::system_error(err, libperf::Error()) ;
		}
	}
}

void libperf::Tracker::log(std::FILE *const stream, const std::size_t tag) const noexcept(false)
{
	const auto err = libperf_log(this->_tracker, stream, tag) ;
	if(err != LIBPERF_EXIT_SUCCESS)
	{
		if(err == LIBPERF_EXIT_SYSTEM_ERROR)
		{
			throw std::system_error(errno, std::generic_category()) ;
		}
		else {
			throw std::system_error(err, libperf::Error()) ;
		}
	}
}

libperf::Tracker::~Tracker() noexcept
{
	libperf_fini(this->_tracker) ;
}
