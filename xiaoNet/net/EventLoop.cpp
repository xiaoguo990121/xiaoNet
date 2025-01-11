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

#ifdef _WIN32
#else
#include <poll.h>
#endif
#include <iostream>
#ifdef __linux__
#include <sys/eventfd.h>
#endif

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
          poller_(Poller::newPoller(this)),

} // namespace xiaoNet
