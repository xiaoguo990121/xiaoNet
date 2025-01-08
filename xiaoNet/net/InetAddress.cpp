/**
 * @file InetAddress.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */

#include <xiaoNet/net/InetAddress.h>

#include <xiaoLog/Logger.h>

#ifdef _WIN32
#else
#include <string.h>
#endif

static const in_addr_t kInaddrAny = INADDR_ANY;           // 0.0.0.0
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK; // 127.0.0.1

using namespace xiaoNet;
using namespace xiaoLog;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6)
    : isIpV6_(ipv6)
{
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htons(port);
    }
    else
    {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = htonl(ip);
        addr_.sin_port = htons(port);
    }
    isUnspecified_ = false;
}

InetAddress::InetAddress(const std::string &ip, uint16_t port, bool ipv6)
    : isIpV6_(ipv6)
{
    if (ipv6)
    {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0)
        {
            return;
        }
    }
    else
    {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
        {
            return;
        }
    }
    isUnspecified_ = false;
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = "";
    uint16_t port = ntohs(addr_.sin_port);
    snprintf(buf, sizeof(buf), ":%u", port);
    return toIp() + std::string(buf);
}
std::string InetAddress::toIpPortNetEndian() const
{
    std::string buf;
    static constexpr auto bytes = sizeof(addr_.sin_port);
    buf.resize(bytes);
#if defined _WIN32
#else
    std::memcpy(&buf[0], &addr_.sin_port, bytes);
#endif
    return toIpNetEndian() + buf;
}
bool InetAddress::isIntranetIp() const
{
    if (addr_.sin_family == AF_INET)
    {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
            (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
            (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
            ip_addr == 0x7f000001)
        {
            return true;
        }
    }
    else
    {
        auto addrP = ip6NetEndian();
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 &&
            ntohl(*(addrP + 3)) == 1)
            return true;
        auto i32 = (ntohl(*addrP) & 0xffc00000);
        if (i32 == 0xfec00000 || i32 == 0xfe800000)
            return true;
        if (*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff)
        {
            // the IPv6 version of an IPv4 IP address
            uint32_t ip_addr = ntohl(*(addrP + 3));
            if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) ||
                (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
                (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) ||
                ip_addr == 0x7f000001)

            {
                return true;
            }
        }
    }
    return false;
}

bool InetAddress::isLoopbackIp() const
{
    if (!isIpV6())
    {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if (ip_addr == 0x7f000001)
        {
            return true;
        }
    }
    else
    {
        auto addrP = ip6NetEndian();
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 &&
            ntohl(*(addrP + 3)) == 1)
            return true;
        // the IPv6 version of an IPv4 loopback address
        if (*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff &&
            ntohl(*(addrP + 3)) == 0x7f000001)
            return true;
    }
    return false;
}

std::string InetAddress::toIp() const
{
    char buf[64];
    if (addr_.sin_family == AF_INET)
    {
#if defined _WIN32
        ::inet_ntop(AF_INET, (PVOID)&addr_.sin_addr, buf, sizeof(buf));
#else
        ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
#endif
    }
    else if (addr_.sin_family == AF_INET6)
    {
#if defined _WIN32
        ::inet_ntop(AF_INET6, (PVOID)&addr6_.sin6_addr, buf, sizeof(buf));
#else
        ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof(buf));
#endif
    }

    return buf;
}

std::string InetAddress::toIpNetEndian() const
{
    std::string buf;
    if (addr_.sin_family == AF_INET)
    {
        static constexpr auto bytes = sizeof(addr_.sin_addr.s_addr);
        buf.resize(bytes);
#if defined _WIN32
        std::memcpy((PVOID)&buf[0], (PVOID)&addr_.sin_addr.s_addr, bytes);
#else
        std::memcpy(&buf[0], &addr_.sin_addr.s_addr, bytes);
#endif
    }
    else if (addr_.sin_family == AF_INET6)
    {
        static constexpr auto bytes = sizeof(addr6_.sin6_addr);
        buf.resize(bytes);
#if defined _WIN32
        std::memcpy((PVOID)&buf[0], (PVOID)ip6NetEndian(), bytes);
#else
        std::memcpy(&buf[0], ip6NetEndian(), bytes);
#endif
    }

    return buf;
}

uint32_t InetAddress::ipNetEndian() const
{
    return addr_.sin_addr.s_addr;
}

const uint32_t *InetAddress::ip6NetEndian() const
{
// assert(family() == AF_INET6);
#if defined __linux__ || defined __HAIKU__
    return addr6_.sin6_addr.s6_addr32;
#elif defined __sun
    return addr6_.sin6_addr._S6_un._S6_u32;
#elif defined _WIN32
    // TODO is this OK ?
    const struct in6_addr_uint *addr_temp =
        reinterpret_cast<const struct in6_addr_uint *>(&addr6_.sin6_addr);
    return (*addr_temp).uext.__s6_addr32;
#else
    return addr6_.sin6_addr.__u6_addr.__u6_addr32;
#endif
}

uint16_t InetAddress::toPort() const
{
    return ntohs(portNetEndian());
}