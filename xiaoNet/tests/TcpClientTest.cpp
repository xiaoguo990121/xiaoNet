#include <xiaoNet/net/TcpClient.h>
#include <xiaoLog/Logger.h>
#include <xiaoNet/utils/MsgBuffer.h>

#ifdef _WIN32
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#endif

using namespace xiaoNet;
#define USE_IPV6 0
int main()
{
    xiaoLog::Logger::setLogLevel(xiaoLog::Logger::kTrace);
    LOG_DEBUG << "TcpClient class test!";
    EventLoop loop;
#if USE_IPV6
    InetAddress serverAddr("::1", 8888, true);
#else
    InetAddress serverAddr("127.0.0.1", 8888);
#endif
    std::shared_ptr<xiaoNet::TcpClient> client[10];
    std::atomic_int connCount;
    connCount = 1;
    for (int i = 0; i < 1; ++i)
    {
        client[i] = std::make_shared<xiaoNet::TcpClient>(&loop,
                                                         serverAddr, "tcpclienttest");
        client[i]->setSockOptCallback(
            [](int fd)
            {
                LOG_DEBUG << "setSockOptCallback!";

#ifdef _WIN32
#elif __linux__
                int optval = 10;
                ::setsockopt(fd,
                             SOL_TCP,
                             TCP_KEEPCNT,
                             &optval,
                             static_cast<socklen_t>(sizeof optval));

                ::setsockopt(fd,
                             SOL_TCP,
                             TCP_KEEPIDLE,
                             &optval,
                             static_cast<socklen_t>(sizeof optval));
                ::setsockopt(fd,
                             SOL_TCP,
                             TCP_KEEPINTVL,
                             &optval,
                             static_cast<socklen_t>(sizeof optval));
#else
#endif
            });

        client[i]->setConnectionCallback(
            [i, &loop, &connCount](const TcpConnectionPtr &conn)
            {
                if (conn->connected())
                {
                    LOG_DEBUG << i << " connected!";
                    std::string tmp = std::to_string(i) + " client!!";
                    conn->send(tmp);
                }
                else
                {
                    LOG_DEBUG << i << " disconnected";
                    --connCount;
                    if (connCount == 0)
                        loop.quit();
                }
            });
        client[i]->setMessageCallback(
            [](const TcpConnectionPtr &conn, MsgBuffer *buf)
            {
                LOG_DEBUG << std::string(buf->peek(), buf->readableBytes());
                buf->retrieveAll();
                conn->shutdown();
            });
        client[i]->connect();
    }
    loop.loop();
}