#include "slave_counter_task.hpp"

void SlaveCounterTask::Do()
{
    // Deliberately more compute-intensive than a bare increment, so the
    // slave is genuinely busy for a measurable stretch each call instead
    // of returning near-instantly -- counts primes below kPrimeRange via
    // plain trial division, a classic CPU-bound workload. Recomputed
    // from scratch every call (not incremental), so this is real,
    // repeated work dispatched to the slave, not a one-time cost paid
    // only at startup.
    //
    // Kept modest (hardware-confirmed 2000 caused a visible framerate
    // regression even after the master's own dispatch was made
    // non-blocking -- see the throttled dispatch site in main.cxx for
    // the working theory: sustained slave activity contending with the
    // master for the shared memory bus). main.cxx's throttled dispatch
    // interval is the primary mitigation; this is kept small too so
    // each individual burst of slave activity is short.
    static constexpr uint32_t kPrimeRange = 500;
    uint32_t primes = 0;
    for (uint32_t n = 2; n < kPrimeRange; ++n)
    {
        bool isPrime = true;
        for (uint32_t d = 2; d * d <= n; ++d)
        {
            if (n % d == 0) { isPrime = false; break; }
        }
        if (isPrime) { ++primes; }
    }

    this->primeCount = primes;
    this->counter = this->counter + 1;
}
