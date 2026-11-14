#pragma once

#include "platform/benchmark/benchmark.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>

namespace platform::threading
{
    /**
     * @brief Running averages of the runs made with one worker count. An
     * average stays empty until a run reports that metric, so runs that
     * could not measure it don't drag it towards 0.
     */
    struct WorkerStats
    {
        std::size_t                 runs = 0;
        std::optional<double>       wallMs;
        std::optional<double>       cpuMs;
        std::optional<double>       cores;
        std::optional<std::int64_t> ramBytes;
    };

    /**
     * @brief What one operation's past runs say about how well it scales.
     *
     * Runs are grouped by the worker count they were given. The serial
     * fraction comes from the Karp–Flatt metric, taking the average cores
     * busy (CPU time / wall time) as the speedup; the preferred worker count
     * is where Amdahl's law says one more worker would add less than
     * kMinGain speedup.
     */
    class ScalabilityProfile final
    {
      public:
        /** @brief Weight of a new sample in the averages. */
        static constexpr double kAlpha = 0.3;
        /** @brief Same, for a run that shared the CPU with another operation. */
        static constexpr double kContendedAlpha = 0.1;
        /** @brief Smallest speedup one more worker must add to be worth it. */
        static constexpr double kMinGain = 0.1;

        /**
         * @brief Folds one run made with `workers` workers into the averages.
         * Fields missing from the report leave their average unchanged.
         */
        void record(std::size_t workers, const benchmark::Report& report, bool contended);

        /** @brief Runs recorded, all worker counts together. */
        [[nodiscard]] std::size_t runs() const noexcept;

        [[nodiscard]] const std::map<std::size_t, WorkerStats>& byWorkers() const noexcept;

        /**
         * @brief Estimated share of the work that does not parallelize, in
         * [0, 1]; empty until a run with more than one worker reported its
         * cores.
         */
        [[nodiscard]] std::optional<double> serialFraction() const noexcept;

        /** @brief Best worker count in [1, total]; empty while serialFraction() is. */
        [[nodiscard]] std::optional<std::size_t> preferredWorkers(std::size_t total) const noexcept;

        [[nodiscard]] nlohmann::json toJson() const;

        /**
         * @brief Reads what toJson() wrote; empty when the node is malformed.
         * Worker counts of 0 or above maxWorkers (another machine) are dropped.
         */
        [[nodiscard]] static std::optional<ScalabilityProfile> fromJson(const nlohmann::json& node,
                                                                        std::size_t           maxWorkers);

      private:
        std::map<std::size_t, WorkerStats> _byWorkers;
    };
} // namespace platform::threading
