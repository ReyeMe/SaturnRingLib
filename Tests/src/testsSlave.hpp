// Tests/src/testsSlave.hpp
// SRL::Slave and SRL::Types::ITask: running tasks on the slave CPU, running them again, and the task state the
// master CPU sees. The two CPUs' caches are not kept in sync, so the tests read task state through ReadUncached
// (campaign.hpp) unless they test the ITask getters themselves.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_slave.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Spin count before the master gives up waiting for the slave (a few seconds). */
static constexpr int SlaveSpinLimit = 20000000;

/** @brief Counts its runs; exposes its state read through the cache-through address. */
class SlaveCounterTask : public Types::ITask
{
public:
    int runs = 0;
    bool DoneUncached() const { return ReadUncached(this->done); }
    bool RunningUncached() const { return ReadUncached(this->running); }

protected:
    void Do() override { this->runs = ReadUncached(this->runs) + 1; }
};

/** @brief Runs until the master opens the gate, so the master can look at a running task. */
class SlaveGateTask : public SlaveCounterTask
{
public:
    bool open = false;
    bool started = false;

protected:
    void Do() override
    {
        this->started = true;
        while (!ReadUncached(this->open)) { }
        SlaveCounterTask::Do();
    }
};

/** @brief Waits until the task is done (read through the cache-through address); false on timeout. */
static bool slave_wait_done(const SlaveCounterTask &task)
{
    for (int spin = 0; spin < SlaveSpinLimit; spin++)
    {
        if (task.DoneUncached()) return true;
    }

    return false;
}

/** @brief A task runs on the slave CPU once and is then done. */
MU_TEST(slave_task_runs_and_finishes)
{
    static SlaveCounterTask task;
    Slave::ExecuteOnSlave(task);

    mu_assert(slave_wait_done(task), "The task did not finish (slave not started or stuck)");
    snprintf(buffer, buffer_size, "The task ran %d times, expected 1", ReadUncached(task.runs));
    mu_assert(ReadUncached(task.runs) == 1, buffer);
    mu_assert(!task.RunningUncached(), "The task is done but still marked running");
}

/** @brief A finished task can be run again. */
MU_TEST(slave_task_runs_again)
{
    static SlaveCounterTask task;
    Slave::ExecuteOnSlave(task);
    mu_assert(slave_wait_done(task), "The first run did not finish");

    Slave::ExecuteOnSlave(task);
    mu_assert(slave_wait_done(task), "The second run did not finish");
    snprintf(buffer, buffer_size, "The task ran %d times, expected 2", ReadUncached(task.runs));
    mu_assert(ReadUncached(task.runs) == 2, buffer);
}

/** @brief Two tasks started back to back both run. */
MU_TEST(slave_two_tasks_back_to_back)
{
    static SlaveCounterTask first;
    static SlaveCounterTask second;
    Slave::ExecuteOnSlave(first);
    Slave::ExecuteOnSlave(second);

    const bool firstDone = slave_wait_done(first);
    const bool secondDone = slave_wait_done(second);
    snprintf(buffer, buffer_size, "First task done %d (ran %d), second done %d (ran %d), expected both done once",
        firstDone, ReadUncached(first.runs), secondDone, ReadUncached(second.runs));
    mu_assert(firstDone && secondDone && ReadUncached(first.runs) == 1 && ReadUncached(second.runs) == 1, buffer);
}

/**
 * @brief IsDone() on the master sees the end of the task, even when the master read IsDone() before the task ran
 *        (that read leaves the old value in the master's cache).
 */
MU_TEST(slave_is_done_seen_by_master)
{
    static SlaveCounterTask task;
    task.ResetTask();
    const bool before = task.IsDone();   // brings the task's cache line into the master's cache

    Slave::ExecuteOnSlave(task);
    mu_assert(slave_wait_done(task), "The task did not finish");

    mu_assert(!before, "IsDone() was true before the task ran");
    mu_assert(task.IsDone(), "IsDone() is still false after the task finished: it reads the master's stale cache");
}

/**
 * @brief While a task runs, IsRunning() is true and ResetTask() returns false
 *        (srl_slave.hpp: "false if it is already running").
 */
MU_TEST(slave_reset_task_while_running)
{
    static SlaveGateTask task;
    task.open = false;
    task.started = false;
    Slave::ExecuteOnSlave(task);

    bool started = false;
    for (int spin = 0; spin < SlaveSpinLimit && !started; spin++) started = ReadUncached(task.started);

    // Look at the running task, then always open the gate so the slave is free for the next tests
    const bool runningSeen = task.RunningUncached();
    const bool resetResult = started ? task.ResetTask() : false;
    task.open = true;
    const bool finished = slave_wait_done(task);

    mu_assert(started && finished, "The gate task did not start or did not finish");
    mu_assert(runningSeen, "The task was not marked running while it ran");
    mu_assert(!resetResult, "ResetTask() returned true while the task was running");
}

MU_TEST_SUITE(slave_test_suite)
{
    MU_RUN_TEST(slave_task_runs_and_finishes);
    MU_RUN_TEST(slave_task_runs_again);
    MU_RUN_TEST(slave_two_tasks_back_to_back);
    MU_RUN_TEST(slave_is_done_seen_by_master);
    MU_RUN_TEST(slave_reset_task_while_running);
}
