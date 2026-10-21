#pragma once

#include <atomic>
#include <filesystem>
#include <random>
#include <string>

namespace tests::support
{
    /**
     * @brief Creates a unique empty directory under the system temp folder and
     * removes it (recursively) when going out of scope.
     */
    class TempDir
    {
      public:
        TempDir()
            : _path(std::filesystem::temp_directory_path() / uniqueName())
        {
            std::filesystem::create_directories(_path);
        }

        ~TempDir()
        {
            std::error_code ec;
            std::filesystem::remove_all(_path, ec);
        }

        TempDir(const TempDir&)            = delete;
        TempDir& operator=(const TempDir&) = delete;

        [[nodiscard]] const std::filesystem::path& path() const noexcept
        {
            return _path;
        }

      private:
        static std::string uniqueName()
        {
            static std::atomic<unsigned> counter{0};
            return "splinter3d-test-" + std::to_string(std::random_device{}()) + "-" + std::to_string(counter++);
        }

        std::filesystem::path _path;
    };

    /**
     * @brief A TempDir that is also the current working directory while alive.
     */
    class TempCwd : public TempDir
    {
      public:
        TempCwd()
            : _previous(std::filesystem::current_path())
        {
            std::filesystem::current_path(path());
        }

        ~TempCwd()
        {
            // Leave the directory before TempDir's destructor removes it.
            std::filesystem::current_path(_previous);
        }

        TempCwd(const TempCwd&)            = delete;
        TempCwd& operator=(const TempCwd&) = delete;

      private:
        std::filesystem::path _previous;
    };
} // namespace tests::support
