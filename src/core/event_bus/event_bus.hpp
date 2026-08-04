#pragma once

#include "core/utils/singleton.hpp"
#include "platform/logger/logger.hpp"

#include <functional>
#include <source_location>
#include <typeindex>
#include <unordered_map>
#include <vector>

using namespace platform::logger;

namespace core::event_bus
{
    class EventBus : public utils::Singleton<EventBus>
    {
      public:
        template <typename Event>
        void subscribe(std::function<void(const Event&)> handler, std::source_location loc = std::source_location::current())
        {
            Logger::getInstance().clog(
                "[EventBus] Subscribing to ", typeid(Event).name(),
                " from ", loc.file_name(), ":", loc.line(),
                " (", loc.function_name(), ")\n");
            _handlers[typeid(Event)].push_back(
                [handler](const void* e) { handler(*static_cast<const Event*>(e)); });
        }

        template <typename Event>
        void publish(const Event& event, std::source_location loc = std::source_location::current())
        {
            Logger::getInstance().clog(
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
