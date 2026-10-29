#include <srl.hpp>

#include "workload.hpp"
#include "vdp1_load.hpp"

/**
 * @brief Runs a workload with known costs every frame, for the profiler to measure.
 * @details SRL::Core::Initialize() starts the profiler. Pull the data with GDB
 * (`srl-prof-dump`) or stream it (`SRL_PROFILER_REPORT_FRAMES`), then read it with
 * `tools/scripts/srl_prof.py`. See readme.md.
 */
int main()
{
    SRL::Core::Initialize(SRL::Types::HighColor::Colors::Black);

    // Hook cost in ticks (16.16) -> nanoseconds, for display.
    const uint32_t hookCostQ16 = SRL::Profiler::GetHookCostQ16();
    const uint32_t hookCostNs = static_cast<uint32_t>((static_cast<uint64_t>(hookCostQ16) * 1000000000ULL) / (static_cast<uint64_t>(g_srl_prof.Head.TickHz) << 16));

    SRL::Debug::Print(1, 1, "Profiler Sample");
    SRL::Debug::Print(1, 3, "Hook pair cost: %d ns", static_cast<int>(hookCostNs));
    SRL::Debug::Print(1, 5, "Connect GDB, then:");
    SRL::Debug::Print(2, 6, "source tools/scripts/srl_prof.gdb");
    SRL::Debug::Print(2, 7, "srl-prof-dump prof.bin");

    // Drop what the setup above recorded.
    SRL::Profiler::Reset();

    SlaveWorkTask slaveTask;
    uint32_t frame = 0;

    while (1)
    {
        {
            SRL_PROF_SCOPE("VDP1 load setup");
            DrawVdp1Load(frame++);
        }

        {
            SRL_PROF_SCOPE("Workload");
            RunWorkload();
        }

        {
            // Timed on the master too: the slave's own profile uses the slave's FRT.
            SRL_PROF_SCOPE("Slave job");
            SRL::Slave::ExecuteOnSlave(slaveTask);

            // Bounded, so a lost completion flag can't hang the sample.
            // IsDone(), not IsRunning(): that one only turns true once the slave starts the task.
            for (uint32_t wait = 0; !slaveTask.IsDone() && wait < 5000000U; wait++)
            {
            }
        }

        {
            SRL_PROF_SCOPE("Status text");
            SRL::Debug::Print(1, 9, "Frames profiled: %d   ", static_cast<int>(g_srl_prof.Head.Frames));
            SRL::Debug::Print(1, 10, "Reports sent: %d   ", static_cast<int>(g_srl_prof.Head.Reports));
        }

        SRL::Core::Synchronize();
    }

    return 0;
}
