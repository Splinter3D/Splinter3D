#include <Gui/CenteredToolbar.hpp>
#include <Gui/States/ResizePanelState.hpp>
#include <Renderer/IRenderer.hpp>
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

        // ── Import ────────────────────────────────────────────────────────────
        buttons_.emplace_back(Button(
            "import",
            []() { std::cout << "[Toolbar] Import\n"; },
            [&renderer](void* c) { renderer.drawImportIcon(c); },
            renderer,
            true, "Import (I)", renderer::Key::I));

        // ── Export ────────────────────────────────────────────────────────────
        buttons_.emplace_back(Button(
            "export",
            []() { std::cout << "[Toolbar] Export\n"; },
            [&renderer](void* c) { renderer.drawExportIcon(c); },
            renderer,
            true, "Export (E)", renderer::Key::E));

        // ── Preview ───────────────────────────────────────────────────────────
        buttons_.emplace_back(Button(
            "preview",
            []() { std::cout << "[Toolbar] Preview\n"; },
            [&renderer](void* c) { renderer.drawPreviewIcon(c); },
            renderer,
            true, "Preview (P)", renderer::Key::P));

        // ── Slice ─────────────────────────────────────────────────────────────
        buttons_.emplace_back(Button(
            "slice",
            []() { std::cout << "[Toolbar] Slice\n"; },
            [&renderer](void* c) { renderer.drawSliceIcon(c); },
            renderer,
            true, "Slice (L)", renderer::Key::L));

        // ── Scale  (has panel) ────────────────────────────────────────────────
        buttons_.emplace_back(Button(
            "scale",
            nullptr,
            [&renderer](void* c) { renderer.drawScaleIcon(c); },
            renderer,
            true, "Scale (S)", renderer::Key::S,
            /*hasPanel=*/true,
            [](const renderer::IRenderer& r,
               float px, float py, float /*pw*/, float /*ph*/) {
                auto& state = gui::states::ResizePanelState::instance();
                r.drawText("Scale (%)", px + 10.f, py + 10.f, 18);
                r.drawText(("X: " + std::to_string(state.scaleX)).c_str(), px + 10.f, py + 42.f, 16);
                r.drawText(("Y: " + std::to_string(state.scaleY)).c_str(), px + 10.f, py + 70.f, 16);
                r.drawText(("Z: " + std::to_string(state.scaleZ)).c_str(), px + 10.f, py + 98.f, 16);
            }));

        repositionButtons(renderer);
        _lastScreenWidth  = renderer.getScreenWidth();
        _lastScreenHeight = renderer.getScreenHeight();
    }

    void CenteredToolbar::update(renderer::IRenderer& renderer)
    {
        rebuildIfResized(renderer);

        const bool outsideClick = renderer.isMouseButtonPressed((int) renderer::MouseButton::Left) && [&]() {
            for (const auto& btn : buttons_)
            {
                const auto mouse = renderer.getMousePosition();
                if (mouse.x >= btn.x && mouse.x <= btn.x + btn.width &&
                    mouse.y >= btn.y && mouse.y <= btn.y + btn.height)
                    return false;
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
            btn.drawBase(renderer);
        for (const auto& btn : buttons_)
            btn.drawOverlay(renderer);
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