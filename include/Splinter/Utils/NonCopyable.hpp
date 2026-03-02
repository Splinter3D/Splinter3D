#pragma once

#include <Splinter/Api.hpp>

namespace splinter::utils {

/**
 * @brief A base class for classes that should not be copyable.
 */
class SPLINTER_API NonCopyable
{
    public:
        NonCopyable(const NonCopyable &other) = delete;
        NonCopyable &operator=(const NonCopyable &rhs) = delete;
        NonCopyable(NonCopyable &&other) = delete;
        NonCopyable &operator=(NonCopyable &&rhs) = delete;

    protected:
        constexpr explicit NonCopyable() = default;
        ~NonCopyable() noexcept = default;
};

}// namespace splinter::utils
