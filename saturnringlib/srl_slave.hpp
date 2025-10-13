
#pragma once

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
                return this->done;
            }

            /** @brief Task Running Status getter
             * @returns Task Running Status
             */
            virtual bool IsRunning()
            {
                return this->running;
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
                if (!this->running)
                {
                    this->done = false;
                }
                return !this->done;
            }

        protected:
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
