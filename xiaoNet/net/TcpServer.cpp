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
                           { LOG_ERROR << "unhandled recv message [" << buffer->readableBytes()
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