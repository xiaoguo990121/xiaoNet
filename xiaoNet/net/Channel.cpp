/**
 * @file Channel.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */

#include "Channel.h"
#include <xiaoNet/net/EventLoop.h>

#ifdef _WIN32

#else
#include <poll.h>
#endif

namespace xiaoNet
{
    const int Channel::kNoneEvent = 0;

    const int Channel::kReadEvent = POLLIN | POLLPRI;
    const int Channel::kWriteEvent = POLLOUT;

    Channel::Channel(EventLoop *loop, int fd)
        : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
    {
    }

    void Channel::remove()
    {
        assert(events_ == kNoneEvent);
        addedToLoop_ = false;
        loop_->removeChannel(this);
    }

    void Channel::update()
    {
        loop_->updateChannel(this);
    }

    void Channel::handleEvent()
    {
        if (events_ == kNoneEvent)
            return;
        if (tied_)
        {
            std::shared_ptr<void> guard = tie_.lock();
            if (guard)
            {
                handleEventSafely();
            }
        }
        else
        {
            handleEventSafely();
        }
    }
    void Channel::handleEventSafely()
    {
        if (eventCallback_)
        {
            eventCallback_();
            return;
        }
        if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
        {
            if (closeCallback_)
                closeCallback_();
        }
        if (revents_ & (POLLNVAL | POLLERR))
        {
            if (errorCallback_)
                errorCallback_();
        }
#ifdef __linux__
        if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
#else
#endif
        {
            if (readCallback_)
                readCallback_();
        }
#ifdef _WIN32
#else
        if (revents_ & POLLOUT)
#endif
        {
            if (writeCallback_)
                writeCallback_();
        }
    }
}