/**
 * @file Acceptor.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include "Acceptor.h"
using namespace xiaoNet;

#ifndef O_CLOEXEC
#define O_CLOEXEC O_NOINHERIT
#endif

Acceptor::Acceptor(EventLoop *loop,
                   const InetAddress &addr,
                   bool reUseAddr,
                   bool reUsePort)
    :
#ifndef _WIN32
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
#endif
      sock_(
          Socket::createNonblockingSocketOrDie(addr.getSockAddr()->sa_family)),
      addr_(addr),
      loop_(loop),
      acceptChannel_(loop, sock_.fd())
{
    sock_.setReuseAddr(reUseAddr);
    sock_.setReusePort(reUsePort);
    sock_.bindAddress(addr_);
    acceptChannel_.setReadCallback(std::bind(&Acceptor::readCallback, this));
    if (addr_.toPort() == 0)
    {
        addr_ = InetAddress{Socket::getLocalAddr(sock_.fd())};
    }
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
#ifndef _WIN32
    ::close(idleFd_);
#endif
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    if (beforeListenSetSockOptCallback_)
        beforeListenSetSockOptCallback_(sock_.fd());
    sock_.listen();
    LOG_DEBUG << "listen called";
    acceptChannel_.enableReading();
}

void Acceptor::readCallback()
{
    LOG_DEBUG << "Acceptor::readCallback called";
    InetAddress peer;
    int newsock = sock_.accept(&peer);
    LOG_DEBUG << newsock;
    if (newsock >= 0)
    {
        if (afterAcceptSetSockOptCallback_)
            afterAcceptSetSockOptCallback_(newsock);
        if (newConnectionCallback_)
            newConnectionCallback_(newsock, peer);
        else
        {
#ifndef _WIN32
            ::close(newsock);
#else
            closesocket(newsock);
#endif
        }
    }
    else
    {
        LOG_SYSERR << "Acceptor::readCallback";
// Read the section named "The special problem of
// accept()ing when you can't" in libev's doc.
// By Marc Lehmann, author of libev.
/// errno is thread safe
#ifndef _WIN32
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = sock_.accept(&peer);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
#endif
    }
}
