#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <system_error>
#include <cstdint>
#include <cerrno>

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
		case LIBPERF_COUNTER_INVALID:
		{
			message = "Invalid counter supplied" ;
			break ;
		}
		case LIBPERF_COUNTER_UNINITIALISABLE:
		{
			message = "Counter specified uninitialisable" ;
			break ;
		}
		case LIBPERF_HANDLE_INVALID:
		{
			message = "Invalid handle" ;
			break ;
		}
		default:
		{
			message = "Unknown error" ;
		}
	}

	return message ;
}

libperf::Tracker::Tracker(const pid_t id, const int cpu) noexcept(false)
{
	this->_tracker = libperf_init(id, cpu) ;
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

std::uint64_t libperf::Tracker::read_counter(const enum libperf_tracepoint counter) const noexcept(false)
{
	std::uint64_t value ;

	const auto err = libperf_read_counter(this->_tracker, counter, &value) ;
	if(err != LIBPERF_SUCCESS)
	{
		if(err == LIBPERF_SYSTEM_ERROR)
		{
			throw std::system_error(errno, std::generic_category()) ;
		}
		else {
			throw std::system_error(err, libperf::Error()) ;
		}
	}

	return value ;
}

void libperf::Tracker::toggle_counter(const enum libperf_tracepoint counter, const bool toggle_type) noexcept(false)
{
	const auto err = libperf_toggle_counter(this->_tracker, counter, toggle_type) ;
	if(err != LIBPERF_SUCCESS)
	{
		if(err == LIBPERF_SYSTEM_ERROR)
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
	if(err != LIBPERF_SUCCESS)
	{
		if(err == LIBPERF_SYSTEM_ERROR)
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
