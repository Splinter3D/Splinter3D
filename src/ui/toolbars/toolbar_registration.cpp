#include "toolbar_registration.hpp"

#include "toolbar_registry.hpp"
#include "transform_toolbar/transform_toolbar.hpp"
#include "ui/framework/wx/ids/ids.hpp"

// Add one include + one Register(...) line per real toolbar as they get built.

namespace ui::toolbars
{
    void RegisterAllToolBars()
    {
        // Retrieve the singleton registry responsible for toolbar factories.
        auto& registry = ToolBarRegistry::getInstance();

        registry.Register(
            ui::framework::wx::ids::toolbars::kTransform,
            [](wxWindow* parent) {
                return new TransformToolbar(parent);
            });
    }

} // namespace ui::toolbars
