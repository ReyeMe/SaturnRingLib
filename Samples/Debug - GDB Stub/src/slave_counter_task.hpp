#pragma once

#include <srl.hpp>

/**
 * @brief Task run on the Slave SH-2, used to exercise SRL::Slave::ExecuteOnSlave().
 *
 * @warning Hardware-confirmed: SRL::GDBStub::InstallSlaveFreezeHandler() does
 * NOT reliably work in a program that also uses SRL::Slave::ExecuteOnSlave(),
 * in either ordering. This file's own comments on InstallSlaveFreezeHandler()
 * called that "likely but unverified" -- real-hardware testing of this sample
 * confirmed it two different ways:
 *   1. Redispatching this task every frame (an early version of this sample):
 *      SRL::GDBStub::g_slave_ici_count -- which should tick up by exactly one
 *      per debug stop -- incremented only for the first stop or two after
 *      boot, then went permanently silent.
 *   2. Dispatching this task a fixed 5 times at startup ONLY, then installing
 *      the freeze handler last and never touching SRL::Slave again: this
 *      does NOT fix it either. g_slave_ici_count read exactly 5 (matching
 *      the dispatch count, not any debug stop) immediately after boot, then
 *      stayed at 5 across 8 further real Ctrl-C-triggered debug stops --
 *      it tracks past SRL::Slave activity, not live freeze pulses.
 *
 * The working theory: SGL's slSlaveFunc uses the slave's FRT input-capture
 * interrupt to wake the slave for each dispatched job, and appears to leave
 * the slave's on-chip TIER.ICIE (interrupt enable) bit disabled once it has
 * no more queued work. That bit lives in the slave's own private on-chip
 * peripheral space -- the master cannot poke it directly across the bus, and
 * the only sanctioned way to run code on the slave that could re-arm it is
 * SRL::Slave::ExecuteOnSlave() itself, which reopens the same conflict.
 * There is no ordering or dispatch-count workaround found so far; a real fix
 * would need either a from-scratch (non-SGL) slave wake-up path or a way to
 * safely coexist with SGL's own internal use of the interrupt.
 *
 * This task is kept here purely to demonstrate SRL::Slave::ExecuteOnSlave()
 * itself working (it does, reliably) -- see main.cxx for how it's dispatched.
 * SRL::GDBStub::InstallSlaveFreezeHandler() is intentionally NOT called by
 * this sample any more; see the readme for the full writeup.
 */
class SlaveCounterTask : public SRL::Types::ITask
{
public:
    SlaveCounterTask() : counter(0), primeCount(0) {}

    uint32_t GetCounter() const
    {
        return this->counter;
    }

    /**
     * @brief Result of Do()'s workload: how many primes below kPrimeRange
     * were found on the most recent call. Exposed mainly so there's a
     * visible, changing result of the computation, not just a job count.
     */
    uint32_t GetPrimeCount() const
    {
        return this->primeCount;
    }

protected:
    void Do() override;

private:
    volatile uint32_t counter;
    volatile uint32_t primeCount;
};
