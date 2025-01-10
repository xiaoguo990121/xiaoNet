/**
 * @file TimingWheel.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-10
 *
 *
 */

#include <xiaoNet/utils/TimingWheel.h>

using namespace xiaoNet;
using namespace xiaoLog;

TimingWheel::TimingWheel(xiaoNet::EventLoop *loop,
                         size_t maxTimeout,
                         float ticksInterval,
                         size_t bucketsNumPerWheel)
    : loop_(loop),
      ticksInterval_(ticksInterval),
      bucketsNumPerWheel_(bucketsNumPerWheel)
{
    assert(maxTimeout > 1);
    assert(ticksInterval > 0);
    assert(bucketsNumPerWheel_ > 1);
    size_t maxTickNum = static_cast<size_t>(maxTimeout / ticksInterval);
    auto ticksNum = bucketsNumPerWheel;
    wheelsNum_ = 1;
    while (maxTickNum > ticksNum)
    {
        ++wheelsNum_;
        ticksNum *= bucketsNumPerWheel_;
    }
    wheels_.resize(wheelsNum_);
    for (size_t i = 0; i < wheelsNum_; ++i)
    {
        wheels_[i].resize(bucketsNumPerWheel_);
    }
    timerId_ = loop->runEvery(ticksInterval_, [this]()
                              {
        ++ticksCounter_;
        size_t t = ticksCounter_;
        size_t pow = 1;
        for(size_t i = 0; i < wheelsNum_; ++i)
        {
            if((t % pow) == 0)
            {
                EntryBucket tmp;
                {
                    wheels_[i].front().swap(tmp);
                    wheels_[i].pop_front();
                    wheels_[i].push_back(EntryBucket());
                }
            }
            pow = pow * bucketsNumPerWheel_;
        } });
}

TimingWheel::~TimingWheel()
{
    loop_->assertInLoopThread();
    loop_->invalidateTimer(timerId_);
    for (auto iter = wheels_.rbegin(); iter != wheels_.rend(); ++iter)
    {
        iter->clear();
    }
    LOG_TRACE << "TimingWheel destruct!";
}

void TimingWheel::insertEntry(size_t delay, EntryPtr entryPtr)
{
    if (delay < 0)
        return;
    if (!entryPtr)
        return;
    if (loop_->isInLoopThread())
    {
        insertEntryInloop(delay, entryPtr);
    }
    else
    {
        loop_->runInLoop(
            [this, delay, entryPtr]()
            { insertEntryInloop(delay, entryPtr); });
    }
}

void TimingWheel::insertEntryInloop(size_t delay, EntryPtr entryPtr)
{
    loop_->assertInLoopThread();

    delay = static_cast<size_t>(delay / ticksInterval_ + 1);
    size_t t = ticksCounter_;
    for (size_t i = 0; i < wheelsNum_; ++i)
    {
        if (delay <= bucketsNumPerWheel_)
        {
            wheels_[i][delay - 1].insert(entryPtr);
            break;
        }
        if (i < (wheelsNum_ - 1))
        {
            entryPtr = std::make_shared<CallbackEntry>(
                [this, delay, i, t, entryPtr]()
                {
                    if (delay > 0)
                    {
                        wheels_[i][(delay + (t % bucketsNumPerWheel_) - 1) %
                                   bucketsNumPerWheel_]
                            .insert(entryPtr);
                    }
                });
        }
        else
        {
            wheels_[i][bucketsNumPerWheel_ - 1].insert(entryPtr);
        }
        delay = (delay + (t % bucketsNumPerWheel_) - 1) / bucketsNumPerWheel_;
        t = t / bucketsNumPerWheel_;
    }
}