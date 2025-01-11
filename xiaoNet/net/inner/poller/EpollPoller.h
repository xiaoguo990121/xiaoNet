/**
 * @file EpollPoller.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once

#include "../Poller.h"
#include <xiaoNet/utils/NonCopyable.h>
#include <xiaoNet/net/EventLoop.h>

#if defined __linux__ || defined _WIN32
#include <memory>
#include <map>
using EventList = std::vector<struct epoll_event>;
#endif
namespace xiaoNet
{
    class Channel;

    class EpollPoller : public Poller
    {
    public:
        explicit EpollPoller(EventLoop *loop);
        virtual ~EpollPoller();
        virtual void poll(int timeoutMs, ChannelList *activeChannels) override;
        virtual void updateChannel(Channel *channel) override;
        virtual void removeChannel(Channel *Channel) override;
#ifdef _WIN32
        virtual void postEvent(uint64_t event) override;
        virtual void setEventCallback(const EventCallback &cb) override
        {
            eventCallback_ = cb;
        }
#endif

    private:
#if defined __linux__ || defined _WIN32
        static const int kInitEventListSize = 16;
#ifdef _WIN32
#else
        int epollfd_;
#endif
        EventList events_;
        void update(int operation, Channel *channel);
#ifndef NDEBUG
        using ChannelMap = std::map<int, Channel *>;
        ChannelMap channels_;
#endif
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
#endif
    };
}