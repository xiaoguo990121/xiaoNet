/**
 * @file EventLoop.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */

#pragma once
#include <xiaoNet/utils/NonCopyable.h>
#include <xiaoLog/Date.h>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

namespace xiaoNet
{
    class Poller;
    class TimerQueue;
    class Channel;
    using ChannelList = std::vector<Channel *>;
    using Func = std::function<void()>;
    using TimerId = uint64_t;
    enum
    {
        InvalidTimerId = 0
    };

    /**
     * @brief As the name implies, this class represents an event loop that runs in
     * a perticular thread. The event loop can handle network I/O events and timers
     * in asynchronous mode.
     * @note An event loop object always belongs to a separate thread, and there is
     * one event loop object at most in a thread. We can call an event loop object
     * the event loop of the thread it belongs to, or call that thread the thread of
     * teh event loop.
     *
     */
    class XIAONET_EXPORT EventLoop : NonCopyable
    {
    public:
        EventLoop();
        ~EventLoop();

        /**
         * @brief Run the event loop. This method will be blocked until the event
         * loop exits.
         *
         */
        void loop();

        /**
         * @brief Let the event loop quit.
         *
         */
        void quit();

        /**
         * @brief Assertion that the current thread is the thread to which the event
         * loop belongs. If the assertion fails, the program aborts.
         *
         */
        void assertInLoopThread()
        {
            if (!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        };

#ifdef __linux__
        /**
         * @brief Make the timer queue woks after calling the fork() function.
         *
         */
        void resetTimerQueue();
#endif
        /**
         * @brief Make the event loop works after calling the fork() function.
         *
         */
        void resetAfterFork();

        /**
         * @brief Return true if the current thread is the thread to which the event
         * loop belongs.
         *
         * @return true
         * @return false
         */
        bool isInLoopThread() const
        {
            return threadId_ == std::this_thread::get_id();
        }

        /**
         * @brief Get the Event Loop Of Current Thread object. Return nullptr if there
         * is no event loop in the current thread.
         *
         * @return EventLoop*
         */
        static EventLoop *getEventLoopOfCurrentThread();

        /**
         * @brief Run the function f in the thread of the event loop.
         *
         * @tparam Functor
         * @param f
         */
        template <typename Functor>
        inline void runInLoop(Functor &&f)
        {
            if (isInLoopThread())
            {
                f();
            }
            else
            {
                queueInLoop(std::forward<Functor>(f));
            }
        }

        /**
         * @brief Run the function f in the thread of the event loop.
         *
         * @param f
         * @note The difference between this method and the runInLoop() method is
         * that the function f is executed after the method exiting no matter if the
         * current thread is the thread of the event loop.
         */
        void queueInLoop(const Func &f);
        void queueInLoop(Func &&f);

        /**
         * @brief Run a function at a time point.
         *
         * @param time The time to run the function.
         * @param cb The function to run.
         * @return TimerId The ID of the timer.
         */
        TimerId runAt(const xiaoLog::Date &time, const Func &cb);
        TimerId runAt(const xiaoLog::Date &time, Func &&cb);

        /**
         * @brief Run a function after a period of time.
         *
         * @param delay
         * @param cb
         * @return TimerId
         */
        TimerId runAfter(double delay, const Func &cb);
        TimerId runAfter(double delay, Func &&cb);

        /**
         * @brief Run a function after a period of time.
         * @note Users could use chrono literals to represent a time duration
         * For example:
         * @code
           runAfter(5s, task);
           runAfter(10min, task);
           @endcode
         */
        TimerId runAfter(const std::chrono::duration<double> &delay, const Func &cb)
        {
            return runAfter(delay.count(), cb);
        }
        TimerId runAfter(const std::chrono::duration<double> &delay, Func &&cb)
        {
            return runAfter(delay.count(), std::move(cb));
        }

        /**
         * @brief Repeatedly run a function every period of time.
         *
         * @param interval The duration in seconds.
         * @param cb The function to run.
         * @return TimerId
         */
        TimerId runEvery(double interval, const Func &cb);
        TimerId runEvery(double interval, Func &&cb);

        /**
         * @brief Repeatedly run a function every period of time.
         * Users could use chrono literals to represent a time duration
         * For example:
         * @code
           runEvery(5s, task);
           runEvery(10min, task);
           runEvery(0.1h, task);
           @endcode
         */
        TimerId runEvery(const std::chrono::duration<double> &interval,
                         const Func &cb)
        {
            return runEvery(interval.count(), cb);
        }
        TimerId runEvery(const std::chrono::duration<double> &interval, Func &&cb)
        {
            return runEvery(interval.count(), std::move(cb));
        }

        /**
         * @brief Invalidate the timer identified by the given ID.
         *
         * @param id
         */
        void invalidateTimer(TimerId id);

        /**
         * @brief Move the EventLoop to the current thread, this method must be
         * called before the loop is running.
         *
         */
        void moveToCurrentThread();

        /**
         * @brief Update channel status. This method is usually used internally.
         *
         * @param chl
         */
        void updateChannel(Channel *chl);

        /**
         * @brief Remove a channel from the event loop. This method is usually used
         * internally.
         *
         * @param chl
         */
        void removeChannel(Channel *chl);

        /**
         * @brief Return the index of the event loop.
         *
         * @return size_t
         */
        size_t index()
        {
            return index_;
        }

        /**
         * @brief Set the Index object
         *
         * @param index
         */
        void setIndex(size_t index)
        {
            index_ = index;
        }

        /**
         * @brief Return true if the event loop is running.
         *
         * @return true
         * @return false
         */
        bool isRunning()
        {
            return looping_.load(std::memory_order_acquire) &&
                   (!quit_.load(std::memory_order_acquire));
        }

        /**
         * @brief Check if the event loop is calling a function.
         *
         * @return true
         * @return false
         */
        bool isCallingFunctions()
        {
            return callingFuncs_;
        }

        /**
         * @brief Run functions when the event loop quits
         *
         * @param cb
         */
        void runOnQuit(Func &&cb);
        void runOnQuit(const Func &cb);

    private:
        void abortNotInLoopThread();
        std::atomic<bool> looping_;
        std::thread::id threadId_;
        std::atomic<bool> quit_;
        std::unique_ptr<Poller> poller_;

        bool callingFuncs_{false};

#ifdef _WIN32
        size_t index_{size_t(-1)};
#else
        size_t index_{std::numeric_limits<size_t>::max()};
#endif
    };
}