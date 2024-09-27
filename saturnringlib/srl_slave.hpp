#pragma once

extern "C" {
    #include <sgl.h>  // For slSlaveFunc
}

/** @brief Main SGL namespace
*/
namespace SRL
{
    /** @brief Abstract class that defines the class prototype to implement
    *          a Task that runs on Slave SH2
    */
    class ITask
    {
    public:
        /** @brief Constructor
        */
        ITask() : _done(false) {}

        /** @brief Destructor
        */
        virtual ~ITask() {}

        /** @brief Task Status getter
        * @returns Task Status
        */
        virtual bool IsDone()
        {
            return _done;
        }

        /** @brief Start the Task on Slave SH2, then set its status to Done
        */
        virtual void Start()
        {
            this->Do();
            _done = true;
        }

        /** @brief Reset Task Status before running
        */
        virtual void ResetTask()
        {
            _done = false;
        }

    protected:
        volatile bool  _done;

        /** @brief Absctract method that defines the task's behavior
        */
        virtual void Do() = 0;
    };

    /** @brief Core functions of the library
    */
    class Slave
    {
    private:

        /** @brief Internal Wrapper function executed on Slave SH2 CPU
        */
        inline static void SlaveTask(void * pTask)
        {
            ITask * task = static_cast<ITask *>(pTask);
            task->Start();
        }

    public:

        /** @brief API call to execute an ITask onto Slave SH2
        * @param task ITask object to be executed
        */
        inline static void ExecuteOnSlave(ITask & task)
        {
            task.ResetTask();
            slSlaveFunc(SlaveTask, static_cast<void *>(&task));
        }

    };
};
