#include <Gui/CenteredToolbar.hpp>
#include <Gui/Pannels/CutPannel.hpp>
#include <Gui/Pannels/ExportPannel.hpp>
#include <Gui/Pannels/RotationPannel.hpp>
#include <Gui/Pannels/ScalePannel.hpp>
#include <Gui/Pannels/TransformPannel.hpp>
#include <Gui/States/CutPannelState.hpp>
#include <Gui/States/ExportPannelState.hpp>
#include <Gui/States/RotationPannelState.hpp>
#include <Gui/States/ScalePannelState.hpp>
#include <Gui/States/TransformPannelState.hpp>
#include <Gui/Utils/FileDialog.hpp>
#include <Input/Action.hpp>
#include <Input/InputManager.hpp>
#include <Renderer/IRenderer.hpp>
#include <Scene/Scene.hpp>
#include <algorithm>

namespace gui
{

    CenteredToolbar::CenteredToolbar(float topMargin, float buttonSize, float spacing)
        : topMargin_(topMargin), buttonSize_(buttonSize), spacing_(spacing)
    { }

    void CenteredToolbar::initialize(renderer::IRenderer& renderer)
    {
        buttons_.clear();
        buttons_.reserve(6);

        // Import
        buttons_.emplace_back(Button::Builder("import")
                                  .icon([&renderer](void* c) { renderer.drawImportIcon(c); })
                                  .action([]() { input::InputManager::getInstance().trigger(input::Action::Import); })
                                  .tooltip("Import (Ctrl+I)")
                                  .build(renderer));

        buttons_.emplace_back(Button::Builder("export")
                                  .icon([&renderer](void* c) { renderer.drawExportIcon(c); })
                                  .pannel(panels::ExportPannel())
                                  .action([]() { input::InputManager::getInstance().trigger(input::Action::OpenPannelExport); })
                                  .tooltip("Export (Ctrl+E)")
                                  .build(renderer));

        // Rotation (has pannel)
        buttons_.emplace_back(Button::Builder("rotation")
                                  .icon([&renderer](void* c) { renderer.drawRotationIcon(c); })
                                  .action([]() { input::InputManager::getInstance().trigger(input::Action::OpenPannelRotation); })
                                  .tooltip("Rotation (Ctrl+R)")
                                  .pannel(pannels::RotationPannel())
                                  .build(renderer));

        // Transform (has pannel)
        buttons_.emplace_back(Button::Builder("transform")
                                  .icon([&renderer](void* c) { renderer.drawTransformIcon(c); })
                                  .action([]() { input::InputManager::getInstance().trigger(input::Action::OpenPannelTransform); })
                                  .tooltip("Transform (Ctrl+T)")
                                  .pannel(pannels::TransformPannel())
                                  .build(renderer));

        // Scale (has pannel)
        buttons_.emplace_back(Button::Builder("scale")
                                  .icon([&renderer](void* c) { renderer.drawScaleIcon(c); })
                                  .action([]() { input::InputManager::getInstance().trigger(input::Action::OpenPannelScale); })
                                  .tooltip("Scale (Ctrl+S)")
                                  .pannel(pannels::ScalePannel())
                                  .build(renderer));

        // Cut (has pannel)
        buttons_.emplace_back(Button::Builder("cut")
                                  .icon([&renderer](void* c) { renderer.drawSliceIcon(c); })
                                  .action([]() { input::InputManager::getInstance().trigger(input::Action::OpenPannelCut); })
                                  .tooltip("Cut (Ctrl+X)")
                                  .pannel(pannels::CutPannel())
                                  .build(renderer));

        for (auto& btn : buttons_)
            btn.subscribeToPannelEvents();

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

                // Inside pannel bounds (if open)
                if (btn.isPannelOpen())
                {
                    float px = btn.x + (btn.width - Button::kPannelW) * 0.5f;
                    px       = std::max(4.0f, std::min(px, (float) renderer.getScreenWidth() - Button::kPannelW - 4.0f));
                    float py = btn.y + btn.height + Button::kPannelGap;

                    if (mouse.x >= px && mouse.x <= px + Button::kPannelW &&
                        mouse.y >= py && mouse.y <= py + btn.getPannelHeight())
                        return false;
                }
            }
            return true;
        }();
        for (size_t i = 0; i < buttons_.size(); ++i)
        {
            buttons_[i].update(renderer);

            if (buttons_[i].isPannelOpen())
            {
                for (size_t j = 0; j < buttons_.size(); ++j)
                    if (j != i)
                        buttons_[j].closePannel();
            }

            if (outsideClick)
                buttons_[i].closePannel();
        }
    }

    void CenteredToolbar::draw(const renderer::IRenderer& renderer) const
    {
        for (const auto& btn : buttons_)
            btn.draw(renderer);
    }

    bool CenteredToolbar::isMouseOver(const renderer::IRenderer& renderer) const
    {
        const auto mouse = renderer.getMousePosition();
        for (const auto& btn : buttons_)
        {
            if (mouse.x >= btn.x && mouse.x <= btn.x + btn.width &&
                mouse.y >= btn.y && mouse.y <= btn.y + btn.height)
                return true;

            if (!btn.isPannelOpen())
                continue;

            float px       = btn.x + (btn.width - Button::kPannelW) * 0.5f;
            px             = std::max(4.0f, std::min(px, (float) renderer.getScreenWidth() - Button::kPannelW - 4.0f));
            const float py = btn.y + btn.height + Button::kPannelGap;
            const float ph = btn.getPannelHeight();

            if (mouse.x >= px && mouse.x <= px + Button::kPannelW &&
                mouse.y >= py && mouse.y <= py + ph)
                return true;
        }
        return false;
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
