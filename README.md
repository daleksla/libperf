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

This fork was created as:
- The original creator does not maintain the library
- Components of original library, if failed, would kill running process (bad for libraries)
- To include a C++ API, including an RAII container

## Building

libperf has 4 dependancies:
- A modern Linux Kernel (post v2.6.31)
- GNU make utility
- C99 conformant compiler
- CXX11 conformant compiler


Run the following step to build the library for *use*:
- `make` or `make all`

## Using 

### C API

Include `libperf.h`, and call `libperf_initialise` function - this provides a handle to be used for future library calls.
- If the id value is -1, then the library will monitor the entire system
- Setting the cpu value to -1 causes the libperf counters to count across all CPUs for a given id (i.e. aggregate CPU statistics)

The available counters are defined in the `enum libperf_tracepoint`; `libperf_enablecounter` and `libperf_disablecounter` are used to enable and
disable individual counters as desired. 'libperf_readcounter' is then used to read a single (enabled) 64 bit counter from the library.

Use `libperf_log` to then obtin a log of all counters - this appends logs into a file named after the PID value passed into `libperf_initialise`.

Finally, call `libperf_close` to shut down the library

Refer to `example.c`

### CXX API

All functions from the C API are put into namespace `libperf`, with exception `std::runtime_error` being thrown if errors occur. Furthermore, the `libperf::Perf` approaches access to this library via the RAII idiom 

### Compiling 

Statically link to archive output `libperf.a`

---

See LICENSE for terms of usage.
