/**
 * @file TcpServer.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-10
 *
 *
 */

#include <xiaoNet/net/TcpServer.h>
#include <xiaoLog/Logger.h>
#include <xiaoNet/utils/MsgBuffer.h>
#include "Acceptor.h"
#include <functional>
#include "inner/TcpConnectionImpl.h"

using namespace xiaoNet;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &address,
                     std::string name,
                     bool reUseAddr,
                     bool reUsePort)
    : loop_(loop),
      acceptorPtr_(new Acceptor(loop, address, reUseAddr, reUsePort)),
      serverName_(std::move(name)),
      recvMessageCallback_([](const TcpConnectionPtr &, MsgBuffer *buffer)
                           {
          LOG_ERROR << "unhandled recv message [" << buffer->readableBytes()
                    << " bytes]";
          buffer->retrieveAll(); }),
      ioLoops_({loop}),
      numIoLoops_(1)
{
  acceptorPtr_->setNewConnectionCallback(
      [this](int fd, const InetAddress &peer)
      { newConnection(fd, peer); });
}

TcpServer::~TcpServer()
{
  LOG_TRACE << "TcpServer::~TcpServer [" << serverName_ << "] destructing";
}

void TcpServer::setBeforeListenSockOptCallback(SockOptCallback cb)
{
  acceptorPtr_->setBeforeListenSockOptCallback(std::move(cb));
}

void TcpServer::setAfterAcceptSockOptCallback(SockOptCallback cb)
{
  acceptorPtr_->setAfterAcceptSockOptCallback(std::move(cb));
}

void TcpServer::newConnection(int sockfd, const InetAddress &peer)
{
  LOG_TRACE << "new connection:fd=" << sockfd
            << " address=" << peer.toIpPort();
  loop_->assertInLoopThread();
  EventLoop *ioLoop = ioLoops_[nextLoopIdx_];
  if (++nextLoopIdx_ >= numIoLoops_)
  {
    nextLoopIdx_ = 0;
  }
  TcpConnectionPtr newPtr;
  if (policyPtr_)
  {
    assert(sslContextPtr_);
    newPtr = std::make_shared<TcpConnectionImpl>(
        ioLoop,
        sockfd,
        InetAddress(Socket::getLocalAddr(sockfd)),
        peer,
        policyPtr_,
        sslContextPtr_);
  }
  else
  {
    newPtr = std::make_shared<TcpConnectionImpl>(
        ioLoop, sockfd, InetAddress(Socket::getLocalAddr(sockfd)), peer);
  }

  if (idleTimeout_ > 0)
  {
    assert(timingWheelMap_[ioLoop]);
    newPtr->enableKickingOff(idleTimeout_, timingWheelMap_[ioLoop]);
  }

  newPtr->setRecvMsgCallback(recvMessageCallback_);

  newPtr->setConnectionCallback(
      [this](const TcpConnectionPtr &connectionPtr)
      {
        if (connectionCallback_)
          connectionCallback_(connectionPtr);
      });
  newPtr->setWriteCompleteCallback(
      [this](const TcpConnectionPtr &connectionPtr)
      {
        if (writeCompleteCallback_)
          writeCompleteCallback_(connectionPtr);
      });

  newPtr->setCloseCallback([this](const TcpConnectionPtr &closeConnPtr)
                           { connectionClosed(closeConnPtr); });

  connSet_.insert(newPtr);
  newPtr->connectEstablished();
}

void TcpServer::start()
{
  loop_->runInLoop([this]()
                   {
        assert(!started_);
        started_ = true;
        if (idleTimeout_ > 0)
        {
            for (EventLoop *loop : ioLoops_)
            {
                timingWheelMap_[loop] =
                    std::make_shared<TimingWheel>(loop,
                                                  idleTimeout_,
                                                  1.0F,
                                                  idleTimeout_ < 500
                                                      ? idleTimeout_ + 1
                                                      : 100);
            }
        }
        LOG_TRACE << "map size=" << timingWheelMap_.size();
        acceptorPtr_->listen(); });
}

void TcpServer::stop()
{
  if (loop_->isInLoopThread())
  {
    acceptorPtr_.reset();
    // copy the connSet_ to a vector, use the vector to close the
    // connections to avoid the iterator invalidation.
    std::vector<TcpConnectionPtr> connPtrs;
    connPtrs.reserve(connSet_.size());
    for (auto &conn : connSet_)
    {
      connPtrs.push_back(conn);
    }
    for (auto &connection : connPtrs)
    {
      connection->forceClose();
    }
  }
  else
  {
    std::promise<void> pro;
    auto f = pro.get_future();
    loop_->queueInLoop([this, &pro]()
                       {
            acceptorPtr_.reset();
            std::vector<TcpConnectionPtr> connPtrs;
            connPtrs.reserve(connSet_.size());
            for (auto &conn : connSet_)
            {
                connPtrs.push_back(conn);
            }
            for (auto &connection : connPtrs)
            {
                connection->forceClose();
            } });
    f.get();
  }
  loopPoolPtr_.reset();
  for (auto &iter : timingWheelMap_)
  {
    std::promise<void> pro;
    auto f = pro.get_future();
    iter.second->getLoop()->runInLoop([&iter, &pro]() mutable
                                      {
            iter.second.reset();
            pro.set_value(); });
    f.get();
  }
}

void TcpServer::handleCloseInLoop(const TcpConnectionPtr &connectionPtr)
{
  size_t n = connSet_.erase(connectionPtr);
  (void)n;
  assert(n == 1);
  auto connLoop = connectionPtr->getLoop();

  // NOTE: always queue this operation in connLoop, because this connection
  // may be in loop_'s current active channels, waiting to be processed.
  // If connectDestroyed() is called here, we will be using and wild pointer
  // later.
  connLoop->queueInLoop(
      [connectionPtr]()
      { connectionPtr->connectDestroyed(); });
}
void TcpServer::connectionClosed(const TcpConnectionPtr &connectionPtr)
{
  LOG_TRACE << "connectionClosed";
  if (loop_->isInLoopThread())
  {
    handleCloseInLoop(connectionPtr);
  }
  else
  {
    loop_->queueInLoop(
        [this, connectionPtr]()
        { handleCloseInLoop(connectionPtr); });
  }
}

std::string TcpServer::ipPort() const
{
  return acceptorPtr_->addr().toIpPort();
}

const xiaoNet::InetAddress &TcpServer::address() const
{
  return acceptorPtr_->addr();
}

void TcpServer::reloadSSL()
{
  if (loop_->isInLoopThread())
  {
    if (policyPtr_)
    {
      sslContextPtr_ = newSSLContext(*policyPtr_, true);
    }
  }
  else
  {
    loop_->queueInLoop([this]()
                       {
            if (policyPtr_)
            {
                sslContextPtr_ = newSSLContext(*policyPtr_, true);
            } });
  }
}
