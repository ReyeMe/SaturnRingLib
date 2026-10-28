
#pragma once

#include <cstdint>

extern "C" {
    #include <sgl.h>  // For slSlaveFunc
}

namespace SRL
{
    namespace Types
    {
        /** @brief Abstract class that defines the class prototype to implement a Task that runs on Slave SH2
         */
        class ITask
        {
        public:
            /** @brief Destructor
             */
            virtual ~ITask() {}

            /** @brief Task Status getter
             * @returns Task Status
             */
            virtual bool IsDone()
            {
                return ITask::Uncached(this->done);
            }

            /** @brief Task Running Status getter
             * @returns Task Running Status
             */
            virtual bool IsRunning()
            {
                return ITask::Uncached(this->running);
            }

            /** @brief Start the Task on Slave SH2, then set its status to Done
             */
            virtual void Start()
            {
                if (!this->running)
                {
                    this->running = true;
                    this->Do();
                }
                this->done = true;
                this->running = false;
            }

            /** @brief Reset Task Status before running
             * @return true if the task is ready to run, false if it is already running
             */
            virtual bool ResetTask()
            {
                if (!ITask::Uncached(this->running))
                {
                    this->done = false;
                }
                return !ITask::Uncached(this->done);
            }

        protected:
            /** @brief Reads a status flag through the 0x20000000 cache-through mirror
             * @details The slave writes done/running and the master reads them. The two SH-2
             * caches don't snoop each other, so a master polling through its own cache can see
             * a stale value forever (e.g. IsRunning() never clearing).
             * @param flag Flag to read
             * @return Current value in memory
             */
            static bool Uncached(const volatile bool& flag)
            {
                return *reinterpret_cast<const volatile bool*>(reinterpret_cast<uint32_t>(&flag) | 0x20000000U);
            }

            /** @brief Holds a value indicating whether task has been completed
             */
            volatile bool done;

            /** @brief Holds a value indicating whether task has been completed
             * @return true if the task has been completed, false otherwise
             */
            volatile bool running;

            /** @brief Constructor
             */
            ITask() : done(false), running(false) {}

            /** @brief Abstract method that defines the task's behavior
             */
            virtual void Do() = 0;
        };
    }
    /** @brief Core functions of the library
    */
    class Slave
    {
    private:

        /** @brief Internal Wrapper function executed on Slave SH2 CPU
         * @param pTask Pointer to the ITask object to be executed
         * This function is called by the slSlaveFunc to execute the task on the Slave SH2.
         * It casts the void pointer to ITask and calls its Start method.
         * @note This function is not meant to be called directly, it is used internally by the library.
        */
        inline static void SlaveTask(void * pTask)
        {
#ifdef DEBUG
            // GDB breakpoints are patched into shared RAM by the master, which can't purge this
            // CPU's cache: purge it here, on the slave, before every task (DEBUG builds only).
            *reinterpret_cast<volatile uint8_t*>(0xFFFFFE92) |= 0x10;
            asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop" ::: "memory");
#endif

            Types::ITask * task = static_cast<Types::ITask *>(pTask);
            task->Start();
        }

    public:

        /** @brief API call to execute an ITask onto Slave SH2
        * @param task ITask object to be executed
        */
        inline static void ExecuteOnSlave(Types::ITask & task)
        {
            if(task.ResetTask())
            {
                slSlaveFunc(SlaveTask, static_cast<void *>(&task));
            }
        }

    };
};
