#pragma once

#include <Splinter3D/Utils/NonCopyable.hpp>
#include <type_traits>

namespace splinter3D::utils
{

    /**
     * @brief A template class for creating singletons.
     *
     * @tparam C The class to make a singleton.
     */
    template <class C>
    class Singleton : public NonCopyable
    {
      public:
        /**
         * @brief Gets the singleton instance.
         *
         * @return The singleton instance.
         */
        static C& getInstance() noexcept(std::is_nothrow_default_constructible_v<C>)
        {
            static C instance;
            return instance;
        }

      protected:
        constexpr explicit Singleton() noexcept = default;
        ~Singleton() noexcept                   = default;
    };

} // namespace splinter3D::utils
