/**
 * @file InetAddress.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */

#pragma once

#include <xiaoLog/Date.h>
#include <xiaoNet/exports.h>

#ifdef _WIN32
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif
#include <string>
#include <unordered_map>
#include <mutex>

namespace xiaoNet
{

    /**
     * @brief Wrapper of sockaddr_in. This is an POD interface class.
     *
     */
    class XIAONET_EXPORT InetAddress
    {
    public:
        /**
         * @brief Construct a new Inet Address object. Mostly used in
         * TcpServer listening.
         *
         * @param port
         * @param loopbackOnly
         * @param ipv6
         */
        InetAddress(uint16_t port = 0,
                    bool loopbackOnly = false,
                    bool ipv6 = false);

        /**
         * @brief Construct a new Inet Address object
         *
         * @param ip
         * @param port
         * @param ipv6
         */
        InetAddress(const std::string &ip, uint16_t port, bool ipv6 = false);

        /**
         * @brief Construct a new Inet Address object. Mostly used in
         * used when accepting new connections
         *
         * @param addr
         */
        explicit InetAddress(const struct sockaddr_in &addr)
            : addr_(addr), isUnspecified_(false)
        {
        }

        explicit InetAddress(const struct sockaddr_in6 &addr)
            : addr6_(addr), isIpV6_(true), isUnspecified_(false)
        {
        }

        /**
         * @brief return the sin_family of the endpoint.
         *
         * @return sa_family_t
         */
        sa_family_t family() const
        {
            return addr_.sin_family;
        }

        /**
         * @brief return the ip string of the endpoint.
         *
         * @return std::string
         */
        std::string toIp() const;

        /**
         * @brief return the ip and port string of the endpoint.
         *
         * @return std::string
         */
        std::string toIpPort() const;

        /**
         * @brief return the ip bytes of the endpoint in net endian byte order
         *
         * @return std::string
         */
        std::string toIpNetEndian() const;

        /**
         * @brief return the ip and port bytes of the endpoint in net endian byte order
         *
         * @return std::string
         */
        std::string toIpPortNetEndian() const;

        /**
         * @brief return the port number of the endpoint.
         *
         * @return uint16_t
         */
        uint16_t toPort() const;

        /**
         * @brief check if the endpoint is ipv4 or ipv6
         *
         * @return true
         * @return false
         */
        bool isIpV6() const
        {
            return isIpV6_;
        }

        /**
         * @brief return true if the endpoint is an intranet endpoint.
         *
         * @return true
         * @return false
         */
        bool isIntranetIp() const;

        /**
         * @brief return true if the endpoint is a loopback endpoint.
         *
         * @return true
         * @return false
         */
        bool isLoopbackIp() const;

        /**
         * @brief Get the Sock Addr object
         *
         * @return const struct sockaddr*
         */
        const struct sockaddr *getSockAddr() const
        {
            return static_cast<const struct sockaddr *>((void *)(&addr6_));
        }

        /**
         * @brief Set the Sock Addr Inet6 object
         *
         * @param addr6
         */
        void setSockAddrInet6(const struct sockaddr_in6 &addr6)
        {
            addr6_ = addr6;
            isIpV6_ = (addr6_.sin6_family == AF_INET6);
            isUnspecified_ = false;
        }

        /**
         * @brief return the integer value of the ip(v4) in net endian byte order.
         *
         * @return uint32_t
         */
        uint32_t ipNetEndian() const;

        /**
         * @brief return the pointer to the integer value of the ip(v6) int net
         * endian byte order.
         *
         * @return const uint32_t*
         */
        const uint32_t *ip6NetEndian() const;

        /**
         * @brief return the port number in net endian byte order.
         *
         * @return uint16_t
         */
        uint16_t portNetEndian() const
        {
            return addr_.sin_port;
        }

        /**
         * @brief Set the Port Net Endian object
         *
         * @param port
         */
        void setPortNetEndian(uint16_t port)
        {
            addr_.sin_port = port;
        }

        inline bool isUnspecified() const
        {
            return isUnspecified_;
        }

    private:
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };
        bool isIpV6_{false};
        bool isUnspecified_{true};
    };
}