/**
 * @file NonCopyable.h
 * @author Guo Xiao
 * @brief
 * @version 0.1
 * @date 2024-12-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <xiaoNet/exports.h>

namespace xiaoNet
{
    /**
     * @brief This class represents a non-copyable object.
     *
     */
    class XIAONET_EXPORT NonCopyable
    {
    protected:
        NonCopyable()
        {
        }
        ~NonCopyable()
        {
        }
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable &operator=(const NonCopyable &) = delete;

        NonCopyable(NonCopyable &&) noexcept(true) = default;
        NonCopyable &operator=(NonCopyable &&) noexcept(true) = default;
    };
}