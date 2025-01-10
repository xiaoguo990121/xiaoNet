/**
 * @file BufferNode.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-10
 *
 *
 */

#pragma once
#ifdef _WIN32
#include <stdio.h>
#endif
#include <xiaoNet/utils/MsgBuffer.h>
#include <xiaoNet/utils/NonCopyable.h>
#include <xiaoLog/Logger.h>
#include <memory>
#include <functional>

using namespace xiaoLog;
namespace xiaoNet
{
    class BufferNode;
    using BufferNodePtr = std::shared_ptr<BufferNode>;
    using StreamCallback = std::function<std::size_t(char *, std::size_t)>;
    class BufferNode : public NonCopyable
    {
    public:
        virtual bool isFile() const
        {
            return false;
        }
        virtual ~BufferNode() = default;
        virtual bool isStream() const
        {
            return false;
        }
        virtual void getData(const char *&data, size_t &len) = 0;
        virtual void append(const char *, size_t)
        {
            LOG_FATAL << "Not a memeory buffer node";
        }
        virtual void retrieve(size_t len) = 0;
        virtual long long remainingBytes() const = 0;
        virtual int getFd() const
        {
            LOG_FATAL << "Not a file buffer node";
        }
        virtual bool available() const
        {
            return true;
        }
        virtual bool isAsync() const
        {
            return false;
        }

        void done()
        {
            isDone_ = true;
        }
        static BufferNodePtr newMemBufferNode();

        static BufferNodePtr newStreamBufferNode(StreamCallback &&cb);
#ifdef _WIN32
        static BufferNodePtr newFileBufferNode(const wchar_t *fileName,
                                               long long offset,
                                               long long length);
#else
        static BufferNodePtr newFileBufferNode(const char *fileName,
                                               long long offset,
                                               long long length);
#endif
        static BufferNodePtr newAsyncStreamBufferNode();

    protected:
        bool isDone_{false};
    };
}
