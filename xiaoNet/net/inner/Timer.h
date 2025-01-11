/**
 * @file Timer.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once

#include <xiaoNet/utils/NonCopyable.h>
#include <xiaoNet/net/callbacks.h>
#include <chrono>

namespace xiaoNet
{
    using TimerId = uint64_t;
    using TimePoint = std::chrono::steady_clock::time_point;
    using TimeInterval = std::chrono::microseconds;
    class Timer : public NonCopyable
    {
    public:
        Timer(const TimerCallback &cb,
              const TimePoint &when,
              const TimeInterval &interval);
        Timer(TimerCallback &&cb,
              const TimePoint &when,
              const TimeInterval &interval);
        ~Timer()
        {
        }
        void run() const;
        void restart(const TimePoint &now);
        bool operator<(const Timer &t) const;
        bool operator>(const Timer &t) const;
        const TimePoint &when() const
        {
            return when_;
        }
        bool isRepeat()
        {
            return repeat_;
        }
        TimerId id()
        {
            return id_;
        }

    private:
        TimerCallback callback_;
        TimePoint when_;
        const TimeInterval interval_;
        const bool repeat_;
        const TimerId id_;
        static std::atomic<TimerId> timersCreated_;
    };
}