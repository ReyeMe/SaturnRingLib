#pragma once

#include <srl.hpp>
#include <cstdint>

/** @brief Busy time of each workload function, in FRT ticks (about 4.5 us each).
 *  @details Tests/Profiler/run_profiler_test.py checks the profile against these numbers.
 */
namespace WorkloadTicks
{
    constexpr uint16_t Calibrate = 100;
    constexpr uint16_t OuterOwn = 30;
    constexpr uint16_t InnerA = 50;
    constexpr uint16_t InnerB = 20;
    constexpr uint16_t SlaveWork = 40;
}

/** @brief Runs one frame of the workload: WarmUp(), Calibrate100() and Outer(), once each.
 *  @details Outer() spends 30 ticks itself and calls InnerA() twice and InnerB() once,
 *  so its total is about 150 ticks and its own time about 30.
 */
void RunWorkload();

/** @brief Runs WarmUp() and SlaveWork() (40 ticks) on the slave SH-2, profiled in the slave's tables. */
class SlaveWorkTask : public SRL::Types::ITask
{
protected:
    void Do() override;
};
