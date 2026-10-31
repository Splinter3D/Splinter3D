#pragma once

#include "core/utils/singleton.hpp"

#include <functional>
#include <unordered_map>
#include <utility>
#include <wx/window.h>

namespace ui::framework::wx::registry
{
    /**
     * @brief Generic registry for wxWidgets widget factories.
     *
     * The registry maps an integer identifier to a factory function capable
     * of constructing a widget of type `WidgetT`.
     *
     * It is intended for infrastructure-level registration of UI components
     * (panels, custom controls, pages, etc.) whose constructor follows the
     * standard wxWidgets pattern:
     *
     * @code{.cpp}
     * WidgetT* widget = new WidgetT(parent);
     * @endcode
     *
     * Registrations are typically performed once during application bootstrap,
     * while widget creation happens later when the UI needs to instantiate
     * the registered component.
     *
     * This class deliberately knows nothing about the application's domain.
     * Higher-level aliases (for example `PanelRegistry`) should live alongside
     * the feature they belong to.
     *
     * @tparam WidgetT Base type created by this registry. Must derive from
     *                 wxWindow.
     */
    template <typename WidgetT>
    class WidgetRegistry : public core::utils::Singleton<WidgetRegistry<WidgetT>>
    {
      public:
        /**
         * @brief Factory function creating a widget.
         *
         * The parent window is provided by the caller.
         */
        using Factory = std::function<WidgetT*(::wxWindow* parent)>;

        /**
         * @brief Registers a widget factory.
         *
         * If another factory already exists for the same identifier,
         * it is replaced.
         *
         * Registration is usually performed once during application startup.
         *
         * @param id Unique identifier associated with the widget.
         * @param factory Factory responsible for constructing the widget.
         */
        void Register(int id, Factory factory)
        {
            factories_[id] = std::move(factory);
        }

        /**
         * @brief Creates a widget from its registered identifier.
         *
         * Returns nullptr if no factory has been registered for the given id.
         *
         * @param id Identifier of the widget to create.
         * @param parent Parent window owning the new widget.
         *
         * @return Newly created widget, or nullptr if the id is unknown.
         */
        WidgetT* Create(int id, ::wxWindow* parent) const
        {
            const auto it = factories_.find(id);
            if (it == factories_.end())
            {
                return nullptr;
            }

            return it->second(parent);
        }

        /**
         * @brief Checks whether a widget identifier is registered.
         *
         * @param id Widget identifier.
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
