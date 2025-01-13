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
#include <xiaoNet/net/EventLoopThreadPool.h>
#include <xiaoLog/Logger.h>
#include <csignal>
#include <set>
#include <map>

using namespace xiaoLog;
namespace xiaoNet
{
    class Acceptor;

    /**
     * @brief This class represents a TCP server.
     *
     */
    class XIAONET_EXPORT TcpServer : NonCopyable
    {
    public:
        /**
         * @brief Construct a new Tcp Server object
         *
         * @param loop
         * @param address
         * @param name
         * @param reUseAddr
         * @param reUsePort
         */
        TcpServer(EventLoop *loop,
                  const InetAddress &address,
                  std::string name,
                  bool reUseAddr = true,
                  bool reUsePort = true);
        ~TcpServer();

        /**
         * @brief Start the server.
         *
         */
        void start();

        /**
         * @brief Stop the server.
         *
         */
        void stop();

        /**
         * @brief Set the number of event loops in which the I/O of connections to
         * the server is handled.
         * An EventLoopThreadPool is created and managed by TcpServer.
         *
         * @param num
         */
        void setIoLoopNum(size_t num)
        {
            assert(!started_);
            loopPoolPtr_ = std::make_shared<EventLoopThreadPool>(num);
            loopPoolPtr_->start();
            ioLoops_ = loopPoolPtr_->getLoops();
            numIoLoops_ = ioLoops_.size();
        }

        /**
         * @brief Set the event loops pool in which the I/O of connection to
         * the server is handled.
         * A shared_ptr of EventLoopThreadPool is copied.
         *
         * @param pool
         */
        void setIoLoopThreadPool(const std::shared_ptr<EventLoopThreadPool> &pool)
        {
            assert(pool->size() > 0);
            assert(!started_);
            loopPoolPtr_ = pool;
            loopPoolPtr_->start();
            ioLoops_ = loopPoolPtr_->getLoops();
            numIoLoops_ = ioLoops_.size();
        }

        /**
         * @brief Set the event loops in which the I/O of connections to
         * the server is handled.
         * The loops are managed by caller. Caller should ensure that ioLoops
         * lives longer than TcpServer.
         *
         * @param ioLoops
         */
        void setIoLoops(const std::vector<xiaoNet::EventLoop *> &ioLoops)
        {
            assert(!ioLoops.empty());
            assert(!started_);
            ioLoops_ = ioLoops;
            numIoLoops_ = ioLoops_.size();
            loopPoolPtr_.reset();
        }

        /**
         * @brief Set the Recv Message Callback object
         *
         * @param cb The callback is called when some data is received on a
         * connection to the server.
         */
        void setRecvMessageCallback(const RecvMessageCallback &cb)
        {
            recvMessageCallback_ = cb;
        }
        void setRecvMessageCallback(RecvMessageCallback &&cb)
        {
            recvMessageCallback_ = std::move(cb);
        }

        /**
         * @brief Set the Connection Callback object
         *
         * @param cb The callback is called when a connection is established or
         * closed.
         */
        void setConnectionCallback(const ConnectionCallback &cb)
        {
            connectionCallback_ = cb;
        }
        void setConnectionCallback(ConnectionCallback &&cb)
        {
            connectionCallback_ = std::move(cb);
        }

        /**
         * @brief Set the write complete callback.
         *
         * @param cb The callback is called when data to send is written to the
         * socket of a connection.
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
         * @brief Set the before listen setsockopt callback.
         *
         * @param cb This callback will be called before the listen
         */
        void setBeforeListenSockOptCallback(SockOptCallback cb);
        /**
         * @brief Set the after accept setsockopt callback.
         *
         * @param cb This callback will be called after accept
         */
        void setAfterAcceptSockOptCallback(SockOptCallback cb);

        /**
         * @brief Get the name of the server.
         *
         * @return const std::string&
         */
        const std::string &name() const
        {
            return serverName_;
        }

        /**
         * @brief Get the IP and port string of the server.
         *
         * @return std::string
         */
        std::string ipPort() const;

        /**
         * @brief Get the address of the server.
         *
         * @return const xiaoNet::InetAddress&
         */
        const xiaoNet::InetAddress &address() const;

        /**
         * @brief Get the event loop of the server.
         *
         * @return EventLoop*
         */
        EventLoop *getLoop() const
        {
            return loop_;
        }

        /**
         * @brief Get the I/O event loops of the server.
         *
         * @return std::vector<EventLoop *>
         */
        std::vector<EventLoop *> getIoLoops() const
        {
            return ioLoops_;
        }

        /**
         * @brief An idle connection is a connection that has no read or write, kick
         * off it after timeout seoncds.
         *
         * @param timeout
         */
        void kickoffIdleConnections(size_t timeout)
        {
            loop_->runInLoop([this, timeout]()
                             {
                assert(!started_);
                idleTimeout_ = timeout; });
        }

        /**
         * @brief enable SSL encryption.
         *
         * @param policy
         */
        void enableSSL(TLSPolicyPtr policy)
        {
            policyPtr_ = std::move(policy);
            sslContextPtr_ = newSSLContext(*policyPtr_, true);
        }

        /**
         * @brief Reload the SSL context.
         * @note Call this function when the certificate or private key is updated.
         * The server will reload the SSL context and use the new certificate and
         * private key. new connection will use the new SSL context.
         */
        void reloadSSL();

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

        // 'loopPoolPtr_' may and may not hold the internal thread pool.
        // We should not access it directly in codes.
        // Insstead, we should use its delegation variable 'ioLoops_'.
        std::shared_ptr<EventLoopThreadPool> loopPoolPtr_;
        // If one of 'setIoLoopNum()', 'setIoLoopThreadPool()' and 'setIoLoops()' is
        // called, 'ioLoops_' will hold the loops passed in.
        // Otherwise, it should contain only one element, which is 'loop_'
        std::vector<EventLoop *> ioLoops_;
        size_t nextLoopIdx_{0};
        size_t numIoLoops_{0};

#ifndef _WIN32
        class IgnoreSigPipe
        {
        public:
            IgnoreSigPipe()
            {
                ::signal(SIGPIPE, SIG_IGN);
                LOG_TRACE << "Ignore SIGPIPE";
            }
        };
        IgnoreSigPipe initObj;
#endif
        bool started_{false};
        TLSPolicyPtr policyPtr_{nullptr};
        SSLContextPtr sslContextPtr_{nullptr};
    };
}