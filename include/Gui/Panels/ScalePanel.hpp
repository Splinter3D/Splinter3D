// ScalePanel.hpp
#pragma once
#include <Gui/States/ScalePanelState.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui::panels
{
    struct ScalePanel
    {
        /**
         * Draws the scale panel UI using the provided renderer.
         */
        void operator()(const renderer::IRenderer& r,
                        float px, float py,
                        float /*pw*/, float /*ph*/) const
        {
            auto& state = gui::states::ScalePanelState::getInstance();

            constexpr float kPad        = 10.0f;
            constexpr float kFieldH     = 24.0f;
            constexpr float kAxisBoxW   = 36.0f;
            constexpr float kAxisStepW  = 62.0f;
            constexpr float kMarkerSize = 8.0f;

            const renderer::Color kAxisX{229, 57, 53, 255};
            const renderer::Color kAxisY{67, 160, 71, 255};
            const renderer::Color kAxisZ{30, 136, 229, 255};

            r.drawText(px + kPad, py + kPad, "Scale", 16, renderer::Layer::Overlay);

            float fy = py + kPad + 24.0f;

            const float xRow = px + kPad;
            const float yRow = fy;

            // X
            r.drawRectangle(xRow, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisX, renderer::Layer::Overlay);
            r.drawText(xRow + 12.0f, yRow + 4.0f, "X", 14, renderer::Layer::Overlay);
            r.drawFloatValueBox(xRow + 24.0f, yRow, kAxisBoxW, kFieldH,
                                "", state.scaleXInput, 0, 9999, state.editX, renderer::Layer::Overlay);
            if (state.scaleXInput != state.scaleX)
            {
                state.scaleX = state.scaleXInput;
                if (state.uniformScale)
                {
                    state.scaleY      = state.scaleX;
                    state.scaleZ      = state.scaleX;
                    state.scaleYInput = state.scaleXInput;
                    state.scaleZInput = state.scaleXInput;
                }
                state.applyToTarget();
            }

            // Y
            const float xRowY = xRow + kAxisStepW;
            r.drawRectangle(xRowY, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisY, renderer::Layer::Overlay);
            r.drawText(xRowY + 12.0f, yRow + 4.0f, "Y", 14, renderer::Layer::Overlay);
            r.drawFloatValueBox(xRowY + 24.0f, yRow, kAxisBoxW, kFieldH,
                                "", state.scaleYInput, 0, 9999, state.editY, renderer::Layer::Overlay);
            if (state.scaleYInput != state.scaleY)
            {
                state.scaleY = state.scaleYInput;
                if (state.uniformScale)
                {
                    state.scaleX      = state.scaleY;
                    state.scaleZ      = state.scaleY;
                    state.scaleXInput = state.scaleYInput;
                    state.scaleZInput = state.scaleYInput;
                }
                state.applyToTarget();
            }

            // Z
            const float xRowZ = xRow + kAxisStepW * 2.0f;
            r.drawRectangle(xRowZ, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisZ, renderer::Layer::Overlay);
            r.drawText(xRowZ + 12.0f, yRow + 4.0f, "Z", 14, renderer::Layer::Overlay);
            r.drawFloatValueBox(xRowZ + 24.0f, yRow, kAxisBoxW, kFieldH,
                                "", state.scaleZInput, 0, 9999, state.editZ, renderer::Layer::Overlay);
            if (state.scaleZInput != state.scaleZ)
            {
                state.scaleZ = state.scaleZInput;
                if (state.uniformScale)
                {
                    state.scaleX      = state.scaleZ;
                    state.scaleY      = state.scaleZ;
                    state.scaleXInput = state.scaleZInput;
                    state.scaleYInput = state.scaleZInput;
                }
                state.applyToTarget();
            }
            fy += 34.0f;

            // ── Uniform scale ──────────────────────────────────────────
            r.drawCheckbox(px + kPad, fy, 16.0f, "Uniform scale",
                           state.uniformScale, renderer::Layer::Overlay);
        }
    };
} // namespace gui::panels
