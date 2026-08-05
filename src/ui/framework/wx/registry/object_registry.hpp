#pragma once

#include "core/utils/singleton.hpp"

#include <functional>
#include <unordered_map>
#include <utility>

namespace ui::framework::wx::registry
{
    /**
     * @brief Generic registry for standalone object factories.
     *
     * The registry maps an integer identifier to a factory function capable
     * of constructing an object of type `ObjectT`.
     *
     * Unlike WidgetRegistry, this registry is intended for objects that are
     * created without a parent window, such as wxMenu.
     *
     * Registrations are typically performed once during application bootstrap,
     * while object creation happens later when the UI requires an instance.
     *
     * This class is purely infrastructure and has no knowledge of the
     * application's domain. Higher-level aliases (for example MenuRegistry)
     * should be declared alongside the feature they belong to.
     *
     * @tparam ObjectT Type created by this registry.
     */
    template <typename ObjectT>
    class ObjectRegistry : public core::utils::Singleton<ObjectRegistry<ObjectT>>
    {
      public:
        /**
         * @brief Factory function creating an object.
         */
        using Factory = std::function<ObjectT*()>;

        /**
         * @brief Registers an object factory.
         *
         * If another factory already exists for the same identifier,
         * it is replaced.
         *
         * Registration is usually performed once during application startup.
         *
         * @param id Unique identifier associated with the object.
         * @param factory Factory responsible for constructing the object.
         */
        void Register(int id, Factory factory)
        {
            factories_[id] = std::move(factory);
        }

        /**
         * @brief Creates an object from its registered identifier.
         *
         * Returns nullptr if no factory has been registered for the given id.
         *
         * @param id Identifier of the object to create.
         *
         * @return Newly created object, or nullptr if the id is unknown.
         */
        ObjectT* Create(int id) const
        {
            const auto it = factories_.find(id);
            if (it == factories_.end())
            {
                return nullptr;
            }

            return it->second();
        }

        /**
         * @brief Checks whether an object identifier is registered.
         *
         * @param id Object identifier.
         *
         * @return True if a factory exists for this identifier.
         */
        [[nodiscard]] bool IsRegistered(int id) const
        {
            return factories_.find(id) != factories_.end();
        }

      private:
        std::unordered_map<int, Factory> factories_;
    };

} // namespace ui::framework::wx::registry
