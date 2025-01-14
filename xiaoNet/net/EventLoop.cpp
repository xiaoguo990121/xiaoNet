/**
 * @file EventLoop.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include <xiaoNet/net/EventLoop.h>
#include <xiaoLog/Logger.h>
#include <xiaoLog/Date.h>

#include "Poller.h"
#include "TimerQueue.h"
#include "Channel.h"

#ifdef _WIN32
#else
#include <poll.h>
#endif
#include <iostream>
#ifdef __linux__
#include <sys/eventfd.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#endif
using namespace xiaoLog;

namespace xiaoNet
{
#ifdef __linux__
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            std::cout << "Failed in eventfd" << std::endl;
            abort();
        }

        return evtfd;
    }
    const int kPollTimeMs = 10000;
#endif
    thread_local EventLoop *t_loopInThisThread = nullptr;

    EventLoop::EventLoop()
        : looping_(false),
          threadId_(std::this_thread::get_id()),
          quit_(false),
          poller_(Poller::newPoller(this)), // 初始化poller_, 它是一个用于I/O多路复用的对象
          currentActiveChannel_(nullptr),
          eventHandling_(false),
          timerQueue_(new TimerQueue(this)), // 初始化定时器队列，负责管理定时事件
#ifdef __linux__
          wakeupFd_(createEventfd()),
          wakeupChannelPtr_(new Channel(this, wakeupFd_)), // 唤醒机制
#endif
          threadLocalLoopPtr_(&t_loopInThisThread)
    {
        LOG_DEBUG << "EventLoop constructed, wakeupFd_: " << wakeupFd_;
        if (t_loopInThisThread)
        {
            LOG_FATAL << "There is already an EventLoop in this thread";
            exit(-1);
        }
        t_loopInThisThread = this;
#ifdef __linux__
        wakeupChannelPtr_->setReadCallback(std::bind(&EventLoop::wakeupRead, this));
        wakeupChannelPtr_->enableReading();
#elif !defined _WIN32
#else
#endif
    }
#ifdef __linux__
    void EventLoop::resetTimerQueue()
    {
        assertInLoopThread();
        assert(!looping_.load(std::memory_order_acquire));
        timerQueue_->reset();
    }
#endif
    void EventLoop::resetAfterFork()
    {
        poller_->resetAfterFork();
    }
    EventLoop::~EventLoop()
    {
#ifdef _WIN32
#else
        struct timespec delay = {0, 1000000};
#endif

        quit();

        // Spin waiting for the loop to exit because
        // this may take some time to complete. We
        // assume the loop thread will *always* exit.
        // If this cannot be guaranteed then one option
        // might be to abort waiting and
        // assert(!looping_) after some delay;
        while (looping_.load(std::memory_order_acquire))
        {
#ifdef _WIN32
#else
            nanosleep(&delay, nullptr);
#endif
        }

#ifdef __linux__
        close(wakeupFd_);
#elif defined _WIN32
#else
#endif
    }

    EventLoop *EventLoop::getEventLoopOfCurrentThread()
    {
        return t_loopInThisThread;
    }
    void EventLoop::updateChannel(Channel *channel)
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        poller_->updateChannel(channel);
    }
    void EventLoop::removeChannel(Channel *channel)
    {
        assert(channel->ownerLoop() == this);
        assertInLoopThread();
        poller_->removeChannel(channel);
    }
    void EventLoop::quit()
    {
        quit_.store(true, std::memory_order_release);

        // 如果不是事件循环的线程，会调用唤醒线程
        if (!isInLoopThread())
        {
            wakeup(); // 目的是唤醒时间循环机制，因为事件循环线程可能正在等待某些事件发生，如果不唤醒它，它可能会在没有事件发生时阻塞
        }
    }

    namespace
    {
        template <typename F>
        struct ScopeExit
        {
            ScopeExit(F &&f) : f_(std::forward<F>(f))
            {
            }
            ~ScopeExit()
            {
                f_();
            }
            F f_;
        };

        template <typename F>
        ScopeExit<F> makeScopeExit(F &&f)
        {
            return ScopeExit<F>(std::forward<F>(f));
        };
    }

    void EventLoop::loop()
    {
        assert(!looping_);
        assertInLoopThread();
        looping_.store(true, std::memory_order_release);
        quit_.store(false, std::memory_order_release);

        std::exception_ptr loopException;
        try
        { // Scope where the loop flag is set
            auto loopFlagCleaner = makeScopeExit(
                [this]()
                { looping_.store(false, std::memory_order_release); });
            while (!quit_.load(std::memory_order_acquire))
            {
                activeChannels_.clear(); // 每次循环开始时，清空，用来存储当前活跃的事件通道
#ifdef __linux__
                poller_->poll(kPollTimeMs, &activeChannels_);
#else
#endif
                eventHandling_ = true;
                for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
                {
                    currentActiveChannel_ = *it;
                    currentActiveChannel_->handleEvent();
                }
                currentActiveChannel_ = nullptr;
                eventHandling_ = false;

                doRunInLoopFuncs();
            }
        }
        catch (const std::exception &e)
        {
            LOG_WARN << "Exception thrown from event loop, rethrowing after "
                        "running functions on quit: "
                     << e.what();
            loopException = std::current_exception();
        }

        Func f;
        while (funcsOnQuit_.dequeue(f))
        {
            f();
        }
        t_loopInThisThread = nullptr;
        if (loopException)
        {
            LOG_WARN << "Rethrowing exception from event loop";
            std::rethrow_exception(loopException);
        }
    }
    void EventLoop::abortNotInLoopThread()
    {
        LOG_FATAL << "It is forbidden to run loop on threads other than event-loop "
                     "thread";
        exit(1);
    }
    void EventLoop::queueInLoop(const Func &cb)
    {
        LOG_DEBUG << "EventLoop::queueInLoop called";
        funcs_.enqueue(cb);
        if (!isInLoopThread() || !looping_.load(std::memory_order_acquire))
        {
            LOG_DEBUG << " wakeup() called";
            wakeup();
        }
    }
    void EventLoop::queueInLoop(Func &&cb)
    {
        LOG_DEBUG << "EventLoop::queueInLoop called";
        funcs_.enqueue(std::move(cb));
        if (!isInLoopThread() || !looping_.load(std::memory_order_acquire))
        {
            LOG_DEBUG << " wakeup() called, isInLoopThread: " << isInLoopThread() << " looping_: " << looping_.load(std::memory_order_acquire);
            wakeup();
        }
    }

    TimerId EventLoop::runAt(const Date &time, const Func &cb)
    {
        auto microSeconds =
            time.microSecondsSinceEpoch() - Date::now().microSecondsSinceEpoch();
        std::chrono::steady_clock::time_point tp =
            std::chrono::steady_clock::now() +
            std::chrono::microseconds(microSeconds);
        return timerQueue_->addTimer(cb, tp, std::chrono::microseconds(0));
    }
    TimerId EventLoop::runAt(const Date &time, Func &&cb)
    {
        auto microSeconds =
            time.microSecondsSinceEpoch() - Date::now().microSecondsSinceEpoch();
        std::chrono::steady_clock::time_point tp =
            std::chrono::steady_clock::now() +
            std::chrono::microseconds(microSeconds);
        return timerQueue_->addTimer(std::move(cb),
                                     tp,
                                     std::chrono::microseconds(0));
    }
    TimerId EventLoop::runAfter(double delay, const Func &cb)
    {
        return runAt(Date::date().after(delay), cb);
    }
    TimerId EventLoop::runAfter(double delay, Func &&cb)
    {
        return runAt(Date::date().after(delay), std::move(cb));
    }
    TimerId EventLoop::runEvery(double interval, const Func &cb)
    {
        std::chrono::microseconds dur(
            static_cast<std::chrono::microseconds::rep>(interval * 1000000));
        auto tp = std::chrono::steady_clock::now() + dur;
        return timerQueue_->addTimer(cb, tp, dur);
    }
    TimerId EventLoop::runEvery(double interval, Func &&cb)
    {
        std::chrono::microseconds dur(
            static_cast<std::chrono::microseconds::rep>(interval * 1000000));
        auto tp = std::chrono::steady_clock::now() + dur;
        return timerQueue_->addTimer(std::move(cb), tp, dur);
    }
    void EventLoop::invalidateTimer(TimerId id)
    {
        std::cout << isRunning() << std::endl;
        if (isRunning() && timerQueue_)
            timerQueue_->invalidateTimer(id);
    }
    void EventLoop::doRunInLoopFuncs()
    {
        callingFuncs_ = true;
        {
            auto callingFlagCleaner =
                makeScopeExit([this]()
                              { callingFuncs_ = false; });
            while (!funcs_.empty())
            {
                Func func;
                while (funcs_.dequeue(func))
                {
                    func();
                }
            }
        }
    }
    void EventLoop::wakeup()
    {
        LOG_DEBUG << "wakeup called, wakeupFd_" << wakeupFd_;
        uint64_t tmp = 1;
#ifdef __linux__
        int ret = write(wakeupFd_, &tmp, sizeof(tmp));
        (void)ret;
#elif
#else
#endif
    }
    void EventLoop::wakeupRead()
    {
        LOG_DEBUG << "wakeupRead called, wakeupFd_" << wakeupFd_;
        ssize_t ret = 0;
#ifdef __linux__
        uint64_t tmp;
        ret = read(wakeupFd_, &tmp, sizeof(tmp));
#elif
#else
#endif
        if (ret < 0)
        {
            LOG_SYSERR << "wakeup read error";
        }
    }

    void EventLoop::moveToCurrentThread()
    {
        if (isRunning())
        {
            LOG_FATAL << "EventLoop cannot be moved when running";
            exit(-1);
        }
        if (isInLoopThread())
        {
            LOG_WARN << "This EventLoop is already in the current thread";
            return;
        }
        if (t_loopInThisThread)
        {
            LOG_FATAL << "There is already an EventLoop in this thread, you cannot "
                         "move another in";
            exit(-1);
        }
        *threadLocalLoopPtr_ = nullptr;
        t_loopInThisThread = this;
        threadLocalLoopPtr_ = &t_loopInThisThread;
        threadId_ = std::this_thread::get_id();
    }

    void EventLoop::runOnQuit(Func &&cb)
    {
        funcsOnQuit_.enqueue(std::move(cb));
    }

    void EventLoop::runOnQuit(const Func &cb)
    {
        funcsOnQuit_.enqueue(cb);
    }

} // namespace xiaoNet
