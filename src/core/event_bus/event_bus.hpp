#pragma once

#include "core/utils/singleton.hpp"
#include "platform/logger/logger.hpp"

#include <functional>
#include <source_location>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace core::event_bus
{
    /**
     * @brief A type-erased publish/subscribe event bus.
     *
     * EventBus decouples layers that must react to events (typically
     * ui/) from the layers that produce them (typically application/ or
     * platform/), without requiring either side to know about the other
     * directly - matching the role described in core/README.md.
     *
     * EventBus is a Singleton: there is exactly one instance shared
     * across the whole application.
     *
     * Every subscription and publication is logged with its call site
     * (file, line, function), which is useful for tracing event flow
     * during development.
     *
     * @warning Handlers are stored per exact type (via std::type_index).
     * Publishing an event of a derived type will not trigger handlers
     * subscribed to a base type.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * struct ThemeChanged { bool dark; };
     *
     * core::event_bus::EventBus::getInstance().subscribe<ThemeChanged>(
     *     [](const ThemeChanged& e) {
     *         wxLogMessage("Theme changed, dark=%d", e.dark);
     *     });
     *
     * core::event_bus::EventBus::getInstance().publish(ThemeChanged{true});
     * @endcode
     */
    class EventBus : public utils::Singleton<EventBus>
    {
      public:
        /**
         * @brief Subscribes a handler to a given event type.
         *
         * @tparam Event The event type to listen for.
         * @param handler Callback invoked whenever an event of this type
         * is published.
         * @param loc Call site of the subscription, captured automatically
         * for logging purposes. Should not be provided manually.
         */
        template <typename Event>
        void subscribe(std::function<void(const Event&)> handler, std::source_location loc = std::source_location::current())
        {
            platform::logger::Logger::getInstance().clog(
                "[EventBus] Subscribing to ", typeid(Event).name(),
                " from ", loc.file_name(), ":", loc.line(),
                " (", loc.function_name(), ")\n");
            _handlers[typeid(Event)].push_back(
                [handler](const void* e) { handler(*static_cast<const Event*>(e)); });
        }

        /**
         * @brief Publishes an event to all subscribed handlers.
         *
         * @tparam Event The event type being published.
         * @param event The event instance to dispatch. It is passed by
         * const reference to every matching handler, in subscription order.
         * @param loc Call site of the publication, captured automatically
         * for logging purposes. Should not be provided manually.
         */
        template <typename Event>
        void publish(const Event& event, std::source_location loc = std::source_location::current())
        {
            platform::logger::Logger::getInstance().clog(
                "[EventBus] Publishing ", typeid(Event).name(),
                " from ", loc.file_name(), ":", loc.line(),
                " (", loc.function_name(), ")\n");
            auto it = _handlers.find(typeid(Event));
            if (it != _handlers.end())
                for (auto& h : it->second)
                    h(&event);
        }

      protected:
        EventBus()           = default;
        ~EventBus() noexcept = default;

      private:
        friend class Singleton<EventBus>;
        std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> _handlers;
    };
} // namespace core::event_bus
