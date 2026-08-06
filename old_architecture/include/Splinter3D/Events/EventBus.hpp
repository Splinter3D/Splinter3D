#pragma once

#include <Splinter3D/Utils/Logger.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <functional>
#include <source_location>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace splinter3D::events
{
    class EventBus : public splinter3D::utils::Singleton<EventBus>
    {
      public:
        template <typename Event>
        void subscribe(std::function<void(const Event&)> handler, std::source_location loc = std::source_location::current())
        {
            splinter3D::utils::Logger::getInstance().clog(
                "[EventBus] Subscribing to ", typeid(Event).name(),
                " from ", loc.file_name(), ":", loc.line(),
                " (", loc.function_name(), ")\n");
            _handlers[typeid(Event)].push_back(
                [handler](const void* e) { handler(*static_cast<const Event*>(e)); });
        }

        template <typename Event>
        void publish(const Event& event, std::source_location loc = std::source_location::current())
        {
            splinter3D::utils::Logger::getInstance().clog(
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
} // namespace splinter3D::events
