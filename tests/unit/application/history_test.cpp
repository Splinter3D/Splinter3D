#include "application/commands/history.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <string>
#include <vector>

using application::commands::History;
using application::service::SceneService;

namespace
{
    // A scene of models with these ids, each moved to x = its id * 10 + step.
    History::State state(const std::vector<application::service::ModelId>& ids, double step = 0)
    {
        History::State result;
        for (const auto id : ids)
        {
            SceneService::Model model;
            model.id              = id;
            model.name            = "model " + std::to_string(id);
            model.transform.moveX = static_cast<double>(id) * 10 + step;
            result.models.push_back(model);
        }
        result.visibility.assign(ids.size(), true);
        return result;
    }

    std::vector<double> moves(const History::State& state)
    {
        std::vector<double> result;
        for (const auto& model : state.models)
            result.push_back(model.transform.moveX);
        return result;
    }
} // namespace

TEST_CASE("History: starts with nothing to undo or redo", "[application][history]")
{
    History history;
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.canRedo());
    CHECK_FALSE(history.undo(state({1})));
    CHECK_FALSE(history.redo(state({1})));
}

TEST_CASE("History: undo hands back earlier states, redo the later ones", "[application][history]")
{
    History history;
    history.record(state({}));  // then model 1 was added
    history.record(state({1})); // then model 2

    auto back = history.undo(state({1, 2}));
    REQUIRE(back);
    CHECK(back->models.size() == 1);
    back = history.undo(*back);
    REQUIRE(back);
    CHECK(back->models.empty());
    CHECK_FALSE(history.canUndo());
    CHECK(history.canRedo());

    auto forward = history.redo(*back);
    REQUIRE(forward);
    CHECK(forward->models.size() == 1);
    forward = history.redo(*forward);
    REQUIRE(forward);
    CHECK(forward->models.size() == 2);
    CHECK_FALSE(history.canRedo());
    CHECK(history.canUndo());
}

TEST_CASE("History: a new edit forgets what could be redone", "[application][history]")
{
    History history;
    history.record(state({}));
    REQUIRE(history.undo(state({1})));
    REQUIRE(history.canRedo());

    history.record(state({}));
    CHECK_FALSE(history.canRedo());
}

TEST_CASE("History: edits with the same merge key close together are one step", "[application][history]")
{
    History    history;
    const auto start = History::Clock::now();

    history.record(state({1}, 0), "move:1", start);
    history.record(state({1}, 1), "move:1", start + std::chrono::milliseconds(400));
    history.record(state({1}, 2), "move:1", start + std::chrono::milliseconds(800)); // the window slides

    auto back = history.undo(state({1}, 3));
    REQUIRE(back);
    CHECK(moves(*back) == std::vector<double>{10}); // back to before the first
    CHECK_FALSE(history.canUndo());
}

TEST_CASE("History: a pause, another key or an undo starts a new step", "[application][history]")
{
    History    history;
    const auto start = History::Clock::now();

    SECTION("a pause")
    {
        history.record(state({1}, 0), "move:1", start);
        history.record(state({1}, 1), "move:1", start + History::kMergeWindow);
    }
    SECTION("another key")
    {
        history.record(state({1}, 0), "move:1", start);
        history.record(state({1}, 1), "move:2", start);
    }
    SECTION("no key")
    {
        history.record(state({1}, 0), "", start);
        history.record(state({1}, 1), "", start);
    }
    SECTION("an undo in between")
    {
        history.record(state({1}, 0), "move:1", start);
        history.record(state({1}, 5), "move:1", start); // merged
        REQUIRE(history.undo(state({1}, 1)));
        REQUIRE(history.redo(state({1}, 0)));
        history.record(state({1}, 1), "move:1", start);
    }

    auto back = history.undo(state({1}, 2));
    REQUIRE(back);
    CHECK(moves(*back) == std::vector<double>{11});
    back = history.undo(*back);
    REQUIRE(back);
    CHECK(moves(*back) == std::vector<double>{10});
}

TEST_CASE("History: keeps at most kLimit steps, dropping the oldest", "[application][history]")
{
    History history;
    for (std::size_t i = 0; i <= History::kLimit; ++i)
        history.record(state({1}, static_cast<double>(i)));

    std::size_t steps = 0;
    auto        last  = state({1}, -1);
    while (auto back = history.undo(last))
    {
        last = *back;
        ++steps;
    }
    CHECK(steps == History::kLimit);
    CHECK(moves(last) == std::vector<double>{11}); // step 0 was dropped
}

TEST_CASE("History: states keep no B-Rep, and a visibility per model", "[application][history]")
{
    History history;
    auto    before = state({1, 2});
    before.models[0].brep.emplace();
    before.visibility.clear();
    history.record(before);

    const auto back = history.undo(state({1, 2}));
    REQUIRE(back);
    CHECK_FALSE(back->models[0].brep);
    CHECK(back->visibility == std::vector<bool>{true, true});
}

TEST_CASE("History: clear forgets everything", "[application][history]")
{
    History history;
    history.record(state({}));
    REQUIRE(history.undo(state({1})));
    history.record(state({}));
    history.clear();
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.canRedo());
}
