#pragma once

#include <wx/wx.h>

namespace ui::toolbars
{
    /**
     * @brief Temporary toolbar used to validate the toolbar infrastructure.
     *
     * This toolbar exists solely to verify that the complete toolbar creation
     * pipeline is working correctly:
     *
     * - RegisterAllToolBars()
     * - ToolBarRegistry
     * - MainWindow::InitToolBars()
     * - visible wxToolBar attached to the window
     *
     * Once real application toolbars are implemented, this class can safely
     * be removed along with its registration.
     */
    class ExampleToolBar : public wxToolBar
    {
      public:
        /**
         * @brief Constructs the example toolbar.
         *
         * @param parent Parent window that owns the toolbar.
         *
         * Populates the toolbar with a few placeholder tools using stock
         * wxWidgets icons.
         */
        explicit ExampleToolBar(wxWindow* parent);
    };

} // namespace ui::toolbars
