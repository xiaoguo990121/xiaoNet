/**
 * @file AsyncStream.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-07
 *
 *
 */
#pragma once

#include <xiaoNet/utils/NonCopyable.h>
#include <memory>
#include <string>

namespace xiaoNet
{
    /**
     * @brief This class represents a data stream that can be sent asynchronously.
     * The data is sent in chunks, and the chunks are sent in order, and all the
     * chunks are sent continuously.
     */
    class XIAONET_EXPORT AsyncStream : public NonCopyable
    {
    public:
        virtual ~AsyncStream() = default;

        /**
         * @brief Send data asynchronously.
         *
         * @param data
         * @param len
         * @return true
         * @return false
         */
        virtual bool send(const char *data, size_t len) = 0;
        bool send(const std::string &data)
        {
            return send(data.data(), data.length());
        }
        /**
         * @brief Terminate the stream.
         *
         */
        virtual void close() = 0;
    };
    using AsyncStreamPtr = std::unique_ptr<AsyncStream>;
}