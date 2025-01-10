/**
 * @file TcpConnectionImpl.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-10
 *
 *
 */

#include "TcpConnectionImpl.h"
#include "Socket.h"
#include "Channel.h"

using namespace xiaoNet;

static inline bool isEAGAIN()
{
    if (errno == EWOULDBLOCK || errno == EAGAIN || errno == 0)
    {
        LOG_TRACE << "write buffer is full";
        return true;
    }
    else if (errno == EPIPE || errno == ECONNRESET)
    {
#ifdef _WIN32
        LOG_TRACE << "WSAENOTCONN or WSAECONNRESET, errno=" << errno;
#else
        LOG_TRACE << "EPIPE or ECONNRESET, errno=" << errno;
#endif
        LOG_TRACE << "send node in loop: return on connection closed";
        return false;
    }
    else
    {
        LOG_SYSERR << "send node in loop: return on unexpected error(" << errno << ")";
        return false;
    }
}

TcpConnectionImpl::TcpConnectionImpl(EventLoop *loop,
                                     int socketfd,
                                     const InetAddress &localAddr,
                                     const InetAddress &peerAddr,
                                     TLSPolicyPtr policy,
                                     SSLContextPtr ctx)
    : loop_(loop),
      ioChannelPtr_(new Channel(loop, socketfd)),
      socketPtr_(new Socket(socketfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr)
{
    LOG_TRACE << "new connection:" << peerAddr.toIpPort() << "->"
              << localAddr.toIpPort();
    ioChannelPtr_->setReadCallback([this]()
                                   { readCallback(); });
    ioChannelPtr_->setWriteCallback([this]()
                                    { writeCallback(); });
    ioChannelPtr_->setCloseCallback([this]()
                                    { handleClose(); });
    ioChannelPtr_->setErrorCallback([this]()
                                    { handleError(); });
    socketPtr_->setKeepAlive(true);
    name_ = localAddr.toIpPort() + "--" + peerAddr.toIpPort();

    if (policy != nullptr)
    {
        tlsProviderPtr_ =
            newTLSProvider(this, std::move(policy), std::move(ctx));
        tlsProviderPtr_->setWriteCallback(onSslWrite);
        tlsProviderPtr_->setErrorCallback(onSslError);
        tlsProviderPtr_->setHandshakeCallback(onHandshakeFinished);
        tlsProviderPtr_->setMessageCallback(onSslMessage);
        // This is triggered when peer sends a close alert
        tlsProviderPtr_->setCloseCallback(onSslCloseAlert);
    }
}

TcpConnectionImpl::~TcpConnectionImpl()
{
    std::size_t readableTlsBytes = 0;
    if (tlsProviderPtr_)
    {
        readableTlsBytes = tlsProviderPtr_->getBufferedData().readableBytes();
    }
    if (!writeBufferList_.empty())
    {
        LOG_DEBUG << "Write node list size: " << writeBufferList_.size()
                  << " first node is file? "
                  << writeBufferList_.front()->isFile()
                  << " first node is stream? "
                  << writeBufferList_.front()->isStream()
                  << " first node is async? "
                  << writeBufferList_.front()->isAsync() << " first node size: "
                  << writeBufferList_.front()->remainingBytes()
                  << " buffered TLS data size: " << readableTlsBytes;
    }
    else if (readableTlsBytes != 0)
    {
        LOG_DEBUG << "write node list size: 0 buffered TLS data size: "
                  << readableTlsBytes;
    }
    // send a close alert to peer if we are still connected
    if (tlsProviderPtr_ && status_ == ConnStatus::Connected)
        tlsProviderPtr_->close();
}

void TcpConnectionImpl::readCallback()
{
    loop_->assertInLoopThread();
    int ret = 0;

    ssize_t n = readBuffer_.readFd(socketPtr_->fd(), &ret);
    if (n == 0)
    {
        handleClose();
    }
    else if (n < 0)
    {
        if (errno == EPIPE || errno == ECONNRESET)
        {
#ifdef _WIN32
            LOG_TRACE << "WSAENOTCONN or WSAECONNRESET, errno=" << errno
                      << " fd=" << socketPtr_->fd();
#else
            LOG_TRACE << "EPIPE or ECONNRESET, errno=" << errno
                      << " fd=" << socketPtr_->fd();
#endif
            return;
        }
#ifdef _WIN32
        if (errno == WSAECONNABORTED)
        {
            LOG_TRACE << "WSAECONNABORTED, errno=" << errno;
            handleClose();
            return;
        }
#else
        if (errno == EAGAIN) // TODO: any others?
        {
            LOG_TRACE << "EAGAIN, errno=" << errno
                      << " fd=" << socketPtr_->fd();
            return;
        }
#endif
        LOG_SYSERR << "read socket error";
        handleClose();
        return;
    }
    extendLife();
    if (n > 0)
    {
        bytesReceived_ += n;
        if (tlsProviderPtr_)
        {
            tlsProviderPtr_->recvData(&readBuffer_);
        }
        else if (recvMsgCallback_)
        {
            recvMsgCallback_(shared_from_this(), &readBuffer_);
        }
    }
}
void TcpConnectionImpl::extendLife()
{
    if (idleTimeout_ > 0)
    {
        auto now = Date::date();
        if (now < lastTimingWheelUpdateTime_.after(1.0))
            return;
        lastTimingWheelUpdateTime_ = now;
        auto entry = kickoffEntry_.lock();
        if (entry)
        {
            auto timingWheelPtr = timingWheelWeakPtr_.lock();
            if (timingWheelPtr)
                timingWheelPtr->insertEntry(idleTimeout_, entry);
        }
    }
}