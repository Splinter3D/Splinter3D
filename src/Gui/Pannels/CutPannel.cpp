#include <Gui/Pannels/CutPannel.hpp>
#include <Gui/States/CutPannelState.hpp>
#include <Renderer/Palette.hpp>

namespace gui::pannels
{
    float CutPannel::operator()(const renderer::IRenderer& r,
                                float px, float py,
                                float pw) const
    {
        auto& state = gui::states::CutPannelState::getInstance();

        constexpr float kPad        = 10.0f;
        constexpr float kFieldH     = 24.0f;
        constexpr float kAxisBoxW   = 36.0f;
        constexpr float kAxisStepW  = 62.0f;
        constexpr float kMarkerSize = 8.0f;
        constexpr float kBtnH       = 30.0f;

        const renderer::Color kAxisX{229, 57, 53, 255};
        const renderer::Color kAxisY{67, 160, 71, 255};
        const renderer::Color kAxisZ{30, 136, 229, 255};

        const float mouseX        = r.getMouseX();
        const float mouseY        = r.getMouseY();
        const bool  click         = r.isMouseButtonPressed((int) renderer::MouseButton::Left);
        bool        clickConsumed = false;

        float y = py + kPad;

        // ── Title ─────────────────────────────────────────────────
        r.drawText(px + kPad, y, "Cut", 16, renderer::Layer::Overlay);
        y += 24.0f;

        // ── Axis inputs ───────────────────────────────────────────
        const float xRow = px + kPad;

        // X
        r.drawRectangle(xRow, y + 8.0f, kMarkerSize, kMarkerSize, kAxisX, renderer::Layer::Overlay);
        r.drawText(xRow + 12.0f, y + 4.0f, "X", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRow + 24.0f, y, kAxisBoxW, kFieldH,
                            "", state.bedSizeXInput, -360, 360,
                            state.editX, renderer::Layer::Overlay);
        if (state.bedSizeXInput != state.bedSizeX)
            state.bedSizeX = state.bedSizeXInput;

        // Y
        const float xRowY = xRow + kAxisStepW;
        r.drawRectangle(xRowY, y + 8.0f, kMarkerSize, kMarkerSize, kAxisY, renderer::Layer::Overlay);
        r.drawText(xRowY + 12.0f, y + 4.0f, "Y", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRowY + 24.0f, y, kAxisBoxW, kFieldH,
                            "", state.bedSizeYInput, -360, 360,
                            state.editY, renderer::Layer::Overlay);
        if (state.bedSizeYInput != state.bedSizeY)
            state.bedSizeY = state.bedSizeYInput;

        // Z
        const float xRowZ = xRow + kAxisStepW * 2.0f;
        r.drawRectangle(xRowZ, y + 8.0f, kMarkerSize, kMarkerSize, kAxisZ, renderer::Layer::Overlay);
        r.drawText(xRowZ + 12.0f, y + 4.0f, "Z", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRowZ + 24.0f, y, kAxisBoxW, kFieldH,
                            "", state.bedSizeZInput, -360, 360,
                            state.editZ, renderer::Layer::Overlay);
        if (state.bedSizeZInput != state.bedSizeZ)
            state.bedSizeZ = state.bedSizeZInput;

        y += kFieldH + kPad;

        // ── Cut button ────────────────────────────────────────────
        const float btnW    = pw - kPad * 2.0f;
        const float btnX    = px + kPad;
        const bool  hovered = (mouseX >= btnX && mouseX <= btnX + btnW &&
                              mouseY >= y && mouseY <= y + kBtnH);

        const renderer::Color fillColor   = hovered ? renderer::Color(218, 221, 235, 255)
                                                    : renderer::Color(236, 238, 248, 255);
        const renderer::Color borderColor = renderer::Palette::Primary;

        r.drawRectangle(px + kPad, y, btnW, kBtnH, fillColor, renderer::Layer::Overlay);
        r.drawRectangleLines(px + kPad, y, btnW, kBtnH, borderColor, renderer::Layer::Overlay);

        const float textW = r.measureTextWidth("Cut", 14);
        r.drawText(btnX + (btnW - textW) * 0.5f, y + 8.0f, "Cut", 14, renderer::Layer::Overlay);

        if (hovered && click && !clickConsumed)
        {
            clickConsumed = true;
            state.executeCut();
        }

        y += kBtnH + kPad;

        return y - py; // return HEIGHT consumed, not absolute position
    }
} // namespace gui::pannels