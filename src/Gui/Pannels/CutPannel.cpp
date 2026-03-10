#include <Gui/Pannels/CutPannel.hpp>

namespace gui::pannels
{
    void CutPannel::operator()(const renderer::IRenderer& r,
                               float px, float py,
                               float /*pw*/, float /*ph*/) const
    {
        auto& state = gui::states::CutPannelState::getInstance();

        constexpr float kPad        = 10.0f;
        constexpr float kFieldH     = 24.0f;
        constexpr float kAxisBoxW   = 36.0f;
        constexpr float kAxisStepW  = 62.0f;
        constexpr float kMarkerSize = 8.0f;

        const renderer::Color kAxisX{229, 57, 53, 255};
        const renderer::Color kAxisY{67, 160, 71, 255};
        const renderer::Color kAxisZ{30, 136, 229, 255};

        r.drawText(px + kPad, py + kPad, "Cut", 16, renderer::Layer::Overlay);

        float fy = py + kPad + 24.0f;

        const float xRow = px + kPad;
        const float yRow = fy;

        // X
        r.drawRectangle(xRow, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisX, renderer::Layer::Overlay);
        r.drawText(xRow + 12.0f, yRow + 4.0f, "X", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRow + 24.0f, yRow, kAxisBoxW, kFieldH,
                            "", state.bedSizeXInput, -360, 360, state.editX, renderer::Layer::Overlay);
        if (state.bedSizeXInput != state.bedSizeX)
        {
            state.bedSizeX = state.bedSizeXInput;
        }

        // Y
        const float xRowY = xRow + kAxisStepW;
        r.drawRectangle(xRowY, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisY, renderer::Layer::Overlay);
        r.drawText(xRowY + 12.0f, yRow + 4.0f, "Y", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRowY + 24.0f, yRow, kAxisBoxW, kFieldH,
                            "", state.bedSizeYInput, -360, 360, state.editY, renderer::Layer::Overlay);
        if (state.bedSizeYInput != state.bedSizeY)
        {
            state.bedSizeY = state.bedSizeYInput;
        }

        // Z Input
        const float xRowZ = xRow + kAxisStepW * 2.0f;
        r.drawRectangle(xRowZ, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisZ, renderer::Layer::Overlay);
        r.drawText(xRowZ + 12.0f, yRow + 4.0f, "Z", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRowZ + 24.0f, yRow, kAxisBoxW, kFieldH,
                            "", state.bedSizeZInput, -360, 360, state.editZ, renderer::Layer::Overlay);
        if (state.bedSizeZInput != state.bedSizeZ)
        {
            state.bedSizeZ = state.bedSizeZInput;
        }
        fy += 34.0f;

        // ── Cut button ────────────────────────────────────────────────
        constexpr float kBtnW = 80.0f;
        constexpr float kBtnH = 24.0f;
        const float     btnX  = px + kPad;
        const float     btnY  = fy + kPad;

        r.drawButton(btnX, btnY, kBtnW, kBtnH, "Cut", nullptr, [&state]() { state.executeCut(); }, renderer::Layer::Overlay);

        fy += kBtnH + kPad;
    }
} // namespace gui::pannels
