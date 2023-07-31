# libperf
## README

libperf is a library that wraps around the `perf_event_open()` syscall, to expose the Linux kernel performance counters subsystem to userspace applications.

## Summarising

This is a fork from theonewolf/libperf, who wrote it due to the following:
> This document describes the libperf library implemented as an interface to a
syscall introduced in a mature form into the Linux Kernel mainline in 2009. This
interface was previously only usable via a binary tool called 'perf' included in
Linux Kernel source distributions today.
There are several key shortcomings of interfacing with the system call only
through a controlled binary.  The first is that the granularity of tracing
hardware performance counter registers and software tracepoints is at a whole
binary level.  There is no method for tracing portions of code within the
application if desired.  This is because there is no nice API exposed to
userspace developers that can interface with this system call.  The second
shortcoming is that the tool, although extensive, may not account for every
possible use case for these counters and tracepoints.  Offering a
non-restrictive API to userspace gives developers full power and freedom to
implement any functionality desired.
libperf closes this gap and provides an API for interfacing with the system
call used by the perf tool.  This provides all of the power of the tool with
the granularity of snippets of code.

The perf subsystem exposes a lot of details on a program execution, but this library is focussed on userspace performance analysis. As such, this library covers and automatically generates event monitors for the following types:
- PERF_TYPE_HARDWARE
- PERF_TYPE_SOFTWARE
- PERF_TYPE_HW_CACHE

At some point, support will be added for:
- PERF_TYPE_BREAKPOINT (would be as part of an explicit initialiser where you specify attributes manually)
- The userspace probe `Uprobe`, though it's a newer feature and makes less sense in static profiling (you can just create a series of events, then run the code, and read from the events after)

This fork was created as:
- The original creator does not maintain the library
- Components of original library, if failed, would kill running process (bad for libraries)
- To provide informative, non-invasive feedback using `syslog`
- To include a C++11 API following established standards & practises (RAII, exception handling)

## Building

libperf has 4 dependancies:
- A modern Linux Kernel (post v2.6.31)
- GNU make utility
- C99 conformant compiler
- CXX11 conformant compiler

Run either of following steps to build:
- `make` or `make all` for a full build 
- `make library` for library build only
- `make examples` for library build only

## Using

### C API

Include `libperf.h`, and call `libperf_initialise` function - this provides a handle to be used for future library calls.
- If the id value is -1, then the library will monitor the entire system
- Setting the cpu value to -1 causes the libperf counters to count across all CPUs for a given id (i.e. aggregate CPU statistics)

The available counters are defined in the `enum libperf_event`; `libperf_toggle_counter` are used to configure (e.g. enable, disable) individual counters as desired. `libperf_readcounter` is then used to read a single (enabled) 64 bit counter from the library.

Use `libperf_log` to then obtin a log of all counters - this appends logs into a file named after the PID value passed into `libperf_initialise`.

Finally, call `libperf_close` to shut down the library

The return value of each function can be used to discern whether errors occured or not. For all functions except the initialisation function, an integer code is returned:
- If the code is 0, function executed successfully
- If the code is 1, there was an error interfacing with the `libperf` library (e.g. due to bad user parameters)
- If the code is 2, everything was fine until interacting with the system itself failed

For the latter case, the value of `errno` should be used in the moment to obtain the cause. Logging, for both errors, will inform of the action which failed to execute

Refer to `examples/example.c`

### CXX API

All functions from the C API are put into namespace `libperf`, as methods of class `libperf::Perf` which follows the RAII idiom.

The exception `std::system_error` is thrown when errors occur, distinguishing either a system error code (i.e. `errno`) or a library-centric one. Logging functionality remains the same

Refer to `examples/example.cpp`

### Compiling 

Statically link to archive output `libperf.a`

---

See LICENSE for terms of usage.
