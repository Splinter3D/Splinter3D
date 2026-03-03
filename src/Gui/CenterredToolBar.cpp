#include <Gui/CenteredToolbar.hpp>
#include <Renderer/IRenderer.hpp>
#include <iostream>

namespace gui
{
    CenteredToolbar::CenteredToolbar(float topMargin, float buttonSize, float spacing)
        : topMargin_(topMargin), buttonSize_(buttonSize), spacing_(spacing)
    {
    }

    CenteredToolbar::~CenteredToolbar()
    {
    }

    void CenteredToolbar::draw(const renderer::IRenderer& renderer) const
    {
        if (renderer.getScreenWidth() != _lastScreenWidth || renderer.getScreenHeight() != _lastScreenHeight)
        {
            std::cout << "[CenteredToolbar] Screen size changed, reinitializing toolbar...\n";
            const_cast<CenteredToolbar*>(this)->initialize(const_cast<renderer::IRenderer&>(renderer));
            const_cast<CenteredToolbar*>(this)->_lastScreenWidth  = renderer.getScreenWidth();
            const_cast<CenteredToolbar*>(this)->_lastScreenHeight = renderer.getScreenHeight();
        }
        for (const auto& button : _toolbarButtons)
            button.draw(renderer);
    }

    void CenteredToolbar::initialize(renderer::IRenderer& renderer)
    {
        _toolbarButtons.clear();
        _toolbarButtons.reserve(5);

        const size_t buttonCount = 5;
        float        totalWidth  = buttonCount * buttonSize_ + (buttonCount - 1) * spacing_;
        float        startX      = ((float) renderer.getScreenWidth() - totalWidth) * 0.5f;
        float        y           = topMargin_;

        _toolbarButtons.emplace_back(ToolbarButton(
            "import",
            []() { std::cout << "[Toolbar] Import bouton\n"; },
            [&renderer](void* canvas) { renderer.drawImportIcon(canvas); },
            renderer));

        _toolbarButtons.emplace_back(ToolbarButton(
            "export",
            []() { std::cout << "[Toolbar] Export bouton\n"; },
            [&renderer](void* canvas) { renderer.drawExportIcon(canvas); },
            renderer));

        _toolbarButtons.emplace_back(ToolbarButton(
            "preview",
            []() { std::cout << "[Toolbar] Preview bouton\n"; },
            [&renderer](void* canvas) { renderer.drawPreviewIcon(canvas); },
            renderer));

        _toolbarButtons.emplace_back(ToolbarButton(
            "slice",
            []() { std::cout << "[Toolbar] Slice bouton\n"; },
            [&renderer](void* canvas) { renderer.drawSliceIcon(canvas); },
            renderer));

        // Assign positions & sizes directly in the button
        for (size_t i = 0; i < _toolbarButtons.size(); ++i)
        {
            _toolbarButtons[i].x      = startX + (float) i * (buttonSize_ + spacing_);
            _toolbarButtons[i].y      = y;
            _toolbarButtons[i].width  = buttonSize_;
            _toolbarButtons[i].height = buttonSize_;
        }
    }
} // namespace gui