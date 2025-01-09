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
    }
}