#include "platform/logger/logger.hpp"
#include "support/temp_dir.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using platform::logger::Logger;
using tests::support::TempDir;

namespace
{
    // Swaps a standard stream's buffer for an in-memory one while alive.
    class StreamCapture
    {
      public:
        explicit StreamCapture(std::ostream& stream)
            : _stream(stream)
            , _previous(stream.rdbuf(_buffer.rdbuf()))
        { }

        ~StreamCapture()
        {
            _stream.rdbuf(_previous);
        }

        StreamCapture(const StreamCapture&)            = delete;
        StreamCapture& operator=(const StreamCapture&) = delete;

        [[nodiscard]] std::string str() const
        {
            return _buffer.str();
        }

      private:
        std::ostream&      _stream;
        std::ostringstream _buffer;
        std::streambuf*    _previous;
    };

    std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream      file(path);
        std::ostringstream content;
        content << file.rdbuf();
        return content.str();
    }
} // namespace

TEST_CASE("Logger: cout and cerr concatenate their arguments on one line", "[platform][logger]")
{
    StreamCapture out(std::cout);
    StreamCapture err(std::cerr);

    Logger::getInstance().cout("value=", 42, ' ', 1.5);
    Logger::getInstance().cerr("error ", "code");

    CHECK(out.str() == "value=42 1.5\n");
    CHECK(err.str() == "error code\n");
}

TEST_CASE("Logger: clog only prints in debug", "[platform][logger]")
{
    StreamCapture log(std::clog);
    auto&         logger = Logger::getInstance();

    logger.setDebug(true);
    logger.clog("visible");
    logger.setDebug(false);
    logger.clog("hidden");

#if defined(SPLINTER3D_DEBUG)
    // Debug builds always log, whatever setDebug() says.
    CHECK(log.str() == "visible\nhidden\n");
#else
    CHECK(log.str() == "visible\n");
#endif
}

TEST_CASE("Logger: messages are mirrored to the output file", "[platform][logger]")
{
    TempDir       tmp;
    const auto    path = tmp.path() / "splinter3D.log";
    StreamCapture out(std::cout);
    auto&         logger = Logger::getInstance();

    logger.setOutFile(path.string());
    logger.cout("first");
    logger.cout("second ", 2);
    logger.setOutFile(); // closes (and flushes) the file

    CHECK(out.str() == "first\nsecond 2\n");
    CHECK(readFile(path) == "first\nsecond 2\n");

    logger.cout("after close");
    CHECK(readFile(path) == "first\nsecond 2\n");
}

TEST_CASE("Logger: namespace-level helpers forward to the instance", "[platform][logger]")
{
    TempDir       tmp;
    const auto    path = tmp.path() / "helpers.log";
    StreamCapture out(std::cout);
    StreamCapture err(std::cerr);
    StreamCapture log(std::clog);

    platform::logger::logger::setOutFile(path.string());
    platform::logger::cout("out ", 1);
    platform::logger::cerr("err ", 2);
    platform::logger::logger::setDebug(true);
    platform::logger::clog("log ", 3);
    platform::logger::logger::setDebug(false);
    platform::logger::logger::setOutFile();

    CHECK(out.str() == "out 1\n");
    CHECK(err.str() == "err 2\n");
    CHECK(log.str() == "log 3\n");
    CHECK(readFile(path) == "out 1\nerr 2\nlog 3\n");
}
