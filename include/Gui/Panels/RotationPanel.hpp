// RotationPanel.hpp
#pragma once
#include <Gui/States/RotationPanelState.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui::panels
{
    struct RotationPanel
    {
        /**
         * Draws the move panel UI using the provided renderer.
         */
        void operator()(const renderer::IRenderer& r,
                        float px, float py,
                        float /*pw*/, float /*ph*/) const
        {
            auto& state = gui::states::RotationPanelState::getInstance();

            constexpr float kPad        = 10.0f;
            constexpr float kFieldH     = 24.0f;
            constexpr float kAxisBoxW   = 36.0f;
            constexpr float kAxisStepW  = 62.0f;
            constexpr float kMarkerSize = 8.0f;

            const renderer::Color kAxisX{229, 57, 53, 255};
            const renderer::Color kAxisY{67, 160, 71, 255};
            const renderer::Color kAxisZ{30, 136, 229, 255};

            r.drawText(px + kPad, py + kPad, "Rotation", 16, renderer::Layer::Overlay);

            float fy = py + kPad + 24.0f;

            const float xRow = px + kPad;
            const float yRow = fy;

            // X
            r.drawRectangle(xRow, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisX, renderer::Layer::Overlay);
            r.drawText(xRow + 12.0f, yRow + 4.0f, "X", 14, renderer::Layer::Overlay);
            r.drawFloatValueBox(xRow + 24.0f, yRow, kAxisBoxW, kFieldH,
                                "", state.rotXInput, -360, 360, state.editX, renderer::Layer::Overlay);
            if (state.rotXInput != state.rotX)
            {
                state.rotX = state.rotXInput;
                state.applyToTarget();
            }

            // Y
            const float xRowY = xRow + kAxisStepW;
            r.drawRectangle(xRowY, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisY, renderer::Layer::Overlay);
            r.drawText(xRowY + 12.0f, yRow + 4.0f, "Y", 14, renderer::Layer::Overlay);
            r.drawFloatValueBox(xRowY + 24.0f, yRow, kAxisBoxW, kFieldH,
                                "", state.rotYInput, -360, 360, state.editY, renderer::Layer::Overlay);
            if (state.rotYInput != state.rotY)
            {
                state.rotY = state.rotYInput;
                state.applyToTarget();
            }

            // Z
            const float xRowZ = xRow + kAxisStepW * 2.0f;
            r.drawRectangle(xRowZ, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisZ, renderer::Layer::Overlay);
            r.drawText(xRowZ + 12.0f, yRow + 4.0f, "Z", 14, renderer::Layer::Overlay);
            r.drawFloatValueBox(xRowZ + 24.0f, yRow, kAxisBoxW, kFieldH,
                                "", state.rotZInput, -360, 360, state.editZ, renderer::Layer::Overlay);
            if (state.rotZInput != state.rotZ)
            {
                state.rotZ = state.rotZInput;
                state.applyToTarget();
            }
            fy += 34.0f;
        }
    };
} // namespace gui::panels
