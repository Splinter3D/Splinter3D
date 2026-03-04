// ScalePanel.hpp
#pragma once
#include <Gui/States/ScalePanelState.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui::panels
{
    struct ScalePanel
    {
        void operator()(const renderer::IRenderer& r,
                        float px, float py,
                        float /*pw*/, float /*ph*/) const
        {
            auto& state = gui::states::ScalePanelState::instance();

            constexpr float kPad     = 10.0f;
            constexpr float kRowH    = 28.0f;
            constexpr float kLabelW  = 20.0f;
            constexpr float kFieldW  = 80.0f;
            constexpr float kFieldH  = 24.0f;
            constexpr float kSpacing = 6.0f;

            r.drawText(px + kPad, py + kPad, "Scale (%)", 16, renderer::Layer::Overlay);

            float fy = py + kPad + 24.0f;

            // ── X ─────────────────────────────────────────────────────
            r.drawText(px + kPad, fy + 4.0f, "X", 14, renderer::Layer::Overlay);
            r.drawValueBox(px + kPad + kLabelW + kSpacing, fy, kFieldW, kFieldH,
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
            fy += kRowH + kSpacing;

            // ── Y ─────────────────────────────────────────────────────
            r.drawText(px + kPad, fy + 4.0f, "Y", 14, renderer::Layer::Overlay);
            r.drawValueBox(px + kPad + kLabelW + kSpacing, fy, kFieldW, kFieldH,
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
            fy += kRowH + kSpacing;

            // ── Z ─────────────────────────────────────────────────────
            r.drawText(px + kPad, fy + 4.0f, "Z", 14, renderer::Layer::Overlay);
            r.drawValueBox(px + kPad + kLabelW + kSpacing, fy, kFieldW, kFieldH,
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
            fy += kRowH + kSpacing + 4.0f;

            // ── Uniform scale ──────────────────────────────────────────
            r.drawCheckbox(px + kPad, fy, 16.0f, "Uniform scale",
                           state.uniformScale, renderer::Layer::Overlay);
        }
    };
} // namespace gui::panels