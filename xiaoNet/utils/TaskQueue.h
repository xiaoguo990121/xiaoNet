/**
 * @file TaskQueue.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */
#pragma once

#include "NonCopyable.h"
#include <functional>
#include <future>
#include <string>
namespace xiaoNet
{

    /**
     * @brief This class is a pure virtual class that can be implemented as a
     * SerialTaskQueue or a ConcurrentTaskQueue.
     *
     */
    class TaskQueue : public NonCopyable
    {
    public:
        virtual void runTaskInQueue(const std::function<void()> &task) = 0;
        virtual void runTaskInQueue(std::function<void()> &&task) = 0;
        virtual std::string getName() const
        {
            return "";
        }

        /**
         * @brief Run a task in the queue synchronously. This means that the task is
         * executed before the method returns.
         *
         * @param task
         */
        void syncTaskInQueue(const std::function<void()> &task)
        {
            std::promise<int> prom;
            std::future<int> fut = prom.get_future();
            runTaskInQueue([&]()
                           {
                task();
                prom.set_value(1); });
            fut.get();
        }
        virtual ~TaskQueue()
        {
        }
    };

}