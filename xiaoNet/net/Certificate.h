/**
 * @file Certificate.h
 * @author Guo Xiao (746921314@qq.com)
 * @brief
 * @version 0.1
 * @date 2025-01-08
 *
 *
 */
#pragma once
#include <string>
#include <memory>

namespace xiaoNet
{
    struct Certificate
    {
        virtual ~Certificate() = default;
        virtual std::string sha1Fingerprint() const = 0;
        virtual std::string sha256Fingerprint() const = 0;
        virtual std::string pem() const = 0;
    };
    using CertificatePtr = std::shared_ptr<Certificate>;

}