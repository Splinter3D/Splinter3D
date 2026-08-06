#pragma once

#include "ui/layouts/base_layout/base_layout.hpp"
#include "ui/windows/base_window/base_window.hpp"

#include <memory>

namespace ui::windows
{
    // Proof-of-concept window: reuses the exact same registered menu,
    // toolbar, and panel as MainWindow, but wires them into a different
    // layout. Demonstrates that panels/toolbars/menus are layout-agnostic
    // and that layouts are swappable per-window.
    class DemoWindow : public BaseWindow
    {
      public:
        DemoWindow();

      protected:
        void InitMenuBar() override;
        void InitLayout() override;
        void InitToolBars() override;
        void InitPanels() override;

      private:
        std::unique_ptr<ui::layouts::BaseLayout> layout_;
    };
} // namespace ui::windows
