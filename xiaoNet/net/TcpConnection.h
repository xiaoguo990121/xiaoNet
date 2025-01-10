/**
 * @file TcpConnection.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-09
 *
 *
 */

#pragma once
#include <xiaoNet/exports.h>
#include <xiaoNet/net/EventLoop.h>
#include <xiaoNet/net/callbacks.h>
#include <xiaoNet/net/TLSPolicy.h>
#include <xiaoNet/net/AsyncStream.h>
#include <xiaoNet/net/Certificate.h>
#include <xiaoNet/net/InetAddress.h>

#include <memory>

namespace xiaoNet
{
    class TimingWheel;

    struct SSLContext;
    using SSLContextPtr = std::shared_ptr<SSLContext>;

    /**
     * @brief This class represents a TCP connection.
     *
     */
    class XIAONET_EXPORT TcpConnection
    {
    public:
        friend class TcpServer;
        friend class TcpConnectionImpl;
        friend class TcpClient;

        TcpConnection() = default;
        virtual ~TcpConnection() {};

        /**
         * @brief Send some data to the peer.
         *
         * @param msg
         * @param len
         */
        virtual void send(const char *msg, size_t len) = 0;
        virtual void send(const void *msg, size_t len) = 0;
        virtual void send(const std::string &msg) = 0;
        virtual void send(std::string &&msg) = 0;
        virtual void send(const MsgBuffer &buffer) = 0;
        virtual void send(MsgBuffer &&buffer) = 0;
        virtual void send(const std::shared_ptr<std::string> &msgPtr) = 0;
        virtual void send(const std::shared_ptr<MsgBuffer> &msgPtr) = 0;

        /**
         * @brief Send a file to the peer.
         *
         * @param fileName
         * @param offset
         * @param length
         */
        virtual void sendFile(const char *fileName,
                              long long offset = 0,
                              long long length = 0) = 0;

        /**
         * @brief Send a file to the peer.
         *
         * @param fileName
         * @param offset
         * @param length
         */
        virtual void sendFile(const wchar_t *fileName,
                              long long offset = 0,
                              long long length = 0) = 0;

        /**
         * @brief Send a stream to the peer.
         *
         * @param callback function to retrieve teh stream data (stream ends when a
         * zero size is returned) the callback will be called with nullptr when the
         * send is finished/interrupted, so that it cleans up any internal data (ex:
         * close file).
         * @warning The buffer size should be >= 10 to allow http chunked-encoding data
         * stream
         */
        virtual void sendStream(std::function<std::size_t(char *, std::size_t)>
                                    callback) = 0;

        /**
         * @brief Send a stream to the peer asynchronously.
         *
         * @param disableKickoff Disable the kickoff mechanism. If this parameter is
         * enabled, the connection will not be closed after the inactive timeout.
         * @note The subsequent data sent after the async stream will be sent after
         * the stream is closed.
         * @return AsyncStreamPtr
         */
        virtual AsyncStreamPtr sendAsyncStream(bool disableKickoff = false) = 0;

        /**
         * @brief Get the local address of the connection.
         *
         * @return const InetAddress&
         */
        virtual const InetAddress &localAddr() const = 0;

        /**
         * @brief Get the remote address of the connection.
         *
         * @return const InetAddress&
         */
        virtual const InetAddress &peerAddr() const = 0;

        /**
         * @brief Return true if the connection is established.
         *
         * @return true
         * @return false
         */
        virtual bool connected() const = 0;

        /**
         * @brief Return false if the connection is established.
         *
         * @return true
         * @return false
         */
        virtual bool disconnected() const = 0;

        /**
         * @brief Set the High Water Mark Callback
         *
         * @param cb The callback is called when the data in sending buffer is
         * larger than the water mark.
         * @param markLen
         */
        virtual void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                              size_t markLen) = 0;

        /**
         * @brief Set the TCP_NODELAY option to the socket.
         *
         * @param on
         */
        virtual void setTcpNoDelay(bool on) = 0;

        /**
         * @brief Shutdown the connection.
         * @note This method only closes the writing direction.
         *
         */
        virtual void shutdown() = 0;

        /**
         * @brief Close the connection forcefully.
         *
         */
        virtual void forceClose() = 0;

        /**
         * @brief Get the event loop in which the connection I/O is handled.
         *
         * @return EventLoop*
         */
        virtual EventLoop *getLoop() = 0;

        /**
         * @brief Set the custom data on the connection.
         *
         * @param context
         */
        void setContext(const std::shared_ptr<void> &context)
        {
            contextPtr_ = context;
        }
        void setContext(std::shared_ptr<void> &&context)
        {
            contextPtr_ = std::move(context);
        }
        virtual std::string applicationProtocol() const = 0;

        /**
         * @brief Get the custom data from the connection.
         *
         * @tparam T
         * @return std::shared_ptr<T>
         */
        template <typename T>
        std::shared_ptr<T> getContext() const
        {
            return std::static_pointer_cast<T>(contextPtr_);
        }

        /**
         * @brief Return true if the custom data is set by user.
         *
         * @return true
         * @return false
         */
        bool hasContext() const
        {
            return (bool)contextPtr_;
        }

        /**
         * @brief Clear the custom data.
         *
         */
        void clearContext()
        {
            contextPtr_.reset();
        }

        /**
         * @brief Call this method to avoid being kicked off by TcpServer, refer to
         * the kickoffIdleConnection method in the TcpServer class.
         *
         */
        virtual void keepAlive() = 0;

        /**
         * @brief Return true if the keepAlive() method is called.
         *
         * @return true
         * @return false
         */
        virtual bool isKeepAlive() = 0;

        /**
         * @brief Return the number of bytes sent
         *
         * @return size_t
         */
        virtual size_t bytesSent() const = 0;

        /**
         * @brief Return the number of bytes received.
         *
         * @return size_t
         */
        virtual size_t bytesReceived() const = 0;

        /**
         * @brief Check whether the connection is SSL encrypted.
         *
         * @return true
         * @return false
         */
        virtual bool isSSLConnection() const = 0;

        /**
         * @brief Get buffer of unprompted data.
         *
         * @return MsgBuffer*
         */
        virtual MsgBuffer *getRecvBuffer() = 0;

        /**
         * @brief Get peer certificate (if any).
         *
         * @return CertificatePtr
         */
        virtual CertificatePtr peerCertificate() const = 0;

        /**
         * @brief Get the SNI name (for server connections only)
         *
         * @return std::string
         */
        virtual std::string sniName() const = 0;

        /**
         * @brief Start TLS. If the connection is specified as a server, the
         * connection will be upgraded to a TLS server connection. If the connection
         * is specified as a client, the connection will be upgraded to a TLS client
         * @note This method is only availabel for non-SSL connections.
         *
         * @param policy
         * @param isServer
         * @param upgradeCallback
         */
        virtual void startEncryption(TLSPolicyPtr policy,
                                     bool isServer,
                                     std::function<void(const TcpConnectionPtr &)> upgradeCallback = nullptr) = 0;

        void setValidationPolicy(TLSPolicy &&policy)
        {
            tlsPolicy_ = std::move(policy);
        }
        void setRecvMsgCallback(const RecvMessageCallback &cb)
        {
            recvMsgCallback_ = cb;
        }
        void setRecvMsgCallback(RecvMessageCallback &&cb)
        {
            recvMsgCallback_ = std::move(cb);
        }
        void setConnectionCallback(const ConnectionCallback &cb)
        {
            connectionCallback_ = cb;
        }
        void setConnectionCallback(ConnectionCallback &&cb)
        {
            connectionCallback_ = std::move(cb);
        }
        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            writeCompleteCallback_ = cb;
        }
        void setWriteCompleteCallback(WriteCompleteCallback &&cb)
        {
            writeCompleteCallback_ = std::move(cb);
        }
        void setCloseCallback(const CloseCallback &cb)
        {
            closeCallback_ = cb;
        }
        void setCloseCallback(CloseCallback &&cb)
        {
            closeCallback_ = std::move(cb);
        }
        void setSSLErrorCallback(const SSLErrorCallback &cb)
        {
            sslErrorCallback_ = cb;
        }
        void setSSLErrorCallback(SSLErrorCallback &&cb)
        {
            sslErrorCallback_ = std::move(cb);
        }

        virtual void connectEstablished() = 0;
        virtual void connectDestroyed() = 0;
        virtual void enableKickingOff(
            size_t timeout,
            const std::shared_ptr<TimingWheel> &timingWheel) = 0;

    protected:
        RecvMessageCallback recvMsgCallback_;
        ConnectionCallback connectionCallback_;
        CloseCallback closeCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        SSLErrorCallback sslErrorCallback_;
        TLSPolicy tlsPolicy_;

    private:
        std::shared_ptr<void> contextPtr_;
    };
    XIAONET_EXPORT SSLContextPtr newSSLContext(const TLSPolicy &policy,
                                               bool server);
}