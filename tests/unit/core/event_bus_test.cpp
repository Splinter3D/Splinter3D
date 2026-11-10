#include "core/event_bus/event_bus.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

// EventBus is a process-wide singleton; tests/support/reset_globals_listener.cpp clears
// it around every test case so handlers never leak from one case into another.

namespace
{
    struct PublishWithoutSubscriberEvent
    { };

    struct OrderEvent
    {
        int value;
    };

    struct TypeIsolationA
    { };

    struct TypeIsolationB
    { };

    struct BaseEvent
    { };

    struct DerivedEvent : BaseEvent
    { };
} // namespace

TEST_CASE("EventBus: publishing with no subscriber does nothing", "[core][event_bus]")
{
    auto& bus = core::event_bus::EventBus::getInstance();
    REQUIRE_NOTHROW(bus.publish(PublishWithoutSubscriberEvent{}));
}

TEST_CASE("EventBus: handlers receive the payload in subscription order", "[core][event_bus]")
{
    auto&            bus = core::event_bus::EventBus::getInstance();
    std::vector<int> calls;

    bus.subscribe<OrderEvent>([&calls](const OrderEvent& e) { calls.push_back(e.value); });
    bus.subscribe<OrderEvent>([&calls](const OrderEvent& e) { calls.push_back(e.value * 10); });

    bus.publish(OrderEvent{4});

    REQUIRE(calls == std::vector<int>{4, 40});
}

TEST_CASE("EventBus: handlers only fire for their own event type", "[core][event_bus]")
{
    auto& bus    = core::event_bus::EventBus::getInstance();
    int   aCalls = 0;
    int   bCalls = 0;

    bus.subscribe<TypeIsolationA>([&aCalls](const TypeIsolationA&) { ++aCalls; });
    bus.subscribe<TypeIsolationB>([&bCalls](const TypeIsolationB&) { ++bCalls; });

    bus.publish(TypeIsolationA{});
    bus.publish(TypeIsolationA{});

    CHECK(aCalls == 2);
    CHECK(bCalls == 0);
}

TEST_CASE("EventBus: a derived event does not reach base-type handlers", "[core][event_bus]")
{
    auto& bus       = core::event_bus::EventBus::getInstance();
    int   baseCalls = 0;

    bus.subscribe<BaseEvent>([&baseCalls](const BaseEvent&) { ++baseCalls; });

    bus.publish(DerivedEvent{});

    REQUIRE(baseCalls == 0);
}

TEST_CASE("EventBus: clear() drops every handler", "[core][event_bus]")
{
    auto& bus   = core::event_bus::EventBus::getInstance();
    int   calls = 0;

    bus.subscribe<OrderEvent>([&calls](const OrderEvent&) { ++calls; });
    bus.subscribe<TypeIsolationA>([&calls](const TypeIsolationA&) { ++calls; });
    REQUIRE(bus.subscriberCount<OrderEvent>() == 1);

    bus.clear();
    bus.publish(OrderEvent{1});
    bus.publish(TypeIsolationA{});

    CHECK(calls == 0);
    CHECK(bus.subscriberCount<OrderEvent>() == 0);
    CHECK(bus.subscriberCount<TypeIsolationA>() == 0);
}

TEST_CASE("EventBus: test cases start with no subscribers", "[core][event_bus]")
{
    // Earlier test cases subscribed to OrderEvent; the reset listener must have dropped them.
    REQUIRE(core::event_bus::EventBus::getInstance().subscriberCount<OrderEvent>() == 0);
}
