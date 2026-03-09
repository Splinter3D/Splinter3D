#pragma once

#include <Gui/States/ExportPannelState.hpp>
#include <Renderer/IRenderer.hpp>
#include <Renderer/Palette.hpp>
#include <algorithm>
#include <array>
#include <string>

namespace gui::panels
{
    struct ExportPannel
    {
        float operator()(const renderer::IRenderer& r,
                         float px, float py,
                         float pw) const
        {
            auto& state = gui::states::ExportPannelState::getInstance();

            constexpr float kPad            = 10.0f;
            constexpr float kDropdownHeight = 26.0f;
            constexpr float kButtonHeight   = 30.0f;

            const float mouseX = r.getMouseX();
            const float mouseY = r.getMouseY();
            const bool  click  = r.isMouseButtonPressed((int) renderer::MouseButton::Left);
            bool        clickConsumed{false};

            float y = py + kPad;
            r.drawText(px + kPad, y, "Export", 16, renderer::Layer::Overlay);
            y += 24.0f;

            drawTargetOption(r, px + kPad, y, "Export selected object",
                             state, gui::states::ExportPannelState::TargetMode::SelectedObject,
                             mouseX, mouseY, click, clickConsumed);
            y += 24.0f;

            drawTargetOption(r, px + kPad, y, "Export all objects as one",
                             state, gui::states::ExportPannelState::TargetMode::AllAsOne,
                             mouseX, mouseY, click, clickConsumed);
            y += 30.0f;

            r.drawText(px + kPad, y, "Format", 14, renderer::Layer::Overlay);
            y += 18.0f;

            const float dropdownWidth = pw - kPad * 2.0f;
            const float dropdownX     = px + kPad;
            const float dropdownY     = y;

            drawDropdown(r, dropdownX, dropdownY, dropdownWidth, kDropdownHeight,
                         mouseX, mouseY, click, clickConsumed, state);

            const float buttonY = dropdownY + kDropdownHeight + 32.0f;

            drawPrimaryButton(r, dropdownX, buttonY, dropdownWidth, kButtonHeight,
                              "Export", mouseX, mouseY, click, clickConsumed,
                              [&state]() { state.exportCurrentSelection(); });

            float contentBottom = buttonY + kButtonHeight;
            if (state.dropdownOpen)
            {
                const float optionsBottom = dropdownY + kDropdownHeight +
                                            kDropdownOptionHeight * (float) kFormatOptions.size();
                contentBottom             = std::max(contentBottom, optionsBottom);
            }

            contentBottom += kPad;
            return contentBottom - py;
        }

      private:
        struct FormatOption
        {
            gui::states::ExportPannelState::Format format;
            const char*                            label;
        };

        static constexpr float                       kDropdownOptionHeight = 24.0f;
        static constexpr std::array<FormatOption, 2> kFormatOptions{{
            {gui::states::ExportPannelState::Format::BinarySTL, "Binary STL (.stl)"},
            {gui::states::ExportPannelState::Format::AsciiSTL, "ASCII STL (.stl)"},
        }};

        static void drawTargetOption(const renderer::IRenderer&                 r,
                                     float                                      x,
                                     float                                      y,
                                     const char*                                label,
                                     gui::states::ExportPannelState&            state,
                                     gui::states::ExportPannelState::TargetMode mode,
                                     float                                      mouseX,
                                     float                                      mouseY,
                                     bool                                       click,
                                     bool&                                      clickConsumed)
        {
            constexpr float boxSize   = 16.0f;
            constexpr float padding   = 8.0f;
            const bool      isActive  = state.isTargetSelected(mode);
            const float     labelW    = r.measureTextWidth(label, 14);
            const float     rowWidth  = boxSize + padding + labelW;
            const float     rowHeight = boxSize;
            const bool      hovered   = pointInRect(x, y, rowWidth, rowHeight, mouseX, mouseY);

            if (hovered)
            {
                r.drawRectangle(x - 4.0f, y - 2.0f,
                                rowWidth + 8.0f, rowHeight + 4.0f,
                                renderer::Color(210, 215, 232, 120), renderer::Layer::Overlay);
            }

            r.drawRectangleLines(x, y, boxSize, boxSize, renderer::Palette::Primary, renderer::Layer::Overlay);
            if (isActive)
                r.drawRectangle(x + 3.0f, y + 3.0f, boxSize - 6.0f, boxSize - 6.0f,
                                renderer::Palette::Primary, renderer::Layer::Overlay);

            r.drawText(x + boxSize + padding, y - 2.0f, label, 14, renderer::Layer::Overlay);

            if (hovered && click && !clickConsumed)
            {
                state.selectTarget(mode);
                clickConsumed = true;
            }
        }

        static bool pointInRect(float x, float y, float w, float h, float px, float py)
        {
            return px >= x && px <= x + w && py >= y && py <= y + h;
        }

        static void drawDropdown(const renderer::IRenderer&      r,
                                 float                           x,
                                 float                           y,
                                 float                           w,
                                 float                           h,
                                 float                           mouseX,
                                 float                           mouseY,
                                 bool                            click,
                                 bool&                           clickConsumed,
                                 gui::states::ExportPannelState& state)
        {
            const bool            hovered   = pointInRect(x, y, w, h, mouseX, mouseY);
            const renderer::Color baseCol   = hovered ? renderer::Color(218, 222, 236, 255)
                                                      : renderer::Color(238, 240, 248, 255);
            const renderer::Color borderCol = renderer::Palette::Primary;

            r.drawRectangle(x, y, w, h, baseCol, renderer::Layer::Overlay);
            r.drawRectangleLines(x, y, w, h, borderCol, renderer::Layer::Overlay);

            const std::string label = state.currentFormatLabel();
            r.drawText(x + 8.0f, y + 6.0f, label.c_str(), 14, renderer::Layer::Overlay);
            r.drawText(x + w - 16.0f, y + 6.0f, state.dropdownOpen ? "^" : "v", 14, renderer::Layer::Overlay);

            if (hovered && click && !clickConsumed)
            {
                state.dropdownOpen = !state.dropdownOpen;
                clickConsumed      = true;
            }

            if (!state.dropdownOpen)
                return;

            const float optionsY      = y + h;
            const float optionHeight  = kDropdownOptionHeight;
            const float optionsHeight = optionHeight * (float) kFormatOptions.size();

            r.drawRectangle(x, optionsY, w, optionsHeight,
                            renderer::Color(246, 247, 253, 255), renderer::Layer::Debug);
            r.drawRectangleLines(x, optionsY, w, optionsHeight,
                                 renderer::Palette::Primary, renderer::Layer::Debug);

            const bool optionsHovered = pointInRect(x, optionsY, w, optionsHeight, mouseX, mouseY);

            for (size_t i = 0; i < kFormatOptions.size(); ++i)
            {
                const float rowY       = optionsY + optionHeight * (float) i;
                const bool  rowHovered = pointInRect(x, rowY, w, optionHeight, mouseX, mouseY);

                if (rowHovered)
                    r.drawRectangle(x, rowY, w, optionHeight,
                                    renderer::Color(224, 227, 242, 255), renderer::Layer::Debug);

                r.drawText(x + 8.0f, rowY + 5.0f, kFormatOptions[i].label, 14, renderer::Layer::Debug);

                if (rowHovered && click && !clickConsumed)
                {
                    state.setFormat(kFormatOptions[i].format);
                    state.dropdownOpen = false;
                    clickConsumed      = true;
                }
            }

            if (click && !clickConsumed && !optionsHovered && !hovered)
            {
                state.dropdownOpen = false;
                clickConsumed      = true;
            }
        }

        template <typename Fn>
        static void drawPrimaryButton(const renderer::IRenderer& r,
                                      float x, float y,
                                      float w, float h,
                                      const char* label,
                                      float mouseX, float mouseY,
                                      bool  click,
                                      bool& clickConsumed,
                                      Fn&&  onClick)
        {
            const bool            hovered     = pointInRect(x, y, w, h, mouseX, mouseY);
            const renderer::Color fillColor   = hovered ? renderer::Color(218, 221, 235, 255)
                                                        : renderer::Color(236, 238, 248, 255);
            const renderer::Color borderColor = renderer::Palette::Primary;

            r.drawRectangle(x, y, w, h, fillColor, renderer::Layer::Overlay);
            r.drawRectangleLines(x, y, w, h, borderColor, renderer::Layer::Overlay);

            const float textWidth = r.measureTextWidth(label, 14);
            r.drawText(x + (w - textWidth) * 0.5f, y + 6.0f, label, 14, renderer::Layer::Overlay);

            if (hovered && click && !clickConsumed)
            {
                clickConsumed = true;
                onClick();
            }
        }
    };
} // namespace gui::panels
