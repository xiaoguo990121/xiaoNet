/**
 * @file TcpClient.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-12
 *
 *
 */

#include <xiaoNet/net/TcpClient.h>
#include <xiaoNet/net/inner/TLSProvider.h>

#include "inner/TcpConnectionImpl.h"

#include "Connector.h"
#include "Socket.h"

using namespace xiaoNet;
using namespace std::placeholders;

namespace xiaoNet
{
#ifndef _WIN32
    TcpClient::IgnoreSigPipe TcpClient::initObj;
#endif

    static void defaultConnectionCallback(const TcpConnectionPtr &conn)
    {
        LOG_TRACE << conn->localAddr().toIpPort() << " -> "
                  << conn->peerAddr().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN");
        // do not call conn-forceClose(), because some users want to register
        // message callback only.
    }

    static void defaultMessageCallback(const TcpConnectionPtr &, MsgBuffer *buf)
    {
        buf->retrieveAll();
    }
}

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr, false)),
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true)
{
    (void)validateCert_;
    LOG_TRACE << "TcpClient::TcpClient[" << name_ << "] - connector ";
}

TcpClient::~TcpClient()
{
    LOG_TRACE << "TcpClient::~TcpClient[" << name_ << "] - connector ";
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_ == nullptr)
    {
        connector_->stop();
        return;
    }
    assert(loop_ == connection_->getLoop());
    auto conn =
        std::atomic_load_explicit(&connection_, std::memory_order_relaxed);
    loop_->runInLoop([conn = std::move(conn)]()
                     { conn->setCloseCallback([](const TcpConnectionPtr &connPtr) mutable
                                              { connPtr->getLoop()->queueInLoop(
                                                    [connPtr]
                                                    { connPtr->connectDestroyed(); }); }); });
    connection_->forceClose();
}

void TcpClient::connect()
{
    LOG_TRACE << "TcpClietn::connect[" << name_ << "] = connecting to "
              << connector_->serverAddress().toIpPort();
    auto weakPtr = std::weak_ptr<TcpClient>(shared_from_this());
    connector_->setNewConnectionCallback([weakPtr](int sockfd)
                                         {
        auto ptr = weakPtr.lock();
        if(ptr)
        {
            ptr->newConnection(sockfd);
        } });
    connector_->setErrorCallback([weakPtr]()
                                 {
         auto ptr = weakPtr.lock();
        if (ptr && ptr->connectionErrorCallback_)
        {
            ptr->connectionErrorCallback_();
        } });
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::setSockOptCallback(SockOptCallback &&cb)
{
    connector_->setSockOptCallback(std::move(cb));
}

void TcpClient::setSockOptCallback(const SockOptCallback &cb)
{
    connector_->setSockOptCallback(cb);
}

void TcpClient::newConnection(int sockfd)
{
    LOG_DEBUG << "newConnection called";
    loop_->assertInLoopThread();
    InetAddress peerAddr(Socket::getPeerAddr(sockfd));
    InetAddress localAddr(Socket::getLocalAddr(sockfd));

    TcpConnectionPtr conn;
    LOG_TRACE << "SSL enabeld: " << (tlsPolicyPtr_ ? "true" : "false");
    if (tlsPolicyPtr_)
    {
        assert(sslContextPtr_);
        conn = std::make_shared<TcpConnectionImpl>(
            loop_, sockfd, localAddr, peerAddr, tlsPolicyPtr_, sslContextPtr_);
    }
    else
    {
        conn = std::make_shared<TcpConnectionImpl>(loop_,
                                                   sockfd, localAddr, peerAddr);
    }
    conn->setConnectionCallback(connectionCallback_);
    conn->setRecvMsgCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    std::weak_ptr<TcpClient> weakSelf(shared_from_this());
    auto closeCb = std::function<void(const TcpConnectionPtr &)>(
        [weakSelf](const TcpConnectionPtr &c)
        {
            if (auto self = weakSelf.lock())
            {
                self->removeConnection(c);
            }
            // Else the TcpClient instance has already been destroyed
            else
            {
                LOG_TRACE << "TcpClient::removeConnection was skipped because "
                             "TcpClient instanced already freed";
                c->getLoop()->queueInLoop([c]
                                          { c->connectDestroyed(); });
            }
        });
    conn->setCloseCallback(std::move(closeCb));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->setSSLErrorCallback([weakSelf = std::move(weakSelf)](SSLError err)
                              {
        auto self = weakSelf.lock();
        if (self && self->sslErrorCallback_)
            self->sslErrorCallback_(err); });
    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop([conn]()
                       { conn->connectDestroyed(); });
    if (retry_ && connect_)
    {
        LOG_TRACE << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                  << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}
