#pragma once

#include <Splinter3D/Api.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <iostream>
#include <mutex>
#include <utility>

namespace splinter3D::utils
{

    /**
     * @brief A simple logger class.
     *
     * This class is a singleton, so there can only be one instance of it.
     * It provides two methods, `cout` and `cerr`, which are thread-safe
     * versions of `std::cout` and `std::cerr`.
     */
    class SPLINTER3D_API Logger : public Singleton<Logger>
    {
      public:
        /**
         * @brief Prints a message to standard output.
         * @param args The arguments to print.
         */
        template <typename... Args>
        void cout(Args&&... args)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            (std::cout << ... << args) << std::endl;
        }

        /**
         * @brief Prints a message to standard output.
         * @param args The arguments to print.
         */
        template <typename... Args>
        void clog([[maybe_unused]] Args&&... args)
        {
#if defined(SPLINTER3D_DEBUG)
            std::lock_guard<std::mutex> lock(_mtx);
            (std::clog << ... << args) << std::endl;
#endif
        }

        /**
         * @brief Prints a message to standard error.
         * @param args The arguments to print.
         */
        template <typename... Args>
        void cerr(Args&&... args)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            (std::cerr << ... << args) << std::endl;
        }

      protected:
        constexpr explicit Logger() noexcept = default;
        ~Logger() noexcept                   = default;

      private:
        std::mutex _mtx;

        friend class Singleton<Logger>;
    };

/**
 * @brief Prints a message to given output.
 * @param args The arguments to print.
 */
#define __S3D_LOGGER_FUNCS_FACTORY(FNAME)                                                                                 \
    template <typename... Args>                                                                                           \
    static inline void FNAME(Args&&... args) noexcept(noexcept(Logger::getInstance().FNAME(std::forward<Args>(args)...))) \
    {                                                                                                                     \
        Logger::getInstance().FNAME(std::forward<Args>(args)...);                                                         \
    }

    __S3D_LOGGER_FUNCS_FACTORY(cout)
    __S3D_LOGGER_FUNCS_FACTORY(cerr)
    __S3D_LOGGER_FUNCS_FACTORY(clog)

#undef __S3D_LOGGER_FUNCS_FACTORY

} // namespace splinter3D::utils
