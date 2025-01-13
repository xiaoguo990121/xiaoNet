/**
 * @file Utilities.cpp
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-11
 *
 *
 */

#include "Utilities.h"
#ifdef _WIN32

#else
#include <unistd.h>
#include <string.h>
#if __cplusplus < 201103L || __cplusplus >= 201703L
#include <stdlib.h>
#include <locale.h>
#else
#include <locale>
#include <codecvt>
#endif
#endif

#include <assert.h>
#include <stdint.h>
#if defined(USE_OPENSSL)
#elif defined(USE_BOTAN)
#else
#endif

#ifdef _MSC_VER
#include <intrin.h>
#else
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif
#endif

#if __cplusplus < 201103L || __cplusplus >= 201703L
static std::wstring utf8Toutf16(const std::string &utf8Str)
{
    std::wstring utf16Str;
    utf16Str.reserve(utf8Str.length()); // Reserve space to avoid reallocations

    for (size_t i = 0; i < utf8Str.length();)
    {
        wchar_t unicode_char;

        // Check the first byte
        if ((utf8Str[i] & 0b10000000) == 0)
        {
            // Single-byte character (ASCII)
            unicode_char = utf8Str[i++];
        }
        else if ((utf8Str[i] & 0b11100000) == 0b11000000)
        {
            if (i + 1 >= utf8Str.length())
            {
                // Invalid UTF-8 sequence
                // Handle the error as needed
                return L"";
            }
            // Two-byte character
            unicode_char = ((utf8Str[i] & 0b00011111) << 6) |
                           (utf8Str[i + 1] & 0b00111111);
            i += 2;
        }
        else if ((utf8Str[i] & 0b11110000) == 0b11100000)
        {
            if (i + 2 >= utf8Str.length())
            {
                // Invalid UTF-8 sequence
                // Handle the error as needed
                return L"";
            }
            // Three-byte character
            unicode_char = ((utf8Str[i] & 0b00001111) << 12) |
                           ((utf8Str[i + 1] & 0b00111111) << 6) |
                           (utf8Str[i + 2] & 0b00111111);
            i += 3;
        }
        else
        {
            // Invalid UTF-8 sequence
            // Handle the error as needed
            return L"";
        }

        utf16Str.push_back(unicode_char);
    }

    return utf16Str;
}

static std::string utf16Toutf8(const std::wstring &utf16Str)
{
    std::string utf8Str;
    utf8Str.reserve(utf16Str.length() * 3);

    for (size_t i = 0; i < utf16Str.length(); ++i)
    {
        wchar_t unicode_char = utf16Str[i];

        if (unicode_char <= 0x7F)
        {
            // Single-byte character (ASCII)
            utf8Str.push_back(static_cast<char>(unicode_char));
        }
        else if (unicode_char <= 0x7FF)
        {
            // Two-byte character
            utf8Str.push_back(
                static_cast<char>(0xC0 | ((unicode_char >> 6) & 0x1F)));
            utf8Str.push_back(static_cast<char>(0x80 | (unicode_char & 0x3F)));
        }
        else
        {
            // Three-byte character
            utf8Str.push_back(
                static_cast<char>(0xE0 | ((unicode_char >> 12) & 0x0F)));
            utf8Str.push_back(
                static_cast<char>(0x80 | ((unicode_char >> 6) & 0x3F)));
            utf8Str.push_back(static_cast<char>(0x80 | (unicode_char & 0x3F)));
        }
    }

    return utf8Str;
}

#endif

namespace xiaoNet
{
    namespace utils
    {
        std::string toUtf8(const std::wstring &wstr)
        {
            if (wstr.empty())
                return {};

            std::string strTo;
#ifdef _WIN32
            int nSizeNeeded = ::WideCharToMultiByte(
                CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
            strTo.resize(nSizeNeeded, 0);
            ::WideCharToMultiByte(CP_UTF8,
                                  0,
                                  &wstr[0],
                                  (int)wstr.size(),
                                  &strTo[0],
                                  nSizeNeeded,
                                  NULL,
                                  NULL);
#elif __cplusplus < 201103L || __cplusplus >= 201703L
            strTo = utf16Toutf8(wstr);
#else // c++11 to c++14
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
            strTo = utf8conv.to_bytes(wstr);
#endif
            return strTo;
        }
        std::wstring fromUtf8(const std::string &str)
        {
            if (str.empty())
                return {};
            std::wstring wstrTo;
#ifdef _WIN32
            int nSizeNeeded =
                ::MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
            wstrTo.resize(nSizeNeeded, 0);
            ::MultiByteToWideChar(
                CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], nSizeNeeded);
#elif __cplusplus < 201103L || __cplusplus >= 201703L
            wstrTo = utf8Toutf16(str);
#else // c++11 to c++14
            std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
            try
            {
                wstrTo = utf8conv.from_bytes(str);
            }
            catch (...) // Should never fail if str valid UTF-8
            {
            }
#endif
            return wstrTo;
        }
        std::wstring toWidePath(const std::string &strUtf8Path)
        {
            auto wstrPath{fromUtf8(strUtf8Path)};
#ifdef _WIN32
            // Not needed: normalize path (just replaces '/' with '\')
            std::replace(wstrPath.begin(), wstrPath.end(), L'/', L'\\');
#endif // _WIN32
            return wstrPath;
        }

        std::string fromWidePath(const std::wstring &wstrPath)
        {
#ifdef _WIN32
            auto srcPath{wstrPath};
            // Not needed: to portable path (just replaces '\' with '/')
            std::replace(srcPath.begin(), srcPath.end(), L'\\', L'/');
#else  // _WIN32
            auto &srcPath{wstrPath};
#endif // _WIN32
            return toUtf8(srcPath);
        }

        bool verifySslName(const std::string &certName, const std::string &hostname)
        {
            if (certName.find('*') == std::string::npos)
            {
                return certName == hostname;
            }

            size_t firstDot = certName.find('.');
            size_t hostFirstDot = hostname.find('.');
            size_t pos, len, hostPos, hostLen;

            if (firstDot != std::string::npos)
            {
                pos = firstDot + 1;
            }
            else
            {
                firstDot = pos = certName.size();
            }

            len = certName.size() - pos;

            if (hostFirstDot != std::string::npos)
            {
                hostPos = hostFirstDot + 1;
            }
            else
            {
                hostFirstDot = hostPos = hostname.size();
            }

            hostLen = hostname.size() - hostPos;

            // *. in the beginning of the cert name
            if (certName.compare(0, firstDot, "*") == 0)
            {
                return certName.compare(pos, len, hostname, hostPos, hostLen) == 0;
            }
            // * in the left most. but other chars in the right
            else if (certName[0] == '*')
            {
                // compare if `hostname` ends with `certName` but without the leftmost
                // should be fine as domain names can't be that long
                intmax_t hostnameIdx = hostname.size() - 1;
                intmax_t certNameIdx = certName.size() - 1;
                while (hostnameIdx >= 0 && certNameIdx != 0)
                {
                    if (hostname[hostnameIdx] != certName[certNameIdx])
                    {
                        return false;
                    }
                    hostnameIdx--;
                    certNameIdx--;
                }
                if (certNameIdx != 0)
                {
                    return false;
                }
                return true;
            }
            // * in the right of the first dot
            else if (firstDot != 0 && certName[firstDot - 1] == '*')
            {
                if (certName.compare(pos, len, hostname, hostPos, hostLen) != 0)
                {
                    return false;
                }
                for (size_t i = 0;
                     i < hostFirstDot && i < firstDot && certName[i] != '*';
                     i++)
                {
                    if (hostname[i] != certName[i])
                    {
                        return false;
                    }
                }
                return true;
            }
            // else there's a * in  the middle
            else
            {
                if (certName.compare(pos, len, hostname, hostPos, hostLen) != 0)
                {
                    return false;
                }
                for (size_t i = 0;
                     i < hostFirstDot && i < firstDot && certName[i] != '*';
                     i++)
                {
                    if (hostname[i] != certName[i])
                    {
                        return false;
                    }
                }
                intmax_t hostnameIdx = hostFirstDot - 1;
                intmax_t certNameIdx = firstDot - 1;
                while (hostnameIdx >= 0 && certNameIdx >= 0 &&
                       certName[certNameIdx] != '*')
                {
                    if (hostname[hostnameIdx] != certName[certNameIdx])
                    {
                        return false;
                    }
                    hostnameIdx--;
                    certNameIdx--;
                }
                return true;
            }

            assert(false && "This line should not be reached in verifySslName");
            // should not reach
            return certName == hostname;
        }
    }
}