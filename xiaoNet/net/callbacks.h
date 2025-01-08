/**
 * @file callbacks.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */
#pragma once

#include <functional>
#include <memory>

namespace xiaoNet
{
    enum class SSLError
    {
        kSSLHandshakeError,
        kSSLInvalidCertificate,
        kSSLProtocolError
    };
    using TimerCallback = std::function<void()>;

    class TcpConnection;
    class MsgBuffer;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using RecvMessageCallback =
        std::function<void(const TcpConnectionPtr &, MsgBuffer *)>;
    using ConnectionErrorCallback = std::function<void()>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
    using HighWaterMarkCallback =
        std::function<void(const TcpConnectionPtr &, const size_t)>;
    using SSLErrorCallback = std::function<void(SSLError)>;
    using SockOptCallback = std::function<void(int)>;
}