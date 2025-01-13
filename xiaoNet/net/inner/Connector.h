/**
 * @file Connector.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-12
 *
 *
 */

#pragma once

#include <xiaoNet/net/EventLoop.h>
#include <xiaoNet/net/InetAddress.h>
#include <memory>

namespace xiaoNet
{
    class Connector : public NonCopyable,
                      public std::enable_shared_from_this<Connector>
    {
    public:
        using NewConnectionCallback = std::function<void(int sockfd)>;
        using ConnectionErrorCallback = std::function<void()>;
        using SockOptCallback = std::function<void(int sockfd)>;
        Connector(EventLoop *loop, const InetAddress &addr, bool retry = true);
        Connector(EventLoop *loop, InetAddress &&addr, bool retry = true);
        ~Connector();
        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_ = cb;
        }
        void setNewConnectionCallback(NewConnectionCallback &&cb)
        {
            newConnectionCallback_ = std::move(cb);
        }
        void setErrorCallback(const ConnectionErrorCallback &cb)
        {
            errorCallback_ = cb;
        }
        void setErrorCallback(ConnectionErrorCallback &&cb)
        {
            errorCallback_ = std::move(cb);
        }
        void setSockOptCallback(const SockOptCallback &cb)
        {
            sockOptCallback_ = cb;
        }
        void setSockOptCallback(SockOptCallback &&cb)
        {
            sockOptCallback_ = std::move(cb);
        }
        const InetAddress &serverAddress() const
        {
            return serverAddr_;
        }
        void start();
        void restart();
        void stop();

    private:
        NewConnectionCallback newConnectionCallback_;
        ConnectionErrorCallback errorCallback_;
        SockOptCallback sockOptCallback_;
        enum class Status
        {
            Disconnected,
            Connecting,
            Connected
        };
        static constexpr int kMaxRetryDelayMs = 30 * 1000;
        static constexpr int kInitRetryDelayMs = 500;
        std::shared_ptr<Channel> channelPtr_;
        EventLoop *loop_;
        InetAddress serverAddr_;

        std::atomic_bool connect_{false};
        std::atomic<Status> status_{Status::Disconnected};

        int retryInterval_{kInitRetryDelayMs};
        int maxRetryInterval_{kMaxRetryDelayMs};

        bool retry_;
        bool socketHanded_{false};
        int fd_{-1};

        void startInLoop();
        void connect();
        void connecting(int sockfd);
        int removeAndResetChannel();
        void handleWrite();
        void handleError();
        void retry(int sockfd);
    };
}