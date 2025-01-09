/**
 * @file Socket.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-10
 *
 *
 */

#pragma once

#include <xiaoNet/utils/NonCopyable.h>
#include <xiaoNet/net/InetAddress.h>
#include <xiaoLog/Logger.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <fcntl.h>

namespace xiaoNet
{
    class Socket : NonCopyable
    {
    public:
        static int createNonblockingSocketOrDie(int family)
        {
#ifdef __linux__
            int sock = ::socket(family,
                                SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                                IPPROTO_TCP);
#else
            int sock = static_cast<int>(::socket(family, SOCK_STREAM, IPPROTO_TCP));
            setNonBlockAndCloseOnExec(sock);
#endif
            if (sock < 0)
            {
                LOG_SYSERR << "sockets::createNonblockingOrDie";
                exit(1);
            }
            LOG_TRACE << "sock=" << sock;
            return sock;
        }

        static int getSocketError(int sockfd)
        {
            int optval;
            socklen_t optlen = static_cast<socklen_t>(sizeof optval);
#ifdef _WIN32
#else
            if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
#endif
            {
                return errno;
            }
            else
            {
                return optval;
            }
        }

        static int connect(int sockfd, const InetAddress &addr)
        {
            if (addr.isIpV6())
                return ::connect(sockfd,
                                 addr.getSockAddr(),
                                 static_cast<socklen_t>(
                                     sizeof(struct sockaddr_in6)));
            else
                return ::connect(sockfd,
                                 addr.getSockAddr(),
                                 static_cast<socklen_t>(
                                     sizeof(struct sockaddr_in)));
        }

        static bool isSelfConnect(int sockfd);

        explicit Socket(int sockfd) : sockFd_(sockfd)
        {
        }
        ~Socket();

        /// abort if address in use
        void bindAddress(const InetAddress &localaddr);
        /// abort if address in use
        void listen();
        int accept(InetAddress *peeraddr);
        void closeWrite();
        int read(char *buffer, uint64_t len);
        int fd()
        {
            return sockFd_;
        }
        static struct sockaddr_in6 getLocalAddr(int sockfd);
        static struct sockaddr_in6 getPeerAddr(int sockfd);

        void setTcpNoDelay(bool on);

        void setReuseAddr(bool on);

        void setReusePort(bool on);

        void setKeepAlive(bool on);
        int getSocketError();

    protected:
        int sockFd_;

    public:
        static void setNonBlockAndCloseOnExec(int sockfd)
        {
#ifdef _WIN32
#else
            int flags = ::fcntl(sockfd, F_GETFL, 0);
            flags |= O_NONBLOCK;
            int ret = ::fcntl(sockfd, F_SETFL, flags);

            flags = ::fcntl(sockfd, F_GETFD, 0);
            flags |= FD_CLOEXEC;
            ret = ::fcntl(sockfd, F_SETFD, flags);

            (void)ret;
#endif
        }
    };
}