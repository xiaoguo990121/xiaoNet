/**
 * @file EventLoopThread.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */

#pragma once
#include <xiaoNet/net/EventLoop.h>
#include <memory>
#include <mutex>
#include <future>

namespace xiaoNet
{
    class XIAONET_EXPORT EventLoopThread : NonCopyable
    {
    public:
        explicit EventLoopThread(const std::string &threadName = "EventLoopThread");
        ~EventLoopThread();

        /**
         * @brief Wait for the event loop to exit.
         * @note This method blocks the current thread until the event loop exits.
         */
        void wait();

        /**
         * @brief Get the Loop object
         *
         * @return EventLoop*
         */
        EventLoop *getLoop() const
        {
            return loop_.get();
        }

        /**
         * @brief Run the event loop of the thread. This method doesn't block the
         * current thread.
         *
         */
        void run();

    private:
        std::shared_ptr<EventLoop> loop_;
        std::mutex loopMutex_;

        std::string loopThreadName_;
        void loopFuncs();
        std::promise<std::shared_ptr<EventLoop>> promiseForLoopPointer_;
        std::promise<int> promiseForRun_;
        std::promise<int> promiseForLoop_;
        std::once_flag once_;
        std::thread thread_;
    };
}
