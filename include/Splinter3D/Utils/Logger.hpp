#pragma once

#include <Splinter3D/Utils/Singleton.hpp>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <thread>
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
    class Logger : public Singleton<Logger>
    {
      public:
        /**
         * @brief Prints a message to standard output.
         * @param args The arguments to print.
         */
        template <typename... Args>
        void cout(Args&&... args)
        {
            _printHelper(std::cout, std::forward<Args>(args)...);
        }

        /**
         * @brief Prints a message to standard output.
         * @param args The arguments to print.
         */
        template <typename... Args>
        void clog([[maybe_unused]] Args&&... args)
        {
#if defined(SPLINTER3D_DEBUG)
            _printHelper(std::clog, std::forward<Args>(args)...);
#else
            if (_debug)
            {
                _printHelper(std::clog, std::forward<Args>(args)...);
            }
#endif
        }

        /**
         * @brief Prints a message to standard error.
         * @param args The arguments to print.
         */
        template <typename... Args>
        void cerr(Args&&... args)
        {
            _printHelper(std::cerr, std::forward<Args>(args)...);
        }

        void setOutFile(const std::string& filename)
        {
            _outFile = std::ofstream(filename);
        }

        void setOutFile()
        {
            _outFile.reset();
        }

        void setDebug(bool debug)
        {
            _debug = debug;
        }

      protected:
        constexpr explicit Logger() noexcept = default;
        ~Logger() noexcept                   = default;

      private:
        bool                         _debug = false;
        std::mutex                   _fileMtx;
        std::mutex                   _consoleMtx;
        std::optional<std::ofstream> _outFile;

        template <typename... Args>
        void _printHelper(std::ostream& oss, Args&&... args)
        {
            std::ostringstream buffer;
            (buffer << ... << std::forward<Args>(args));
            const std::string msg = buffer.str();

            _maybeStartThreads(oss, msg);
        }

        void _maybeStartThreads(std::ostream& oss, const std::string& msg);
        void _printToFile(const std::string& msg);
        void _printToConsole(std::ostream& oss, const std::string& msg);

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

    namespace logger
    {

        static inline void setOutFile(const std::string& filename)
        {
            Logger::getInstance().setOutFile(filename);
        }

        static inline void setOutFile()
        {
            Logger::getInstance().setOutFile();
        }

        static inline void setDebug(bool debug)
        {
            Logger::getInstance().setDebug(debug);
        }

    } // namespace logger

} // namespace splinter3D::utils
