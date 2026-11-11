#include "platform/threading/scalability_profile.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <limits>
#include <nlohmann/json.hpp>

using Catch::Matchers::WithinAbs;
using platform::benchmark::Duration;
using platform::benchmark::Report;
using platform::threading::ScalabilityProfile;

namespace
{
    constexpr double kEps = 1e-9;

    // A run that took wallMs and kept `cores` cores busy on average.
    Report report(double cores, double wallMs = 100.0)
    {
        // Fields assigned one by one: GCC's -Wmissing-field-initializers rejects
        // designated initializers that leave Report's other members out.
        Report result;
        result.time     = Duration(wallMs);
        result.cpuTime  = Duration(wallMs * cores);
        result.cpuCores = cores;
        return result;
    }

    // A run that only reported its wall time.
    Report wallOnly(double wallMs)
    {
        Report result;
        result.time = Duration(wallMs);
        return result;
    }
} // namespace

TEST_CASE("ScalabilityProfile: no serial fraction without parallel samples", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(1, report(1.0), false);
    REQUIRE(profile.runs() == 1);
    REQUIRE_FALSE(profile.serialFraction());
    REQUIRE_FALSE(profile.preferredWorkers(8));
}

TEST_CASE("ScalabilityProfile: perfect scaling prefers all workers", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(4, report(4.0), false);
    REQUIRE_THAT(*profile.serialFraction(), WithinAbs(0.0, kEps));
    REQUIRE(profile.preferredWorkers(8) == 8u);
}

TEST_CASE("ScalabilityProfile: serial work prefers one worker", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(4, report(1.0), false);
    REQUIRE_THAT(*profile.serialFraction(), WithinAbs(1.0, kEps));
    REQUIRE(profile.preferredWorkers(8) == 1u);
}

TEST_CASE("ScalabilityProfile: diminishing returns cap the budget", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(2, report(1.25), false);
    REQUIRE_THAT(*profile.serialFraction(), WithinAbs(0.6, kEps));
    REQUIRE(profile.preferredWorkers(10) == 3u);
}

TEST_CASE("ScalabilityProfile: averages move by alpha", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(2, report(1.0, 100.0), false);
    profile.record(2, report(1.0, 200.0), false);
    REQUIRE_THAT(*profile.byWorkers().at(2).wallMs, WithinAbs(130.0, kEps));
    profile.record(2, report(1.0, 230.0), true);
    REQUIRE_THAT(*profile.byWorkers().at(2).wallMs, WithinAbs(140.0, kEps));
    REQUIRE(profile.byWorkers().at(2).runs == 3);
}

TEST_CASE("ScalabilityProfile: report without cpuCores only counts the run", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(4, wallOnly(50.0), false);
    REQUIRE(profile.runs() == 1);
    REQUIRE_FALSE(profile.serialFraction());
}

TEST_CASE("ScalabilityProfile: JSON round-trip", "[platform][threading]")
{
    ScalabilityProfile profile;
    Report             withRam = report(1.5, 80.0);
    withRam.ramBytes           = 4096;
    profile.record(2, withRam, false);
    profile.record(4, report(3.0, 40.0), false);

    const auto loaded = ScalabilityProfile::fromJson(profile.toJson(), 16);
    REQUIRE(loaded);
    REQUIRE(loaded->runs() == profile.runs());
    REQUIRE(loaded->byWorkers().size() == profile.byWorkers().size());
    for (const auto& [workers, stats] : profile.byWorkers())
    {
        const auto& other = loaded->byWorkers().at(workers);
        REQUIRE(other.runs == stats.runs);
        REQUIRE_THAT(*other.wallMs, WithinAbs(*stats.wallMs, kEps));
        REQUIRE_THAT(*other.cpuMs, WithinAbs(*stats.cpuMs, kEps));
        REQUIRE_THAT(*other.cores, WithinAbs(*stats.cores, kEps));
        REQUIRE(other.ramBytes == stats.ramBytes); // set for 2 workers only, empty for 4
    }
}

TEST_CASE("ScalabilityProfile: fromJson drops counts above maxWorkers", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(2, report(2.0), false);
    profile.record(32, report(20.0), false);

    const auto loaded = ScalabilityProfile::fromJson(profile.toJson(), 8);
    REQUIRE(loaded);
    REQUIRE(loaded->byWorkers().size() == 1);
    REQUIRE(loaded->byWorkers().contains(2));
}

TEST_CASE("ScalabilityProfile: malformed JSON yields nullopt", "[platform][threading]")
{
    REQUIRE_FALSE(ScalabilityProfile::fromJson(nlohmann::json::parse(R"({"byWorkers": {"2": {"runs": "x"}}})"), 8));
    REQUIRE_FALSE(ScalabilityProfile::fromJson(nlohmann::json(42), 8));
}

TEST_CASE("ScalabilityProfile: a run without cpuCores does not bias the next one", "[platform][threading]")
{
    ScalabilityProfile profile;
    profile.record(4, wallOnly(50.0), false);
    profile.record(4, report(4.0), false);
    REQUIRE_THAT(*profile.byWorkers().at(4).cores, WithinAbs(4.0, kEps));
    REQUIRE_THAT(*profile.serialFraction(), WithinAbs(0.0, kEps));
}

TEST_CASE("ScalabilityProfile: negative run counts are malformed", "[platform][threading]")
{
    REQUIRE_FALSE(ScalabilityProfile::fromJson(nlohmann::json::parse(R"({"byWorkers": {"2": {"runs": -5}}})"), 8));
}

TEST_CASE("ScalabilityProfile: non-finite samples are ignored", "[platform][threading]")
{
    Report broken   = report(2.0);
    broken.cpuCores = std::numeric_limits<double>::quiet_NaN();
    broken.time     = Duration(std::numeric_limits<double>::infinity());

    ScalabilityProfile profile;
    profile.record(2, broken, false);
    REQUIRE(profile.runs() == 1);
    REQUIRE_FALSE(profile.byWorkers().at(2).cores);
    REQUIRE_FALSE(profile.byWorkers().at(2).wallMs);

    // Still loadable after a save.
    REQUIRE(ScalabilityProfile::fromJson(nlohmann::json::parse(profile.toJson().dump()), 8));
}
