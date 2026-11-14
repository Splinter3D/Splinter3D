#include "application/service/selection.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

using application::service::ModelId;
using application::service::Selection;

namespace
{
    struct CountingSelection
    {
        CountingSelection()
        {
            selection.setOnChanged([this] { ++changes; });
        }

        Selection selection;
        int       changes = 0;
    };
} // namespace

TEST_CASE("Selection: selectOnly replaces the selection", "[unit][selection]")
{
    Selection selection;
    selection.selectOnly(1);
    selection.selectOnly(2);
    CHECK(selection.ids() == std::vector<ModelId>{2});
    CHECK(selection.contains(2));
    CHECK_FALSE(selection.contains(1));
}

TEST_CASE("Selection: toggle adds and removes", "[unit][selection]")
{
    Selection selection;
    selection.toggle(1);
    selection.toggle(2);
    selection.toggle(1);
    CHECK(selection.ids() == std::vector<ModelId>{2});
    CHECK(selection.size() == 1);
}

TEST_CASE("Selection: notifies only when the set changes", "[unit][selection]")
{
    CountingSelection s;
    s.selection.clear();
    CHECK(s.changes == 0);

    s.selection.selectOnly(3);
    s.selection.selectOnly(3);
    CHECK(s.changes == 1);

    CHECK_FALSE(s.selection.remove(9));
    CHECK(s.changes == 1);
    CHECK(s.selection.remove(3));
    CHECK(s.changes == 2);
    CHECK(s.selection.empty());
}

TEST_CASE("Selection: retainOnly drops models that no longer exist", "[unit][selection]")
{
    CountingSelection s;
    s.selection.toggle(1);
    s.selection.toggle(2);
    s.selection.toggle(3);
    s.changes = 0;

    s.selection.retainOnly({2, 5});
    CHECK(s.selection.ids() == std::vector<ModelId>{2});
    CHECK(s.changes == 1);

    s.selection.retainOnly({2});
    CHECK(s.changes == 1);
}
