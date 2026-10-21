#include "core/utils/non_copyable.hpp"
#include "core/utils/singleton.hpp"

#include <catch2/catch_test_macros.hpp>
#include <type_traits>

namespace
{
    class Counter : public core::utils::Singleton<Counter>
    {
      public:
        int value = 0;

      private:
        Counter() = default;
        friend class core::utils::Singleton<Counter>;
    };

    class Resource : public core::utils::NonCopyable
    { };
} // namespace

static_assert(!std::is_copy_constructible_v<Resource>);
static_assert(!std::is_copy_assignable_v<Resource>);
static_assert(!std::is_move_constructible_v<Resource>);
static_assert(!std::is_move_assignable_v<Resource>);

static_assert(!std::is_copy_constructible_v<Counter>);
static_assert(!std::is_default_constructible_v<Counter>, "Singletons must only be reachable through getInstance()");

TEST_CASE("Singleton: getInstance() always returns the same object", "[core][singleton]")
{
    auto& first  = Counter::getInstance();
    auto& second = Counter::getInstance();

    REQUIRE(&first == &second);

    first.value = 42;
    REQUIRE(second.value == 42);
}
