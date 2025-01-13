/**
 * @file TcpClient.h
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
#include <xiaoNet/net/TcpConnection.h>
#include <signal.h>

namespace xiaoNet
{
    class Connector;
    using ConnectorPtr = std::shared_ptr<Connector>;

    /**
     * @brief This class represents a TCP client
     *
     */
    class XIAONET_EXPORT TcpClient : NonCopyable,
                                     public std::enable_shared_from_this<TcpClient>
    {
    public:
        /**
         * @brief Construct a new Tcp Client object
         *
         * @param loop The event loop in which the client runs.
         * @param serverAddr The address of the server.
         * @param nameArg The name of the client.
         */
        TcpClient(EventLoop *loop,
                  const InetAddress &serverAddr,
                  const std::string &nameArg);
        ~TcpClient();

        /**
         * @brief Connect to the server.
         *
         */
        void connect();

        /**
         * @brief Disconnect from the server.
         *
         */
        void disconnect();

        /**
         * @brief Stop connecting to the server.
         *
         */
        void stop();

        /**
         * @brief Get the Tcp connection to the server.
         *
         * @return TcpConnectionPtr
         */
        TcpConnectionPtr connection() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return connection_;
        }

        /**
         * @brief Get the Loop object
         *
         * @return EventLoop*
         */
        EventLoop *getLoop() const
        {
            return loop_;
        }

        /**
         * @brief Check whether the client re-connect to the server.
         *
         * @return true
         * @return false
         */
        bool retry() const
        {
            return retry_;
        }

        /**
         * @brief Enable retrying.
         *
         */
        void enableRetry()
        {
            retry_ = true;
        }

        /**
         * @brief Get the name of the client.
         *
         * @return const std::string&
         */
        const std::string &name() const
        {
            return name_;
        }

        void setConnectionCallback(const ConnectionCallback &cb)
        {
            connectionCallback_ = cb;
        }
        void setConnectionCallback(ConnectionCallback &&cb)
        {
            connectionCallback_ = std::move(cb);
        }

        /**
         * @brief Set the Connection Error Callback object
         *
         * @param cb
         */
        void setConnectionErrorCallback(const ConnectionErrorCallback &cb)
        {
            connectionErrorCallback_ = cb;
        }

        /**
         * @brief Set the message callback.
         *
         * @param cb The callback is called when some data is received from the
         * server.
         */
        void setMessageCallback(const RecvMessageCallback &cb)
        {
            messageCallback_ = cb;
        }
        void setMessageCallback(RecvMessageCallback &&cb)
        {
            messageCallback_ = std::move(cb);
        }

        /// Set write complete callback.
        /// Not thread safe.

        /**
         * @brief Set the write complete callback.
         *
         * @param cb The callback is called when data to send is written to the
         * socket.
         */
        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            writeCompleteCallback_ = cb;
        }
        void setWriteCompleteCallback(WriteCompleteCallback &&cb)
        {
            writeCompleteCallback_ = std::move(cb);
        }

        /**
         * @brief Set the callback for errors of SSL
         * @param cb The callback is called when an SSL error occurs.
         */
        void setSSLErrorCallback(const SSLErrorCallback &cb)
        {
            sslErrorCallback_ = cb;
        }
        void setSSLErrorCallback(SSLErrorCallback &&cb)
        {
            sslErrorCallback_ = std::move(cb);
        }

        /**
         * @brief Set the callback for set socket option
         * @param cb The callback is called, before connect
         */
        void setSockOptCallback(const SockOptCallback &cb);
        void setSockOptCallback(SockOptCallback &&cb);

        /**
         * @brief Enable SSL encryption.
         */
        void enableSSL(TLSPolicyPtr policy)
        {
            tlsPolicyPtr_ = std::move(policy);
            sslContextPtr_ = newSSLContext(*tlsPolicyPtr_, false);
        }

    private:
        /// Not thread safe, but in loop
        void newConnection(int sockfd);
        /// Not thread safe, but in loop
        void removeConnection(const TcpConnectionPtr &conn);

        EventLoop *loop_;
        ConnectorPtr connector_; // avoid revealing Connector
        const std::string name_;
        ConnectionCallback connectionCallback_;
        ConnectionErrorCallback connectionErrorCallback_;
        RecvMessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        SSLErrorCallback sslErrorCallback_;
        std::atomic_bool retry_;
        std::atomic_bool connect_;
        // always in loop thread
        mutable std::mutex mutex_;
        TcpConnectionPtr connection_;
        TLSPolicyPtr tlsPolicyPtr_;
        SSLContextPtr sslContextPtr_;
        bool validateCert_{false};

#ifndef _WIN32
        class IgnoreSigPipe
        {
        public:
            IgnoreSigPipe()
            {
                ::signal(SIGPIPE, SIG_IGN);
            }
        };
        static IgnoreSigPipe initObj;
#endif
    };
}