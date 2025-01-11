/**
 * @file LockFreeQueue.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once
#include <xiaoNet/utils/NonCopyable.h>
#include <atomic>

namespace xiaoNet
{
    /**
     * @brief This class template represents a lock-free multiple producers single
     * consumer queue
     *
     * @tparam T The type of the items in the queue.
     */
    template <typename T>
    class MpscQueue : public NonCopyable
    {
    public:
        MpscQueue()
            : head_(new BufferNode), tail_(head_.load(std::memory_order_relaxed))
        {
        }
        ~MpscQueue()
        {
            T output;
            while (this->dequeue(output))
            {
            }
            BufferNode *front = head_.load(std::memory_order_relaxed);
            delete front;
        }

        /**
         * @brief Put a item into the queue.
         *
         * @param input
         * @note This method can be called in multiple threads.
         */
        void enqueue(T &&input)
        {
            BufferNode *node{new BufferNode(std::move(input))};
            BufferNode *prevhead{head_.exchange(node, std::memory_order_acq_rel)}; // 将head变量的值交换为node，并返回交换前的原值
            // 为什么使用acq_rel， rel是为了确保在更新之前的依赖数据已经写入内存
            // acquire是为了保证当前线程exchange后的操作不会被重排到exchange之前

            prevhead->next_.store(node, std::memory_order_release);
        }
        void enqueue(const T &input)
        {
            BufferNode *node{new BufferNode(input)};
            BufferNode *prevhead{head_.exchange(node, std::memory_order_acq_rel)};
            prevhead->next_.store(node, std::memory_order_release);
        }

        /**
         * @brief Get a item from the queue.
         *
         * @param output
         * @return true
         * @return false
         */
        bool dequeue(T &output)
        {
            BufferNode *tail = tail_.load(std::memory_order_relaxed);
            BufferNode *next = tail_->next_.load(std::memory_order_acquire);

            if (next == nullptr)
            {
                return false;
            }
            output = std::move(*(next->dataPtr_));
            delete next->dataPtr_;
            tail_.store(next, std::memory_order_release);
            delete tail;
            return true;
        }

        bool empty()
        {
            BufferNode *tail = tail_.load(std::memory_order_relaxed);
            BufferNode *next = tail_->next_.load(std::memory_order_acquire);
            return next == nullptr;
        }

    private:
        struct BufferNode
        {
            BufferNode() = default;
            BufferNode(const T &data) : dataPtr_(new T(data))
            {
            }
            BufferNode(T &&data) : dataPtr_(new T(std::move(data)))
            {
            }
            T *dataPtr_;                              // 指向实际数据的指针，用于存储队列中的元素
            std::atomic<BufferNode *> next_{nullptr}; // 指向队列中的下一个节点
        };

        std::atomic<BufferNode *> head_; // 用于指向队列的头部和尾部，分别用于入队和出队操作
        std::atomic<BufferNode *> tail_; // head_用于插入新节点，tail_用于从队列中移除节点
    };
}