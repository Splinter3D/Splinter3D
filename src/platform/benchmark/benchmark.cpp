#include "benchmark.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// psapi.h needs <windows.h>'s types (BOOL, DWORD, WINAPI) declared first.
// clang-format off
#include <psapi.h>
// clang-format on
#else
#include <sys/resource.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#endif
#endif

namespace platform::benchmark
{
    namespace
    {
        constexpr std::size_t kFrameHistory = 4096;

        std::array<std::atomic<std::int64_t>, kFrameHistory> g_frameTimes{};
        std::atomic<std::uint64_t>                           g_frameCount{0};

        std::int64_t toNanoseconds(FrameClock::Clock::time_point time) noexcept
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
        }
    } // namespace

    void FrameClock::tick() noexcept
    {
        const std::uint64_t index = g_frameCount.load(std::memory_order_relaxed);
        g_frameTimes[index % kFrameHistory].store(toNanoseconds(Clock::now()), std::memory_order_relaxed);
        g_frameCount.store(index + 1, std::memory_order_release);
    }

    bool FrameClock::running() noexcept
    {
        return g_frameCount.load(std::memory_order_acquire) != 0;
    }

    FrameClock::Window FrameClock::between(Clock::time_point start, Clock::time_point end) noexcept
    {
        const std::uint64_t count = g_frameCount.load(std::memory_order_acquire);
        const std::int64_t  from  = toNanoseconds(start);
        const std::int64_t  to    = toNanoseconds(end);

        // Walk from the newest frame back to start, measuring each gap.
        Window        window;
        std::int64_t  newer       = to;
        bool          reachedFrom = false;
        std::uint64_t index       = count;
        for (; index > 0 && count - index < kFrameHistory; --index)
        {
            const std::int64_t time = g_frameTimes[(index - 1) % kFrameHistory].load(std::memory_order_relaxed);
            if (time > to)
                continue;
            if (time < from)
            {
                reachedFrom = true;
                break;
            }
            ++window.frames;
            window.longestGap = std::max(window.longestGap, std::chrono::nanoseconds(newer - time));
            newer             = time;
        }
        if (reachedFrom || index == 0)
            window.longestGap = std::max(window.longestGap, std::chrono::nanoseconds(newer - from));
        return window;
    }

#if defined(_WIN32)
    std::optional<Duration> detail::processCpuTime() noexcept
    {
        FILETIME creation, exit, kernel, user;
        if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
            return std::nullopt;
        const auto ticks = [](const FILETIME& time) {
            return (static_cast<std::uint64_t>(time.dwHighDateTime) << 32) | time.dwLowDateTime;
        };
        return Duration(static_cast<double>(ticks(kernel) + ticks(user)) / 10'000.0);
    }

    std::optional<std::int64_t> detail::residentMemoryBytes() noexcept
    {
        PROCESS_MEMORY_COUNTERS counters{};
        if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
            return std::nullopt;
        return static_cast<std::int64_t>(counters.WorkingSetSize);
    }
#else
    std::optional<Duration> detail::processCpuTime() noexcept
    {
        rusage usage{};
        if (getrusage(RUSAGE_SELF, &usage) != 0)
            return std::nullopt;
        const auto toMs = [](const timeval& time) {
            return static_cast<double>(time.tv_sec) * 1'000.0 + static_cast<double>(time.tv_usec) / 1'000.0;
        };
        return Duration(toMs(usage.ru_utime) + toMs(usage.ru_stime));
    }

    std::optional<std::int64_t> detail::residentMemoryBytes() noexcept
    {
#if defined(__APPLE__)
        mach_task_basic_info_data_t info{};
        mach_msg_type_number_t      count = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS)
            return std::nullopt;
        return static_cast<std::int64_t>(info.resident_size);
#else
        // Second field of /proc/self/statm: resident pages.
        std::FILE* file = std::fopen("/proc/self/statm", "r");
        if (!file)
            return std::nullopt;
        long long  size     = 0;
        long long  resident = 0;
        const bool parsed   = std::fscanf(file, "%lld %lld", &size, &resident) == 2;
        std::fclose(file);
        if (!parsed)
            return std::nullopt;
        return static_cast<std::int64_t>(resident) * sysconf(_SC_PAGESIZE);
#endif
    }
#endif

    std::ostream& operator<<(std::ostream& stream, const Report& report)
    {
        const auto flags     = stream.flags();
        const auto precision = stream.precision();
        bool       first     = true;
        const auto separate  = [&]() -> std::ostream& {
            if (!first)
                stream << ", ";
            first = false;
            return stream;
        };

        stream << std::fixed << std::setprecision(2);
        if (report.time)
            separate() << "time " << report.time->count() << " ms";
        if (report.cpuTime)
            separate() << "cpu " << report.cpuTime->count() << " ms";
        if (report.cpuCores)
            separate() << "cores " << *report.cpuCores;
        if (report.ramBytes)
        {
            constexpr std::array units{"B", "KiB", "MiB", "GiB"};
            double               value = std::abs(static_cast<double>(*report.ramBytes));
            std::size_t          unit  = 0;
            for (; value >= 1024.0 && unit + 1 < units.size(); ++unit)
                value /= 1024.0;
            separate() << "ram " << (*report.ramBytes < 0 ? '-' : '+') << value << ' ' << units[unit];
        }
        if (report.gpu)
            separate() << "gpu " << *report.gpu << " %";
        if (report.longestFrame)
            separate() << "longest frame " << report.longestFrame->count() << " ms";
        if (first)
            stream << "no metrics";

        stream.flags(flags);
        stream.precision(precision);
        return stream;
    }

    std::string formatReport(const Report& report)
    {
        std::ostringstream stream;
        stream << report;
        return stream.str();
    }
} // namespace platform::benchmark
