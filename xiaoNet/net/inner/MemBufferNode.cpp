/**
 * @file MemBufferNode.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include <xiaoNet/net/inner/BufferNode.h>

namespace xiaoNet
{
    class MemBufferNode : public BufferNode
    {
    public:
        MemBufferNode() = default;

        void getData(const char *&data, size_t &len) override
        {
            data = buffer_.peek();
            len = buffer_.readableBytes();
        }
        void retrieve(size_t len) override
        {
            buffer_.retrieve(len);
        }
        long long remainingBytes() const override
        {
            if (isDone_)
                return 0;
            return static_cast<long long>(buffer_.readableBytes());
        }
        void append(const char *data, size_t len) override
        {
            buffer_.append(data, len);
        }

    private:
        xiaoNet::MsgBuffer buffer_;
    };
    BufferNodePtr BufferNode::newMemBufferNode()
    {
        return std::make_shared<MemBufferNode>();
    }
}