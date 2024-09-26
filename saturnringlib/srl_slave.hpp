#pragma once

/** @brief Main SGL namespace
 */
extern "C" {
    #include <sgl.h>
}

namespace SRL
{
    class ITask
    {
        public:
            ITask() : _done(false) {}
            virtual ~ITask() {}
            virtual void Do() = 0;
            virtual void Callback() {}
            virtual bool IsDone()
            {
              return _done;
            }
            virtual void SetDone()
            {
              _done = true;
            }
            virtual void ResetDone()
            {
              _done = false;
            }
        protected:
          bool  _done;
    };

    /** @brief Core functions of the library
     */
    class Slave
    {
    private:

        inline static void SlaveTask(void * pTask)
        {
            ITask * task = static_cast<ITask *>(pTask);
            task->ResetDone();
            task->Do();
            task->Callback();
            task->SetDone();
        }

    public:

        /** @brief ...
         */
        inline static void ExecuteOnSlave(ITask * task)
        {
            //task->ResetDone();;
            slSlaveFunc(SlaveTask, static_cast<void *>(task));
        }

    private:

    };
};
