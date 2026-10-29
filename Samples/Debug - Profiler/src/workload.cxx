#include <srl.hpp>

#include "workload.hpp"

// This file is built with -finstrument-functions (see SRL_PROFILE_SOURCES in the makefile).
// noinline keeps each function a real call, so the profile shows the structure written here.

/** @brief Busy-waits a number of FRT ticks, on the FRT of the CPU running it.
 *  @details Inlined and not instrumented: its time counts as its caller's own time, and the
 *  PC sampler finds the caller's code, not Spin()'s.
 */
__attribute__((always_inline, no_instrument_function)) static inline void Spin(uint16_t ticks)
{
    // FRC is the upper 16 bits of Low; the 16-bit difference handles wrap-around.
    const uint16_t start = static_cast<uint16_t>(SRL::Timer::Capture().Low >> 16);

    while (static_cast<uint16_t>(static_cast<uint16_t>(SRL::Timer::Capture().Low >> 16) - start) < ticks)
    {
    }
}

// The first profiled call of a frame runs with the hook code out of the cache,
// which adds about 1.5 ticks to it. WarmUp() takes that hit so the functions below measure hot.
__attribute__((noinline)) void WarmUp()
{
    Spin(1);
}

__attribute__((noinline)) void Calibrate100()
{
    Spin(WorkloadTicks::Calibrate);
}

__attribute__((noinline)) void InnerA()
{
    Spin(WorkloadTicks::InnerA);
}

__attribute__((noinline)) void InnerB()
{
    Spin(WorkloadTicks::InnerB);
}

__attribute__((noinline)) void Outer()
{
    Spin(WorkloadTicks::OuterOwn);
    InnerA();
    InnerA();
    InnerB();
}

void RunWorkload()
{
    WarmUp();
    Calibrate100();
    Outer();
}

__attribute__((noinline)) void SlaveWork()
{
    Spin(WorkloadTicks::SlaveWork);
}

void SlaveWorkTask::Do()
{
    // DEBUG builds purge the slave's cache before every task: WarmUp() takes the misses.
    WarmUp();
    SlaveWork();
}
