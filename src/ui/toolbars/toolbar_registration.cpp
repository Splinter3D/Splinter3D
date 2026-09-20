#include "toolbar_registration.hpp"

#include "toolbar_object/toolbar_object.hpp"
#include "toolbar_registry.hpp"
#include "ui/framework/wx/ids/ids.hpp"

// Add one include + one Register(...) line per real toolbar as they get built.

namespace ui::toolbars
{
    void RegisterAllToolBars()
    {
        // Retrieve the singleton registry responsible for toolbar factories.
        auto& registry = ToolBarRegistry::getInstance();

        // Register the example toolbar.
        //
        // Each toolbar is identified by a unique id and associated with a
        // factory capable of constructing a fresh wxToolBar instance
        // whenever a window requests it.
        registry.Register(
            ui::framework::wx::ids::toolbars::kObject,
            [](wxWindow* parent) {
                return new ToolBarObject(parent);
            });
    }

} // namespace ui::toolbars
