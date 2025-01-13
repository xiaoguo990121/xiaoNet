#include <xiaoNet/net/TcpServer.h>
#include <xiaoLog/Logger.h>
#include <xiaoNet/net/EventLoopThread.h>
#include <xiaoNet/utils/MsgBuffer.h>
#include <iostream>
#include <string>

using namespace xiaoNet;
#define USE_IPV6 0

int main()
{
    LOG_TRACE << "test start";
    Logger::setLogLevel(Logger::kTrace);
    EventLoopThread loopThread;
    loopThread.run();

#if USE_IPV6
    InetAddress addr(8888, true, true);
#else
    InetAddress addr(8888);
#endif
    TcpServer server(loopThread.getLoop(), addr, "test");
    server.setBeforeListenSockOptCallback([](int fd)
                                          { std::cout << "setBeforeListenSockOptCallback:" << fd << std::endl; });
    server.setAfterAcceptSockOptCallback([](int fd)
                                         { std::cout << "afterAcceptSockOptCallback:" << fd << std::endl; });
    server.setRecvMessageCallback(
        [](const TcpConnectionPtr &connectionPtr, MsgBuffer *buffer)
        {
            LOG_DEBUG << "recv callback!";
            std::cout << std::string(buffer->peek(), buffer->readableBytes()) << std::endl;
            connectionPtr->send(buffer->peek(), buffer->readableBytes());
            buffer->retrieveAll();
        });
    server.setConnectionCallback([](const TcpConnectionPtr &connPtr)
                                 {
        if(connPtr->connected())
        {
            LOG_DEBUG << "New connection";
        }
        else if(connPtr->disconnected())
        {
            LOG_DEBUG<<"connection disconnected";
        } });
    server.setIoLoopNum(3);
    server.start();
    loopThread.wait();
}