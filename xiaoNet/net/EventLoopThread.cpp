/**
 * @file EventLoopThread.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */
#include <xiaoNet/net/EventLoopThread.h>

using namespace xiaoNet;

EventLoopThread::EventLoopThread(const std::string &threadName)
    : loop_(nullptr),
      loopThreadName_(threadName),
      thread_([this]()
              { loopFuncs(); })
{
    auto f = promiseForLoopPointer_.get_future();
    loop_ = f.get();
}

EventLoopThread::~EventLoopThread()
{
    run();
    std::shared_ptr<EventLoop> loop;
    {
        std::unique_lock<std::mutex> lk(loopMutex_);
        loop = loop_;
    }
    if (loop)
    {
        loop->quit();
    }
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void EventLoopThread::wait()
{
    thread_.join();
}

void EventLoopThread::loopFuncs()
{
#ifdef __linux__
#endif
    thread_local static std::shared_ptr<EventLoop> loop =
        std::make_shared<EventLoop>();
    loop->queueInLoop([this]()
                      { promiseForLoop_.set_value(1); });
    promiseForLoopPointer_.set_value(loop);
    auto f = promiseForRun_.get_future();
    (void)f.get();
    loop->loop();
    {
        std::unique_lock<std::mutex> lk(loopMutex_);
        loop_ = nullptr;
    }
}

void EventLoopThread::run()
{
    std::call_once(once_, [this]()
                   {
        auto f = promiseForLoop_.get_future();
        promiseForRun_.set_value(1);
        (void)f.get(); });
}