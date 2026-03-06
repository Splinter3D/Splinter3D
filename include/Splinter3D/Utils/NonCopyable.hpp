#pragma once

#include <Splinter3D/Api.hpp>

namespace splinter3D::utils
{

    /**
     * @brief A base class for classes that should not be copyable.
     */
    class SPLINTER3D_API NonCopyable
    {
      public:
        NonCopyable(const NonCopyable& other)          = delete;
        NonCopyable& operator=(const NonCopyable& rhs) = delete;
        NonCopyable(NonCopyable&& other)               = delete;
        NonCopyable& operator=(NonCopyable&& rhs)      = delete;

      protected:
        constexpr explicit NonCopyable() noexcept = default;
        ~NonCopyable() noexcept                   = default;
    };

} // namespace splinter3D::utils
