/**
 * @file Utilities.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#pragma once

#include <xiaoNet/exports.h>
#include <string>

namespace xiaoNet
{
    /**
     * @brief xiaoNet helper functions.
     *
     */
    namespace utils
    {
        /**
         * @brief Convert a wide string to a UTF-8
         * @details UCS2 on Windows, UTF-32 on linux & Mac
         *
         * @param wstr
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT std::string toUtf8(const std::wstring &wstr);

        /**
         * @brief Convert a UTF-8 string to a wide string.
         *
         * @param str
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT std::wstring fromUtf8(const std::string &str);

        /**
         * @brief Convert a wide string path with arbitrary directory separators
         * to a UTF-8 portable path for use with xiaoNet.
         *
         * This a helper, mainly for Windows and multi-platform projects.fromUtf8
         *
         * @note on Windows, backslash directory separators are converted to slash to
         * keep portable paths.
         * @note on other OSes, backslashes are not converted to slash, since they
         * are valid characters for directory/file names.
         *
         * @param strPath
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT std::string fromWidePath(const std::wstring &strPath);

        /**
         * @details Convert a UTF-8 path with arbitrary directory separator to a wide
         * string path.
         *
         * This is a helper, mainly for Windows and multi-platform projects.
         *
         * @note On Windows, slash directory separators are converted to backslash.
         * Although it accepts both slash and backslash as directory separator in its
         * API, it is better to stick to its standard.

         * @remarks On other OSes, slashes are not converted to backslashes, since they
         * are not interpreted as directory separators and are valid characters for
         * directory/file names.
         *
         * @param strUtf8Path Ascii path considered as being UTF-8.
         *
         * @return std::wstring path with, on windows, standard backslash directory
         * separator to stick to its standard.
         */
        XIAONET_EXPORT std::wstring toWidePath(const std::string &strUtf8Path);

#if defined(_WIN32) && !defined(__MINGW32__)

#else
        /**
         * @brief Convert an UTF-8 path to a native path.
         *
         * @param strPath
         * @return const std::string&
         */
        inline const std::string &toNativePath(const std::string &strPath)
        {
            return strPath;
        }

        inline std::string toNativePath(const std::wstring &strPath)
        {
            return fromWidePath(strPath);
        }
#endif

        inline const std::string &fromNativePath(const std::string &strPath)
        {
            return strPath;
        }

        inline std::string fromNativePath(const std::wstring &strPath)
        {
            return fromWidePath(strPath);
        }

        /**
         * @brief Check if the name supplied by the SSL Cert matches a FQDN
         *
         * @param certName
         * @param hostName
         * @return true
         * @return false
         */
        bool verifySslName(const std::string &certName, const std::string &hostName);

        /**
         * @brief Returns the TLS backend used by xiaoNet. Could be "None", "OpenSSL"
         * or  "Botan"
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT std::string tlsBackend();

        struct Hash128
        {
            unsigned char bytes[16];
        };

        struct Hash160
        {
            unsigned char bytes[20];
        };

        struct Hash256
        {
            unsigned char bytes[32];
        };

        // provide sane hash functions so users don't have to provide their own
        /**
         * @brief Compute the MD5 hash of the given data
         *
         * @param data
         * @param len
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT Hash128
        md5(const void *data, size_t len);
        inline Hash128 md5(const std::string &str)
        {
            return md5(str.data(), str.size());
        }

        /**
         * @brief Compute the SHA1 hash of the given data
         */
        XIAONET_EXPORT Hash160 sha1(const void *data, size_t len);
        inline Hash160 sha1(const std::string &str)
        {
            return sha1(str.data(), str.size());
        }

        /**
         * @brief Compute the SHA256 hash of the given data
         */
        XIAONET_EXPORT Hash256 sha256(const void *data, size_t len);
        inline Hash256 sha256(const std::string &str)
        {
            return sha256(str.data(), str.size());
        }

        /**
         * @brief Compute the SHA3 hash of the given data
         */
        XIAONET_EXPORT Hash256 sha3(const void *data, size_t len);
        inline Hash256 sha3(const std::string &str)
        {
            return sha3(str.data(), str.size());
        }

        /**
         * @brief Compute the BLAKE2b hash of the given data
         * @note When in doubt, use SHA3 or BLAKE2b. Both are safe and SHA3 is faster if
         * you are using OpenSSL and it has SHA3 in hardware mode. Otherwise BLAKE2b is
         * faster in software.
         */
        XIAONET_EXPORT Hash256 blake2b(const void *data, size_t len);
        inline Hash256 blake2b(const std::string &str)
        {
            return blake2b(str.data(), str.size());
        }

        /**
         * @brief hex encode the given data
         *
         * @note When in doubt, use SHA3 or BLAKE2b. Both are safe and SHA3 is faster if
         * you are using OpenSSL and it has SHA3 in hardware mode. Otherwise BLAKE2b is
         * faster in software.
         * @param data
         * @param len
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT std::string toHexString(const void *data, size_t len);
        inline std::string toHexString(const Hash128 &hash)
        {
            return toHexString(hash.bytes, sizeof(hash.bytes));
        }

        inline std::string toHexString(const Hash160 &hash)
        {
            return toHexString(hash.bytes, sizeof(hash.bytes));
        }

        inline std::string toHexString(const Hash256 &hash)
        {
            return toHexString(hash.bytes, sizeof(hash.bytes));
        }

        /**
         * @brief Generate cryptographically secure rando bytes
         *
         * @param ptr
         * @param size
         * @return XIAONET_EXPORT
         */
        XIAONET_EXPORT bool secureRandomBytes(void *ptr, size_t size);
    }
}