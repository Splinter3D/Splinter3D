#include "platform/threading/dispatcher.hpp"
#include "support/temp_dir.hpp"

#include <array>
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <latch>
#include <stdexcept>
#include <string>
#include <thread>

using platform::benchmark::Duration;
using platform::benchmark::Report;
using platform::threading::Dispatcher;
using platform::threading::ScalabilityProfile;
using tests::support::TempDir;

namespace
{
    // A profile whose runs with `workers` workers kept `cores` cores busy.
    ScalabilityProfile profileOf(std::size_t workers, double cores)
    {
        // Fields assigned one by one: GCC's -Wmissing-field-initializers rejects
        // designated initializers that leave Report's other members out.
        Report report;
        report.time     = Duration(100.0);
        report.cpuTime  = Duration(100.0 * cores);
        report.cpuCores = cores;

        ScalabilityProfile profile;
        profile.record(workers, report, false);
        return profile;
    }

    // A submitted run releases its workers after fn returns; wait for that so
    // the next test starts with an idle Dispatcher.
    bool waitIdle(Dispatcher& dispatcher)
    {
        for (int i = 0; i < 5000 && dispatcher.freeWorkers() != dispatcher.total(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return dispatcher.freeWorkers() == dispatcher.total();
    }

    void writeFile(const std::string& path, const std::string& content)
    {
        std::ofstream(path) << content;
    }
} // namespace

TEST_CASE("Dispatcher: nproc is 1 outside run", "[platform][threading]")
{
    REQUIRE(NPROC == 1);
}

TEST_CASE("Dispatcher: unknown operation alone gets every worker", "[platform][threading]")
{
    auto&             dispatcher = Dispatcher::getInstance();
    const std::size_t budget     = dispatcher.run("t", [] { return NPROC; });
    REQUIRE(budget == dispatcher.total());
    REQUIRE(dispatcher.profile("t")->runs() == 1);
}

TEST_CASE("Dispatcher: known operation gets its preferred budget", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    dispatcher.setProfile("s", profileOf(4, 1.0)); // fully serial
    REQUIRE(dispatcher.run("s", [] { return NPROC; }) == 1);
}

TEST_CASE("Dispatcher: concurrent runs never exceed total", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    if (dispatcher.total() < 2)
        SKIP("needs at least two cores");

    std::latch  aInside(1);
    std::latch  bDone(1);
    std::size_t budgetA = 0;
    std::size_t budgetB = 0;

    std::thread a([&] {
        dispatcher.run("a", [&] {
            budgetA = NPROC;
            aInside.count_down();
            bDone.wait();
        });
    });
    aInside.wait();
    budgetB = dispatcher.run("b", [] { return NPROC; });
    bDone.count_down();
    a.join();

    // A took every worker; B only gets its own calling thread, never pool workers A holds.
    REQUIRE(budgetA == dispatcher.total());
    REQUIRE(budgetB == 1);
}

TEST_CASE("Dispatcher: nested run stays within parent budget", "[platform][threading]")
{
    auto&       dispatcher = Dispatcher::getInstance();
    std::size_t outer      = 0;
    std::size_t inner      = 0;
    dispatcher.run("outer", [&] {
        outer = NPROC;
        inner = dispatcher.run("inner", [] { return NPROC; });
    });
    REQUIRE(inner >= 1);
    REQUIRE(inner <= outer);
}

TEST_CASE("Dispatcher: NPROC on pool threads is the slice's share of the budget", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    if (dispatcher.total() < 2)
        SKIP("needs at least two cores");

    std::size_t                budget = 0;
    std::array<std::size_t, 2> shares{};
    std::atomic<int>           overBudget = 0;
    dispatcher.run("p", [&] {
        budget = NPROC;
        dispatcher.parallelFor(0, 2, [&](std::size_t i) {
            shares[i] = NPROC;
            // Work nested in a slice stays within that slice's share.
            if (dispatcher.run("p.inner", [] { return NPROC; }) > shares[i])
                ++overBudget;
        });
    });
    REQUIRE(shares[0] >= 1);
    REQUIRE(shares[1] >= 1);
    REQUIRE(shares[0] + shares[1] == budget);
    REQUIRE(overBudget == 0);
}

TEST_CASE("Dispatcher: throwing operation releases its budget and is not recorded", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    REQUIRE_THROWS_AS(dispatcher.run("x", []() -> int { throw std::runtime_error("boom"); }), std::runtime_error);
    REQUIRE(dispatcher.run("y", [] { return NPROC; }) == dispatcher.total());
    REQUIRE_FALSE(dispatcher.profile("x"));
}

TEST_CASE("Dispatcher: every 8th run explores", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    if (dispatcher.total() < 3)
        SKIP("needs at least three cores");

    const ScalabilityProfile profile   = profileOf(2, 1.25); // serial fraction 0.6
    const std::size_t        preferred = *profile.preferredWorkers(dispatcher.total());
    for (std::size_t i = 1; i <= Dispatcher::kExploreEvery; ++i)
    {
        dispatcher.setProfile("e", profile); // keep this test's own runs out of the estimate
        const std::size_t budget = dispatcher.run("e", [] { return NPROC; });
        if (i < Dispatcher::kExploreEvery)
            REQUIRE(budget == preferred);
        else
            REQUIRE(budget == (preferred < dispatcher.total() ? preferred + 1 : preferred - 1));
    }
}

TEST_CASE("Dispatcher: exploration skipped under a tighter cap keeps its direction", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    if (dispatcher.total() < 4)
        SKIP("needs at least four cores");

    const ScalabilityProfile profile   = profileOf(2, 1.25); // serial fraction 0.6, preferred 3
    const std::size_t        preferred = *profile.preferredWorkers(dispatcher.total());

    // Dispatch 8 (an exploration) runs nested in a parent capped at 2 workers, below the preferred 3.
    auto dispatch = [&] {
        dispatcher.setProfile("c", profile);
        return dispatcher.run("c", [] { return NPROC; });
    };
    for (std::size_t i = 1; i < Dispatcher::kExploreEvery; ++i)
        dispatch();

    dispatcher.setProfile("parent", profileOf(2, 2.0 / 1.8)); // serial fraction 0.8, preferred 2
    dispatcher.run("parent", [&] {
        REQUIRE(NPROC == 2);
        REQUIRE(dispatch() == 2);
    });

    // The wasted exploration did not use up the upward direction.
    for (std::size_t i = 1; i < Dispatcher::kExploreEvery; ++i)
        REQUIRE(dispatch() == preferred);
    REQUIRE(dispatch() == preferred + 1);
}

TEST_CASE("Dispatcher: save writes nothing without profiles", "[platform][threading]")
{
    auto&         dispatcher = Dispatcher::getInstance();
    const TempDir dir;
    const auto    path = dir.path() / "profiles.json";

    dispatcher.save(path);
    REQUIRE_FALSE(std::filesystem::exists(path));
}

TEST_CASE("Dispatcher: save() writes back to the loaded file, creating its folder", "[platform][threading]")
{
    auto&         dispatcher = Dispatcher::getInstance();
    const TempDir dir;
    const auto    path = dir.path() / "assets" / Dispatcher::kProfilesFile;

    dispatcher.load(path); // missing: nothing loaded, but the path is remembered
    dispatcher.run("w", [] { });
    dispatcher.save();

    REQUIRE(std::filesystem::exists(path));
    // Written through a temporary file that is renamed over the target.
    std::size_t entries = 0;
    for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator(path.parent_path()))
        ++entries;
    REQUIRE(entries == 1);

    dispatcher.clear();
    dispatcher.load(path);
    REQUIRE(dispatcher.profile("w"));
}

TEST_CASE("Dispatcher: save then load restores profiles", "[platform][threading]")
{
    auto&         dispatcher = Dispatcher::getInstance();
    const TempDir dir;
    const auto    path = (dir.path() / "profiles.json").string();

    dispatcher.run("r", [] { });
    dispatcher.run("r", [] { });
    dispatcher.save(path);
    dispatcher.clear();
    REQUIRE_FALSE(dispatcher.profile("r"));

    dispatcher.load(path);
    REQUIRE(dispatcher.profile("r"));
    REQUIRE(dispatcher.profile("r")->runs() == 2);
}

TEST_CASE("Dispatcher: bad profile files are ignored", "[platform][threading]")
{
    auto&         dispatcher = Dispatcher::getInstance();
    const TempDir dir;
    const auto    path = (dir.path() / "profiles.json").string();

    dispatcher.setProfile("old", profileOf(2, 2.0));
    REQUIRE_NOTHROW(dispatcher.load((dir.path() / "missing.json").string()));
    REQUIRE_FALSE(dispatcher.profile("old"));

    for (const std::string content : {"not json", R"({"version": 2})", "[]"})
    {
        dispatcher.setProfile("old", profileOf(2, 2.0));
        writeFile(path, content);
        REQUIRE_NOTHROW(dispatcher.load(path));
        REQUIRE_FALSE(dispatcher.profile("old"));
    }

    const std::string ok = profileOf(2, 2.0).toJson().dump();
    writeFile(path, R"({"version": 1, "operations": {"ok": )" + ok +
                        R"(, "bad": {"byWorkers": {"2": {"runs": "x"}}}}})");
    REQUIRE_NOTHROW(dispatcher.load(path));
    REQUIRE(dispatcher.profile("ok"));
    REQUIRE_FALSE(dispatcher.profile("bad"));
}

TEST_CASE("Dispatcher: freeWorkers is total when idle and drops during a run", "[platform][threading]")
{
    auto& dispatcher = Dispatcher::getInstance();
    REQUIRE(dispatcher.freeWorkers() == dispatcher.total());
    dispatcher.run("f", [&] { REQUIRE(dispatcher.freeWorkers() == dispatcher.total() - NPROC); });
    REQUIRE(dispatcher.freeWorkers() == dispatcher.total());
}

TEST_CASE("Dispatcher: submit runs fn under a budget on another thread", "[platform][threading]")
{
    auto&           dispatcher = Dispatcher::getInstance();
    std::latch      ran(1);
    std::thread::id thread;
    std::size_t     budget = 0;
    dispatcher.submit("s.async", [&] {
        thread = std::this_thread::get_id();
        budget = NPROC;
        ran.count_down();
    });
    ran.wait();
    REQUIRE(thread != std::this_thread::get_id());
    REQUIRE(budget >= 1);
    REQUIRE(waitIdle(dispatcher));
    REQUIRE(dispatcher.profile("s.async"));
}

TEST_CASE("Dispatcher: submit swallows exceptions", "[platform][threading]")
{
    auto&      dispatcher = Dispatcher::getInstance();
    std::latch ran(1);
    dispatcher.submit("s.throw", [] { throw std::runtime_error("boom"); });
    dispatcher.submit("s.after", [&] { ran.count_down(); });
    ran.wait();
    REQUIRE(waitIdle(dispatcher)); // the pool survived the throwing task
}

TEST_CASE("Dispatcher: submit's then runs once the budget is released", "[platform][threading]")
{
    auto&             dispatcher = Dispatcher::getInstance();
    std::latch        done(1);
    std::size_t       freeInThen = 0;
    std::atomic<bool> ranFn      = false;
    dispatcher.submit(
        "s.then", [&] { ranFn = true; },
        [&] {
            freeInThen = dispatcher.freeWorkers();
            done.count_down();
        });
    done.wait();
    REQUIRE(ranFn);
    REQUIRE(freeInThen == dispatcher.total());
    REQUIRE(waitIdle(dispatcher));
}
