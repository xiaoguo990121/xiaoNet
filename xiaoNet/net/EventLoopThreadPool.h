/**
 * @file EventLoopThreadPool.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-09
 *
 *
 */

#pragma once

#include <xiaoNet/net/EventLoopThread.h>
#include <xiaoNet/exports.h>

namespace xiaoNet
{
    /**
     * @brief This class represents a pool of EventLoopThread objects
     *
     */
    class XIAONET_EXPORT EventLoopThreadPool : NonCopyable
    {
    public:
        EventLoopThreadPool() = delete;

        /**
         * @brief Construct a new Event Loop Thread Pool object
         *
         * @param threadNum
         * @param name
         */
        EventLoopThreadPool(size_t threadNum,
                            const std::string &name = "EventLoopThreadPool");

        /**
         * @brief Run all event loops in the pool.
         * @note This function doesn't block the current thread.
         */
        void start();

        /**
         * @brief Wait for all event loops in teh pool to quit.
         * @note This function blocks the current thread.
         */
        void wait();

        /**
         * @brief Return the number of the event loop.
         *
         * @return size_t
         */
        size_t size()
        {
            return loopThreadVector_.size();
        }

        /**
         * @brief Get the Next Loop object in the pool.
         *
         * @return EventLoop*
         */
        EventLoop *getNextLoop();

        /**
         * @brief Get the Loop object
         *
         * @param id
         * @return EventLoop*
         */
        EventLoop *getLoop(size_t id);

        /**
         * @brief Get the Loops object
         *
         * @return std::vector<EventLoop *>
         */
        std::vector<EventLoop *> getLoops() const;

    private:
        std::vector<std::shared_ptr<EventLoopThread>> loopThreadVector_;
        std::atomic<size_t> loopIndex_{0};
    };
}