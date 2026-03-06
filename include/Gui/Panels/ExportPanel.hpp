#pragma once

#include <Gui/States/ExportPanelState.hpp>
#include <Renderer/IRenderer.hpp>
#include <Renderer/Palette.hpp>
#include <array>
#include <string>

namespace gui::panels
{
    struct ExportPanel
    {
        void operator()(const renderer::IRenderer& r,
                        float px, float py,
                        float pw, float /*ph*/) const
        {
            auto& state = gui::states::ExportPanelState::getInstance();

            constexpr float kPad          = 10.0f;
            constexpr float kCheckboxSize = 16.0f;

            const float mouseX = r.getMouseX();
            const float mouseY = r.getMouseY();
            const bool  click  = r.isMouseButtonPressed((int) renderer::MouseButton::Left);
            bool        clickConsumed{false};

            float y = py + kPad;
            r.drawText(px + kPad, y, "Export", 16, renderer::Layer::Overlay);
            y += 24.0f;

            drawTargetCheckbox(r, px + kPad, y, kCheckboxSize,
                               "Export selected object",
                               state,
                               gui::states::ExportPanelState::TargetMode::SelectedObject);
            y += 24.0f;

            drawTargetCheckbox(r, px + kPad, y, kCheckboxSize,
                               "Export all objects as one",
                               state,
                               gui::states::ExportPanelState::TargetMode::AllAsOne);
            y += 30.0f;

            r.drawText(px + kPad, y, "Format", 14, renderer::Layer::Overlay);
            y += 18.0f;

            const float dropdownWidth  = pw - kPad * 2.0f;
            const float dropdownHeight = 26.0f;
            const float dropdownX      = px + kPad;
            const float dropdownY      = y;

            drawDropdown(r, dropdownX, dropdownY, dropdownWidth, dropdownHeight,
                         mouseX, mouseY, click, clickConsumed, state);

            const float buttonHeight = 30.0f;
            const float buttonY      = dropdownY + dropdownHeight + 32.0f;

            drawPrimaryButton(r, dropdownX, buttonY, dropdownWidth, buttonHeight,
                              "Export", mouseX, mouseY, click, clickConsumed,
                              [&state]() { state.exportCurrentSelection(); });
        }

      private:
        static void drawTargetCheckbox(const renderer::IRenderer& r,
                                       float x, float y, float size,
                                       const char* label,
                                       gui::states::ExportPanelState& state,
                                       gui::states::ExportPanelState::TargetMode mode)
        {
            bool& value = (mode == gui::states::ExportPanelState::TargetMode::SelectedObject)
                              ? state.exportSelectedChecked
                              : state.exportAllChecked;

            const bool previous = value;
            r.drawCheckbox(x, y, size, label, value, renderer::Layer::Overlay);
            if (value != previous)
                state.handleTargetToggle(mode, value);
        }

        static bool pointInRect(float x, float y, float w, float h, float px, float py)
        {
            return px >= x && px <= x + w && py >= y && py <= y + h;
        }

        static void drawDropdown(const renderer::IRenderer&     r,
                                 float                          x,
                                 float                          y,
                                 float                          w,
                                 float                          h,
                                 float                          mouseX,
                                 float                          mouseY,
                                 bool                           click,
                                 bool&                          clickConsumed,
                                 gui::states::ExportPanelState& state)
        {
            const bool hovered = pointInRect(x, y, w, h, mouseX, mouseY);
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

            struct FormatOption
            {
                gui::states::ExportPanelState::Format format;
                const char*                           label;
            };

            constexpr std::array<FormatOption, 2> options{{
                {gui::states::ExportPanelState::Format::BinarySTL, "Binary STL (.stl)"},
                {gui::states::ExportPanelState::Format::AsciiSTL, "ASCII STL (.stl)"},
            }};

            const float optionsY      = y + h;
            const float optionHeight  = 24.0f;
            const float optionsHeight = optionHeight * (float) options.size();

            r.drawRectangle(x, optionsY, w, optionsHeight,
                            renderer::Color(246, 247, 253, 255), renderer::Layer::Debug);
            r.drawRectangleLines(x, optionsY, w, optionsHeight,
                                 renderer::Palette::Primary, renderer::Layer::Debug);

            const bool optionsHovered = pointInRect(x, optionsY, w, optionsHeight, mouseX, mouseY);

            for (size_t i = 0; i < options.size(); ++i)
            {
                const float rowY       = optionsY + optionHeight * (float) i;
                const bool  rowHovered = pointInRect(x, rowY, w, optionHeight, mouseX, mouseY);

                if (rowHovered)
                    r.drawRectangle(x, rowY, w, optionHeight,
                                    renderer::Color(224, 227, 242, 255), renderer::Layer::Debug);

                r.drawText(x + 8.0f, rowY + 5.0f, options[i].label, 14, renderer::Layer::Debug);

                if (rowHovered && click && !clickConsumed)
                {
                    state.setFormat(options[i].format);
                    state.dropdownOpen = false;
                    clickConsumed      = true;
                }
            }

            if (click && !clickConsumed && !optionsHovered && !hovered)
                state.dropdownOpen = false;
        }

        template <typename Fn>
        static void drawPrimaryButton(const renderer::IRenderer& r,
                                      float x, float y,
                                      float w, float h,
                                      const char* label,
                                      float mouseX, float mouseY,
                                      bool click,
                                      bool& clickConsumed,
                                      Fn&& onClick)
        {
            const bool hovered = pointInRect(x, y, w, h, mouseX, mouseY);
            const renderer::Color fillColor = hovered ? renderer::Color(218, 221, 235, 255)
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
