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

using namespace xiaoNet;

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &address,
                     std::string name,
                     bool reUseAddr,
                     bool reUsePort)
    : loop_(loop),
      acceptorPtr_(new Acceptor(loop, address, reUseAddr, reUsePort)),
{
}