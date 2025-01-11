/**
 * @file SerialTaskQueue.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include <xiaoNet/utils/SerialTaskQueue.h>
#include <xiaoLog/Logger.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace xiaoNet
{
    SerialTaskQueue::SerialTaskQueue(const std::string &name)
        : queueName_(name.empty() ? "SerialTaskQueue" : name),
          loopThread_(queueName_)
    {
        loopThread_.run();
    }
    void SerialTaskQueue::stop()
    {
        stop_ = true;
        loopThread_.getLoop()->quit();
        loopThread_.wait();
    }
    SerialTaskQueue::~SerialTaskQueue()
    {
        if (!stop_)
            stop();
        LOG_TRACE << "destruct SerialTaskQueue(" << queueName_ << ")";
    }
    void SerialTaskQueue::runTaskInQueue(const std::function<void()> &task)
    {
        loopThread_.getLoop()->runInLoop(task);
    }
    void SerialTaskQueue::runTaskInQueue(std::function<void()> &&task)
    {
        loopThread_.getLoop()->runInLoop(std::move(task));
    }

    void SerialTaskQueue::waitAllTasksFinished()
    {
        syncTaskInQueue([]() {

        });
    }
}