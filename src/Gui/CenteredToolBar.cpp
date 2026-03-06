#include <Gui/CenteredToolbar.hpp>
#include <Gui/Panels/ScalePanel.hpp>
#include <Gui/States/ScalePanelState.hpp>
#include <Gui/Utils/FileDialog.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>
#include <iostream>

namespace gui
{

    CenteredToolbar::CenteredToolbar(float topMargin, float buttonSize, float spacing)
        : topMargin_(topMargin), buttonSize_(buttonSize), spacing_(spacing)
    { }

    void CenteredToolbar::initialize(renderer::IRenderer& renderer)
    {
        buttons_.clear();
        buttons_.reserve(5);

        // Import
        buttons_.emplace_back(Button::Builder("import")
                                  .icon([&renderer](void* c) { renderer.drawImportIcon(c); })
                                  .action([]() {
                                      auto path = gui::utils::pickSTLFile();
                                      if (path.has_value())
                                          scene::Scene::getInstance().addObject(*path);
                                  })
                                  .shortcut(std::vector<renderer::Key>{renderer::Key::Ctrl, renderer::Key::I}, "Import (I)")
                                  .build(renderer));

        // Export
        buttons_.emplace_back(Button::Builder("export")
                                  .icon([&renderer](void* c) { renderer.drawExportIcon(c); })
                                  .action([]() {
                                      auto path = gui::utils::saveSTLFile();
                                      if (!path.has_value())
                                          return;

                                      bool exported = scene::Scene::getInstance().exportSelected(*path);
                                      if (!exported)
                                          std::cout << "[Toolbar] Export failed: no object selected\n";
                                  })
                                  .shortcut(std::vector<renderer::Key>{renderer::Key::Ctrl, renderer::Key::E}, "Export (E)")
                                  .build(renderer));

        // Preview
        buttons_.emplace_back(Button::Builder("preview")
                                  .icon([&renderer](void* c) { renderer.drawPreviewIcon(c); })
                                  .action([]() { std::cout << "[Toolbar] Preview\n"; })
                                  .shortcut(std::vector<renderer::Key>{renderer::Key::Ctrl, renderer::Key::P}, "Preview (P)")
                                  .build(renderer));

        // Slice
        buttons_.emplace_back(Button::Builder("slice")
                                  .icon([&renderer](void* c) { renderer.drawSliceIcon(c); })
                                  .action([]() { std::cout << "[Toolbar] Slice\n"; })
                                  .shortcut(std::vector<renderer::Key>{renderer::Key::Ctrl, renderer::Key::L}, "Slice (L)")
                                  .build(renderer));

        // Scale (has panel)
        buttons_.emplace_back(Button::Builder("scale")
                                  .icon([&renderer](void* c) { renderer.drawScaleIcon(c); })
                                  .shortcut(std::vector<renderer::Key>{renderer::Key::Ctrl, renderer::Key::S}, "Scale (S)")
                                  .panel(panels::ScalePanel())
                                  .build(renderer));

        repositionButtons(renderer);
        _lastScreenWidth  = renderer.getScreenWidth();
        _lastScreenHeight = renderer.getScreenHeight();
    }

    void CenteredToolbar::update(renderer::IRenderer& renderer)
    {
        rebuildIfResized(renderer);

        const bool outsideClick = renderer.isMouseButtonPressed((int) renderer::MouseButton::Left) && [&]() {
            const auto mouse = renderer.getMousePosition();
            for (const auto& btn : buttons_)
            {
                // Inside button bounds
                if (mouse.x >= btn.x && mouse.x <= btn.x + btn.width &&
                    mouse.y >= btn.y && mouse.y <= btn.y + btn.height)
                    return false;

                // Inside panel bounds (if open)
                if (btn.isPanelOpen())
                {
                    float px = btn.x + (btn.width - Button::kPanelW) * 0.5f;
                    px       = std::max(4.0f, std::min(px, (float) renderer.getScreenWidth() - Button::kPanelW - 4.0f));
                    float py = btn.y + btn.height + Button::kPanelGap;

                    if (mouse.x >= px && mouse.x <= px + Button::kPanelW &&
                        mouse.y >= py && mouse.y <= py + Button::kPanelH)
                        return false;
                }
            }
            return true;
        }();
        for (size_t i = 0; i < buttons_.size(); ++i)
        {
            buttons_[i].update(renderer);

            if (buttons_[i].isPanelOpen())
            {
                for (size_t j = 0; j < buttons_.size(); ++j)
                    if (j != i)
                        buttons_[j].closePanel();
            }

            if (outsideClick)
                buttons_[i].closePanel();
        }
    }

    void CenteredToolbar::draw(const renderer::IRenderer& renderer) const
    {
        for (const auto& btn : buttons_)
            btn.draw(renderer);
    }

    void CenteredToolbar::rebuildIfResized(renderer::IRenderer& renderer)
    {
        if (renderer.getScreenWidth() != _lastScreenWidth ||
            renderer.getScreenHeight() != _lastScreenHeight)
        {
            repositionButtons(renderer);
            _lastScreenWidth  = renderer.getScreenWidth();
            _lastScreenHeight = renderer.getScreenHeight();
        }
    }

    void CenteredToolbar::repositionButtons(renderer::IRenderer& renderer)
    {
        const float totalWidth = (float) buttons_.size() * buttonSize_ + (float) (buttons_.size() - 1) * spacing_;
        const float startX     = ((float) renderer.getScreenWidth() - totalWidth) * 0.5f;

        for (size_t i = 0; i < buttons_.size(); ++i)
        {
            buttons_[i].x      = startX + (float) i * (buttonSize_ + spacing_);
            buttons_[i].y      = topMargin_;
            buttons_[i].width  = buttonSize_;
            buttons_[i].height = buttonSize_;
        }
    }

} // namespace gui
