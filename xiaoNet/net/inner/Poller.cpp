/**
 * @file Poller.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include "Poller.h"
#ifdef __linux__
#include "poller/EpollPoller.h"
#elif defined _WIN32
#elif defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
#else
#endif

using namespace xiaoNet;
Poller *Poller::newPoller(EventLoop *loop)
{
#if defined __linux__ || defined _WIN32
    return new EpollPoller(loop);
#elif
#else
#endif
}