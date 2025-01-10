/**
 * @file Acceptor.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-10
 *
 *
 */

#pragma once

#include <xiaoNet/net/EventLoop.h>
#include <xiaoNet/utils/NonCopyable.h>
#include "Socket.h"
#include <xiaoNet/net/InetAddress.h>
#include "Channel.h"
#include <functional>

namespace xiaoNet
{
    using NewConnectionCallback = std::function<void(int fd, const InetAddress &)>;
    using AcceptorSockOptCallback = std::function<void(int)>;
    class Acceptor : NonCopyable
    {
    public:
        Acceptor(EventLoop *loop,
                 const InetAddress &addr,
                 bool reUseAddr = true,
                 bool reUsePort = true);
        ~Acceptor();
        const InetAddress &addr() const
        {
            return addr_;
        }
        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_ = cb;
        }
        void listen();

        void setBeforeListenSockOptCallback(AcceptorSockOptCallback cb)
        {
            beforeListenSetSockOptCallback_ = std::move(cb);
        }

        void setAfterAcceptSockOptCallback(AcceptorSockOptCallback cb)
        {
            afterAcceptSetSockOptCallback_ = std::move(cb);
        }

    protected:
#ifndef _WIN32
        int idleFd_;
#endif
        Socket sock_;
        InetAddress addr_;
        EventLoop *loop_;
        NewConnectionCallback newConnectionCallback_;
        Channel acceptChannel_;
        void readCallback();
        AcceptorSockOptCallback beforeListenSetSockOptCallback_;
        AcceptorSockOptCallback afterAcceptSetSockOptCallback_;
    };
}
