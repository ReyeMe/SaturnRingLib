#include "slave_counter_task.hpp"

void SlaveCounterTask::Do()
{
    // Count primes by trial division: real work to step through on the slave.
    // Kept small: a range of 2000 visibly slowed the master's frame rate.
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
