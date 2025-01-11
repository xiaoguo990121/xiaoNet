/**
 * @file SerialTaskQueue.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once

#include "TaskQueue.h"
#include <xiaoNet/net/EventLoopThread.h>
#include <xiaoNet/exports.h>

namespace xiaoNet
{
    /**
     * @brief This class represents a task queue in which all tasks are executed one by one
     *
     */
    class XIAONET_EXPORT SerialTaskQueue : public TaskQueue // 串行任务队列
    {
    public:
        /**
         * @brief Run a task in the queue.
         *
         * @param task
         */
        virtual void runTaskInQueue(const std::function<void()> &task);
        virtual void runTaskInQueue(std::function<void()> &&task);

        /**
         * @brief Get the Name object
         *
         * @return std::string
         */
        virtual std::string getName() const
        {
            return queueName_;
        }

        /**
         * @brief Wait until all tasks in the queue are finished.
         *
         */
        void waitAllTasksFinished();

        SerialTaskQueue() = delete;

        /**
         * @brief Construct a new Serial Task Queue object
         *
         * @param name
         */
        explicit SerialTaskQueue(const std::string &name);

        virtual ~SerialTaskQueue();

        /**
         * @brief Check whether a task is running in the queue.
         *
         * @return true
         * @return false
         */
        bool isRunningTask()
        {
            return loopThread_.getLoop()
                       ? loopThread_.getLoop()->isCallingFunctions()
                       : false;
        }

        /**
         * @brief Get the number of tasks in the queue.
         *
         */
        size_t getTaskCount();

        /**
         * @brief Stop the queue.
         *
         */
        void stop();

    protected:
        std::string queueName_;
        EventLoopThread loopThread_;
        bool stop_{false};
    };
}