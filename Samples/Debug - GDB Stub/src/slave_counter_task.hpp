#pragma once

#include <srl.hpp>

/**
 * @brief Small job run on the slave SH-2 with SRL::Slave::ExecuteOnSlave().
 * @details Set `break SlaveCounterTask::Do` to stop on the slave; GDB shows it as thread 2.
 */
class SlaveCounterTask : public SRL::Types::ITask
{
public:
    SlaveCounterTask() : counter(0), primeCount(0) {}

    /** @brief Number of jobs completed. */
    uint32_t GetCounter() const
    {
        return this->counter;
    }

    /** @brief Result of the last job: the number of primes below 500. */
    uint32_t GetPrimeCount() const
    {
        return this->primeCount;
    }

protected:
    void Do() override;

private:
    // volatile: written by the slave, read by the master.
    volatile uint32_t counter;
    volatile uint32_t primeCount;
};
