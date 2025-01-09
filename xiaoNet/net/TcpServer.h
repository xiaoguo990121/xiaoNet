/**
 * @file TcpServer.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-09
 *
 *
 */

#pragma once
#include <xiaoNet/exports.h>
#include <xiaoNet/utils/NonCopyable.h>
#include <xiaoNet/net/callbacks.h>
#include <xiaoNet/net/InetAddress.h>
#include <xiaoNet/net/EventLoop.h>
#include <xiaoNet/net/TcpConnection.h>
#include <set>
#include <map>

namespace xiaoNet
{
    class Acceptor;

    /**
     * @brief This class represents a TCP server.
     *
     */
    class XIAONET_EXPORT TcpServer : NonCopyable
    {

    private:
        void handleCloseInLoop(const TcpConnectionPtr &connectionPtr);
        void newConnection(int fd, const InetAddress &peer);
        void connectionClosed(const TcpConnectionPtr &connectionPtr);

        EventLoop *loop_;
        std::unique_ptr<Acceptor> acceptorPtr_;
        std::string serverName_;
        std::set<TcpConnectionPtr> connSet_;

        RecvMessageCallback recvMessageCallback_;
        ConnectionCallback connectionCallback_;
        WriteCompleteCallback writeCompleteCallback_;

        size_t idleTimeout_{0};
        std::map<EventLoop *, std::shared_ptr<TimingWheel>> timingWheelMap_;

        std::shared_ptr<EventLoopThreadPool> loopPoolPtr_;

        std::vector<EventLoop *> ioLoops_;
        size_t nextLoopIdx_{0};
        size_t numIoLoops_{0};
    };
}