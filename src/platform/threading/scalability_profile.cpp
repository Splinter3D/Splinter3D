#include "scalability_profile.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>

namespace platform::threading
{
    namespace
    {
        // Amdahl's speedup with p workers for serial fraction s.
        double amdahl(double s, std::size_t p) noexcept
        {
            return 1.0 / (s + (1.0 - s) / static_cast<double>(p));
        }

        void blend(std::optional<double>& average, double sample, double alpha) noexcept
        {
            if (!std::isfinite(sample))
                return;
            average = average ? *average + alpha * (sample - *average) : sample;
        }

        template <typename T>
        std::optional<T> optionalField(const nlohmann::json& node, const char* key)
        {
            const auto found = node.find(key);
            if (found == node.end())
                return std::nullopt;
            return found->template get<T>();
        }
    } // namespace

    void ScalabilityProfile::record(std::size_t workers, const benchmark::Report& report, bool contended)
    {
        WorkerStats& stats = _byWorkers[std::max<std::size_t>(workers, 1)];
        const double alpha = contended ? kContendedAlpha : kAlpha;

        if (report.time)
            blend(stats.wallMs, report.time->count(), alpha);
        if (report.cpuTime)
            blend(stats.cpuMs, report.cpuTime->count(), alpha);
        if (report.cpuCores)
            blend(stats.cores, *report.cpuCores, alpha);
        if (report.ramBytes)
        {
            std::optional<double> ram;
            if (stats.ramBytes)
                ram = static_cast<double>(*stats.ramBytes);
            blend(ram, static_cast<double>(*report.ramBytes), alpha);
            if (ram)
                stats.ramBytes = std::llround(*ram);
        }
        ++stats.runs;
    }

    std::size_t ScalabilityProfile::runs() const noexcept
    {
        std::size_t total = 0;
        for (const auto& [workers, stats] : _byWorkers)
            total += stats.runs;
        return total;
    }

    const std::map<std::size_t, WorkerStats>& ScalabilityProfile::byWorkers() const noexcept
    {
        return _byWorkers;
    }

    std::optional<double> ScalabilityProfile::serialFraction() const noexcept
    {
        double weighted = 0.0;
        double weights  = 0.0;
        for (const auto& [workers, stats] : _byWorkers)
        {
            if (workers < 2 || !stats.cores || *stats.cores <= 0.0)
                continue;
            const auto   p       = static_cast<double>(workers);
            const double speedup = std::clamp(*stats.cores, 1.0, p);
            const double s       = std::clamp((1.0 / speedup - 1.0 / p) / (1.0 - 1.0 / p), 0.0, 1.0);
            weighted += s * static_cast<double>(stats.runs);
            weights += static_cast<double>(stats.runs);
        }
        if (weights <= 0.0)
            return std::nullopt;
        return weighted / weights;
    }

    std::optional<std::size_t> ScalabilityProfile::preferredWorkers(std::size_t total) const noexcept
    {
        const auto s = serialFraction();
        if (!s)
            return std::nullopt;
        std::size_t p = 1;
        while (p < total && amdahl(*s, p + 1) - amdahl(*s, p) >= kMinGain)
            ++p;
        return p;
    }

    nlohmann::json ScalabilityProfile::toJson() const
    {
        nlohmann::json byWorkers = nlohmann::json::object();
        for (const auto& [workers, stats] : _byWorkers)
        {
            nlohmann::json entry = {{"runs", stats.runs}};
            if (stats.wallMs)
                entry["wallMs"] = *stats.wallMs;
            if (stats.cpuMs)
                entry["cpuMs"] = *stats.cpuMs;
            if (stats.cores)
                entry["cores"] = *stats.cores;
            if (stats.ramBytes)
                entry["ramBytes"] = *stats.ramBytes;
            byWorkers[std::to_string(workers)] = std::move(entry);
        }

        nlohmann::json node = {{"runs", runs()}, {"byWorkers", std::move(byWorkers)}};
        if (const auto s = serialFraction())
            node["serialFraction"] = *s;
        return node;
    }

    std::optional<ScalabilityProfile> ScalabilityProfile::fromJson(const nlohmann::json& node, std::size_t maxWorkers)
    {
        try
        {
            ScalabilityProfile profile;
            for (const auto& [key, value] : node.at("byWorkers").items())
            {
                const std::size_t workers = std::stoull(key);
                if (!value.at("runs").is_number_unsigned())
                    return std::nullopt;
                WorkerStats stats{
                    .runs     = value.at("runs").get<std::size_t>(),
                    .wallMs   = optionalField<double>(value, "wallMs"),
                    .cpuMs    = optionalField<double>(value, "cpuMs"),
                    .cores    = optionalField<double>(value, "cores"),
                    .ramBytes = optionalField<std::int64_t>(value, "ramBytes"),
                };
                if (workers == 0 || workers > maxWorkers)
                    continue;
                profile._byWorkers[workers] = stats;
            }
            return profile;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }
} // namespace platform::threading
