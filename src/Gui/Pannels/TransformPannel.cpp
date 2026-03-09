#include <Gui/Pannels/TransformPannel.hpp>

namespace gui::pannels
{

    float TransformPannel::operator()(const renderer::IRenderer& r,
                                      float px, float py,
                                      float /*pw*/) const
    {
        auto& state = gui::states::TransformPannelState::getInstance();

        constexpr float kPad        = 10.0f;
        constexpr float kFieldH     = 24.0f;
        constexpr float kAxisBoxW   = 36.0f;
        constexpr float kAxisStepW  = 62.0f;
        constexpr float kMarkerSize = 8.0f;

        const renderer::Color kAxisX{229, 57, 53, 255};
        const renderer::Color kAxisY{67, 160, 71, 255};
        const renderer::Color kAxisZ{30, 136, 229, 255};

        r.drawText(px + kPad, py + kPad, "Transform", 16, renderer::Layer::Overlay);

        const float yRow = py + kPad + 24.0f;

        const float xRow = px + kPad;

        // X
        r.drawRectangle(xRow, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisX, renderer::Layer::Overlay);
        r.drawText(xRow + 12.0f, yRow + 4.0f, "X", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRow + 24.0f, yRow, kAxisBoxW, kFieldH,
                            "", state.posXi, -9999, 9999, state.editX, renderer::Layer::Overlay);
        if (state.posXi != state.posX)
        {
            state.posX = state.posXi;
            state.applyToTarget();
        }

        // Y
        const float xRowY = xRow + kAxisStepW;
        r.drawRectangle(xRowY, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisY, renderer::Layer::Overlay);
        r.drawText(xRowY + 12.0f, yRow + 4.0f, "Y", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRowY + 24.0f, yRow, kAxisBoxW, kFieldH,
                            "", state.posYi, -9999, 9999, state.editY, renderer::Layer::Overlay);
        if (state.posYi != state.posY)
        {
            state.posY = state.posYi;
            state.applyToTarget();
        }

        // Z
        const float xRowZ = xRow + kAxisStepW * 2.0f;
        r.drawRectangle(xRowZ, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisZ, renderer::Layer::Overlay);
        r.drawText(xRowZ + 12.0f, yRow + 4.0f, "Z", 14, renderer::Layer::Overlay);
        r.drawFloatValueBox(xRowZ + 24.0f, yRow, kAxisBoxW, kFieldH,
                            "", state.posZi, -9999, 9999, state.editZ, renderer::Layer::Overlay);
        if (state.posZi != state.posZ)
        {
            state.posZ = state.posZi;
            state.applyToTarget();
        }
        const float contentBottom = yRow + kFieldH;
        return (contentBottom - py) + kPad;
    }

} // namespace gui::pannels
