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
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

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
void TcpConnectionImpl::writeCallback()
{
    loop_->assertInLoopThread();
    if (ioChannelPtr_->isWriting())
    {
        if (tlsProviderPtr_)
        {
            bool sentAll = tlsProviderPtr_->sendBufferedData();
            if (!sentAll)
            {
                return;
            }
        }
        while (!writeBufferList_.empty())
        {
            auto &nodePtr = writeBufferList_.front();
            if (nodePtr->remainingBytes() == 0)
            {
                if (!nodePtr->isAsync() || !nodePtr->available())
                {
                    writeBufferList_.pop_front();
                }
                else
                {
                    ioChannelPtr_->disableWriting();
                    return;
                }
            }
            else
            {
                auto n = sendNodeInLoop(nodePtr);
                if (nodePtr->remainingBytes() > 0 || n < 0)
                    return;
            }
        }
        assert(writeBufferList_.empty());
        if (tlsProviderPtr_ == nullptr ||
            tlsProviderPtr_->getBufferedData().readableBytes() == 0)
        {
            ioChannelPtr_->disableWriting();
            if (closeOnEmpty_)
            {
                shutdown();
            }
        }
    }
    else
    {
        LOG_SYSERR << "no writing but write callbcak called";
    }
}
void TcpConnectionImpl::connectEstablished()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]()
                     {
        LOG_TRACE << "connectEstablished";
        assert(thisPtr->status_ == ConnStatus::Connecting);
        thisPtr->ioChannelPtr_->tie(thisPtr);
        thisPtr->ioChannelPtr_->enableReading();
        thisPtr->status_ = ConnStatus::Connected;

        if(thisPtr->tlsProviderPtr_)
            thisPtr->tlsProviderPtr_->startEncryption();
        else if(thisPtr->connectionCallback_)
            thisPtr->connectionCallback_(thisPtr); });
}
void TcpConnectionImpl::handleClose()
{
    LOG_TRACE << "connection closed, fd=" << socketPtr_->fd();
    LOG_TRACE << "write buffer size: " << writeBufferList_.size();
    if (!writeBufferList_.empty())
    {
        LOG_TRACE << writeBufferList_.front()->isFile();
        LOG_TRACE << writeBufferList_.front()->isStream();
    }
    loop_->assertInLoopThread();
    status_ = ConnStatus::Disconnected;
    ioChannelPtr_->disableAll();
    auto guardThis = shared_from_this();
    if (connectionCallback_)
        connectionCallback_(guardThis);
    if (closeCallback_)
    {
        LOG_TRACE << "to call close callback";
        closeCallback_(guardThis);
    }
}
void TcpConnectionImpl::handleError()
{
    int err = socketPtr_->getSocketError();
    if (err == 0)
        return;
    if (err == EPIPE ||
#ifndef _WIN32
        err == EBADMSG ||
#endif
        err == ECONNRESET)
    {
        LOG_TRACE << "[" << name_ << "] - SO_ERROR = " << err << " "
                  << strerror(err);
    }
    else
    {
        LOG_ERROR << "[" << name_ << "] - SO_ERROR = " << err << " "
                  << strerror(err);
    }
}
void TcpConnectionImpl::setTcpNoDelay(bool on)
{
    socketPtr_->setTcpNoDelay(on);
}
void TcpConnectionImpl::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (status_ == ConnStatus::Connected)
    {
        status_ = ConnStatus::Disconnected;
        ioChannelPtr_->disableAll();

        connectionCallback_(shared_from_this());
    }
    ioChannelPtr_->remove();
}
void TcpConnectionImpl::shutdown()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]()
                     {
        if(thisPtr->status_ == ConnStatus::Connected)
        {
            if(thisPtr->tlsProviderPtr_)
            {
                // there's still data to be sent, so we can't close the
                // connection just yet
                if(thisPtr->tlsProviderPtr_->getBufferedData()
                .readableBytes() != 0 || 
                !thisPtr->writeBufferList_.empty())
                {
                    thisPtr->closeOnEmpty_ = true;
                    return;
                }
                thisPtr->tlsProviderPtr_->close();
            }
            if(thisPtr->tlsProviderPtr_ == nullptr &&
            !thisPtr->writeBufferList_.empty())
            {
                thisPtr->closeOnEmpty_ - true;
                return;
            }
            thisPtr->status_ = ConnStatus::Disconnecting;
            if(!thisPtr->ioChannelPtr_->isWriting())
            {
                thisPtr->socketPtr_->closeWrite();
            }
        } });
}

void TcpConnectionImpl::forceClose()
{
    auto thisPtr = shared_from_this();
    loop_->runInLoop([thisPtr]()
                     {
        if(thisPtr->status_ == ConnStatus::Connected ||
        thisPtr->status_ == ConnStatus::Disconnecting)
        {
            thisPtr->status_ = ConnStatus::Disconnecting;
            thisPtr->handleClose();

            if(thisPtr->tlsProviderPtr_)
                thisPtr->tlsProviderPtr_->close();
        } });
}
#ifndef _WIN32
void TcpConnectionImpl::sendInLoop(const void *buffer, size_t length)
#else
void TcpConnectionImpl::sendInLoop(const char *buffer, size_t length)
#endif
{
    loop_->assertInLoopThread();
    if (status_ != ConnStatus::Connected)
    {
        LOG_DEBUG << "Connection is not connected,give up sending";
        return;
    }
    ssize_t sendLen = 0;
    if (!ioChannelPtr_->isWriting() && writeBufferList_.empty())
    {
        // send directly
        sendLen = writeInLoop(buffer, length);
        if (sendLen < 0)
        {
            LOG_TRACE << "write error";
            return;
        }
        length -= sendLen;
    }
    if (length > 0 && status_ == ConnStatus::Connected)
    {
        if (writeBufferList_.empty() || writeBufferList_.back()->isFile() ||
            writeBufferList_.back()->isStream())
        {
            writeBufferList_.push_back(BufferNode::newMemBufferNode());
        }
        writeBufferList_.back()->append(static_cast<const char *>(buffer) +
                                            sendLen,
                                        length);
        if (highWaterMarkCallback_ &&
            writeBufferList_.back()->remainingBytes() >
                static_cast<long long>(highWaterMarkLen_))
        {
            highWaterMarkCallback_(shared_from_this(),
                                   writeBufferList_.back()->remainingBytes());
        }
        if (highWaterMarkCallback_ && tlsProviderPtr_ &&
            tlsProviderPtr_->getBufferedData().readableBytes() >
                highWaterMarkLen_)
        {
            highWaterMarkCallback_(
                shared_from_this(),
                tlsProviderPtr_->getBufferedData().readableBytes());
        }
    }
}
// The order of data sending should be same as the order of calls of send()
void TcpConnectionImpl::send(const std::shared_ptr<std::string> &msgPtr)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(msgPtr->data(), msgPtr->length());
    }
    else
    {
        loop_->queueInLoop([thisPtr = shared_from_this(), msgPtr]()
                           { thisPtr->sendInLoop(msgPtr->data(), msgPtr->length()); });
    }
}
// The order of data sending should be same as the order of calls of send()
void TcpConnectionImpl::send(const std::shared_ptr<MsgBuffer> &msgPtr)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(msgPtr->peek(), msgPtr->readableBytes());
    }
    else
    {
        loop_->queueInLoop([thisPtr = shared_from_this(), msgPtr]()
                           { thisPtr->sendInLoop(msgPtr->peek(), msgPtr->readableBytes()); });
    }
}

void TcpConnectionImpl::send(const char *msg, size_t len)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(msg, len);
    }
    else
    {
        auto buffer = std::make_shared<std::string>(msg, len);
        loop_->queueInLoop(
            [thisPtr = shared_from_this(), buffer = std::move(buffer)]()
            {
                thisPtr->sendInLoop(buffer->data(), buffer->length());
            });
    }
}
void TcpConnectionImpl::send(const void *msg, size_t len)
{
    if (loop_->isInLoopThread())
    {
#ifndef _WIN32
        sendInLoop(msg, len);
#else

#endif
    }
    else
    {
        auto buffer =
            std::make_shared<std::string>(static_cast<const char *>(msg), len);
        loop_->queueInLoop(
            [thisPtr = shared_from_this(), buffer = std::move(buffer)]()
            {
                thisPtr->sendInLoop(buffer->data(), buffer->length());
            });
    }
}

void TcpConnectionImpl::send(const std::string &msg)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(msg.data(), msg.length());
    }
    else
    {
        loop_->queueInLoop([thisPtr = shared_from_this(), msg]()
                           { thisPtr->sendInLoop(msg.data(), msg.length()); });
    }
}
void TcpConnectionImpl::send(std::string &&msg)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(msg.data(), msg.length());
    }
    else
    {
        loop_->queueInLoop(
            [thisPtr = shared_from_this(), msg = std::move(msg)]()
            {
                thisPtr->sendInLoop(msg.data(), msg.length());
            });
    }
}

void TcpConnectionImpl::send(const MsgBuffer &buffer)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(buffer.peek(), buffer.readableBytes());
    }
    else
    {
        loop_->queueInLoop([thisPtr = shared_from_this(), buffer]()
                           { thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes()); });
    }
}

void TcpConnectionImpl::send(MsgBuffer &&buffer)
{
    if (loop_->isInLoopThread())
    {
        sendInLoop(buffer.peek(), buffer.readableBytes());
    }
    else
    {
        loop_->queueInLoop(
            [thisPtr = shared_from_this(), buffer = std::move(buffer)]()
            {
                thisPtr->sendInLoop(buffer.peek(), buffer.readableBytes());
            });
    }
}

void TcpConnectionImpl::sendFile(const char *fileName,
                                 long long offset,
                                 long long length)
{
    assert(fileName);
#ifdef _WIN32
#else
    auto fileNode = BufferNode::newFileBufferNode(fileName, offset, length);
    if (!fileNode->available())
    {
        LOG_SYSERR << fileName << " open error";
        return;
    }

    sendFile(std::move(fileNode));
#endif
}

void TcpConnectionImpl::sendFile(const wchar_t *fileName,
                                 long long offset,
                                 long long length)
{
    assert(fileName);
#ifndef _WIN32
#else
#endif
}