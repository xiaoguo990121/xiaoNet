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
        LOG_DEBUG << "handleEvent called, fd: " << fd_ << ", events: " << events_;
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
        LOG_DEBUG << "handleEventSafely called, fd: " << fd_ << ", revents: " << revents_;
        // 检查是否存在事件回调
        if (eventCallback_)
        {
            eventCallback_();
            return;
        }
        // 处理挂起事件
        if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
        {
            if (closeCallback_)
                closeCallback_();
        }
        // 处理无效事件或错误事件
        if (revents_ & (POLLNVAL | POLLERR))
        {
            if (errorCallback_)
                errorCallback_();
        }
        // 处理可读事件
#ifdef __linux__
        if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
#else
        if (revents_ & (POLLIN | POLLPRI))
#endif
        {
            if (readCallback_)
            {
                readCallback_();
            }
        }
        // 处理可写事件
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