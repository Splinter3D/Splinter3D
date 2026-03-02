#pragma once

#include <Splinter3D/Utils/NonCopyable.hpp>

namespace splinter3D::utils
{

    /**
     * @brief A template class for creating singletons.
     *
     * @tparam C The class to make a singleton.
     */
    template <class C>
    class SPLINTER3D_API Singleton : public NonCopyable
    {
      public:
        /**
         * @brief Gets the singleton instance.
         *
         * @return The singleton instance.
         */
        static C& getInstance()
        {
            static C instance;
            return instance;
        }

      protected:
        constexpr explicit Singleton() = default;
        ~Singleton() noexcept          = default;
    };

} // namespace splinter3D::utils
