/**
 * @file ConcurrentTaskQueue.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include <xiaoNet/utils/ConcurrentTaskQueue.h>
#include <xiaoLog/Logger.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
using namespace xiaoNet;
using namespace xiaoLog;

ConcurrentTaskQueue::ConcurrentTaskQueue(size_t threadNum,
                                         const std::string &name)
    : queueCount_(threadNum), queueName_(name), stop_(false)
{
    assert(threadNum > 0);
    for (size_t i = 0; i < queueCount_; ++i)
    {
        threads_.push_back(
            std::thread(std::bind(&ConcurrentTaskQueue::queueFunc, this, i)));
    }
}
void ConcurrentTaskQueue::runTaskInQueue(const std::function<void()> &task)
{
    LOG_TRACE << "copy task into queue";
    std::lock_guard<std::mutex> lock(taskMutex_);
    taskQueue_.push(task);
    taskCond_.notify_one();
}
void ConcurrentTaskQueue::runTaskInQueue(std::function<void()> &&task)
{
    LOG_TRACE << "move task into queue";
    std::lock_guard<std::mutex> lock(taskMutex_);
    taskQueue_.push(std::move(task));
    taskCond_.notify_one();
}
void ConcurrentTaskQueue::queueFunc(int queueNum)
{
    char tmpName[32];
    snprintf(tmpName, sizeof(tmpName), "%s%d", queueName_.c_str(), queueNum);
#ifdef __linux__
    ::prctl(PR_SET_NAME, tmpName);
#endif
    while (!stop_)
    {
        std::function<void()> r;
        {
            std::unique_lock<std::mutex> lock(taskMutex_);
            while (!stop_ && taskQueue_.size() == 0)
            {
                taskCond_.wait(lock);
            }
            if (taskQueue_.size() > 0)
            {
                LOG_TRACE << "got a new task!";
                r = std::move(taskQueue_.front());
                taskQueue_.pop();
            }
            else
                continue;
        }
        r();
    }
}

size_t ConcurrentTaskQueue::getTaskCount()
{
    std::lock_guard<std::mutex> guard(taskMutex_);
    return taskQueue_.size();
}

void ConcurrentTaskQueue::stop()
{
    if (!stop_)
    {
        stop_ = true;
        taskCond_.notify_all();
        for (auto &t : threads_)
            t.join();
    }
}
ConcurrentTaskQueue::~ConcurrentTaskQueue()
{
    stop();
}