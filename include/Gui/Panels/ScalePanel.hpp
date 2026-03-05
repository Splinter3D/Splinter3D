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
            r.drawValueBox(xRow + 24.0f, yRow, kAxisBoxW, kFieldH,
                           "", state.scaleXi, 1, 9999, state.editX, renderer::Layer::Overlay);
            if (state.scaleXi != (int) state.scaleX)
            {
                state.scaleX = (float) state.scaleXi;
                if (state.uniformScale)
                {
                    state.scaleY  = state.scaleX;
                    state.scaleZ  = state.scaleX;
                    state.scaleYi = state.scaleXi;
                    state.scaleZi = state.scaleXi;
                }
                state.applyToTarget();
            }

            // Y
            const float xRowY = xRow + kAxisStepW;
            r.drawRectangle(xRowY, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisY, renderer::Layer::Overlay);
            r.drawText(xRowY + 12.0f, yRow + 4.0f, "Y", 14, renderer::Layer::Overlay);
            r.drawValueBox(xRowY + 24.0f, yRow, kAxisBoxW, kFieldH,
                           "", state.scaleYi, 1, 9999, state.editY, renderer::Layer::Overlay);
            if (state.scaleYi != (int) state.scaleY)
            {
                state.scaleY = (float) state.scaleYi;
                if (state.uniformScale)
                {
                    state.scaleX  = state.scaleY;
                    state.scaleZ  = state.scaleY;
                    state.scaleXi = state.scaleYi;
                    state.scaleZi = state.scaleYi;
                }
                state.applyToTarget();
            }

            // Z
            const float xRowZ = xRow + kAxisStepW * 2.0f;
            r.drawRectangle(xRowZ, yRow + 8.0f, kMarkerSize, kMarkerSize, kAxisZ, renderer::Layer::Overlay);
            r.drawText(xRowZ + 12.0f, yRow + 4.0f, "Z", 14, renderer::Layer::Overlay);
            r.drawValueBox(xRowZ + 24.0f, yRow, kAxisBoxW, kFieldH,
                           "", state.scaleZi, 1, 9999, state.editZ, renderer::Layer::Overlay);
            if (state.scaleZi != (int) state.scaleZ)
            {
                state.scaleZ = (float) state.scaleZi;
                if (state.uniformScale)
                {
                    state.scaleX  = state.scaleZ;
                    state.scaleY  = state.scaleZ;
                    state.scaleXi = state.scaleZi;
                    state.scaleYi = state.scaleZi;
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