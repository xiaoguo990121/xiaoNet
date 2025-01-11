/**
 * @file ConcurrentTaskQueue.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once

#include <xiaoNet/utils/TaskQueue.h>
#include <xiaoNet/exports.h>
#include <queue>

namespace xiaoNet
{
    class XIAONET_EXPORT ConcurrentTaskQueue : public TaskQueue
    {
    public:
        /**
         * @brief Construct a new Concurrent Task Queue object
         *
         * @param threadNum The number of threads in the queue.
         * @param name The name of the queue
         */
        ConcurrentTaskQueue(size_t threadNum, const std::string &name);

        virtual void runTaskInQueue(const std::function<void()> &task);
        virtual void runTaskInQueue(std::function<void()> &&task);

        virtual std::string getName() const
        {
            return queueName_;
        }

        size_t getTaskCount();

        void stop();

        ~ConcurrentTaskQueue();

    private:
        size_t queueCount_;
        std::string queueName_;

        std::queue<std::function<void()>> taskQueue_;
        std::vector<std::thread> threads_;

        std::mutex taskMutex_;
        std::condition_variable taskCond_;
        std::atomic_bool stop_;
        void queueFunc(int queueNum);
    };
}