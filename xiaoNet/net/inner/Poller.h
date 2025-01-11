/**
 * @file Poller.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once
#include "NonCopyable.h"
#include "EventLoop.h"

#include <memory>
#include <map>

namespace xiaoNet
{
    class Channel;
#ifdef _WIN32
    using EventCallback = std::function<void(uint64_t)>;
#endif
    class Poller : NonCopyable
    {
    public:
        explicit Poller(EventLoop *loop) : ownerLoop_(loop) {}
        virtual ~Poller() {}
        void assertInLoopThread()
        {
            ownerLoop_->assertInLoopThread();
        }
        virtual void poll(int timeoutMs, ChannelList *activeChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        virtual void removeChannel(Channel *channel) = 0;
#ifdef _WIN32
        virtual void postEvent(uint64_t event) = 0;
        virtual void setEventCallback(const EventCallback &cb) = 0;
#endif
        virtual void resetAfterFork()
        {
        }
        static Poller *newPoller(EventLoop *loop);

    private:
        EventLoop *ownerLoop_;
    };
}