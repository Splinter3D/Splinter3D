#include <Gui/CenteredToolbar.hpp>

namespace gui
{
    CenteredToolbar::CenteredToolbar(float topMargin, float buttonSize, float spacing)
        : topMargin_(topMargin)
        , buttonSize_(buttonSize)
        , spacing_(spacing)
    {
        initialize();
    }

    CenteredToolbar::~CenteredToolbar()
    {
        for (const auto& button : _toolbarButtons)
        {
            if (button.hasIcon)
                UnloadTexture(button.icon);
        }
    }

    void CenteredToolbar::initialize()
    {
        _toolbarButtons.reserve(5);
        _toolbarButtons.emplace_back(ToolbarButton(
            "import",
            []() { std::cout << "[Toolbar] Import bouton\n"; },
            [](Image& image) {
                const Color accent{0, 190, 255, 255};
                ImageDrawRectangle(&image, 30, 10, 4, 26, accent);
                ImageDrawTriangle(
                    &image,
                    Vector2{18.0f, 32.0f},
                    Vector2{46.0f, 32.0f},
                    Vector2{32.0f, 50.0f},
                    accent);
            }));
        _toolbarButtons.emplace_back(ToolbarButton(
            "export",
            []() { std::cout << "[Toolbar] Export bouton\n"; },
            [](Image& image) {
                const Color accent{67, 176, 65, 255};
                ImageDrawRectangle(&image, 30, 20, 4, 26, accent);
                ImageDrawTriangle(
                    &image,
                    Vector2{32.0f, 14.0f},
                    Vector2{18.0f, 32.0f},
                    Vector2{46.0f, 32.0f},
                    accent);
            }));
        _toolbarButtons.emplace_back(ToolbarButton(
            "slice",
            []() { std::cout << "[Toolbar] Slice bouton\n"; },
            [](Image& image) {
                const Color accent{243, 156, 18, 255};
                ImageDrawRectangle(&image, 16, 14, 32, 6, accent);
                ImageDrawRectangle(&image, 16, 26, 32, 6, accent);
                ImageDrawRectangle(&image, 16, 38, 32, 6, accent);
            }));
        _toolbarButtons.emplace_back(ToolbarButton(
            "preview",
            []() { std::cout << "[Toolbar] Preview bouton\n"; },
            [](Image& image) {
                const Color outline{236, 240, 241, 255};
                const Color iris{0, 151, 230, 255};
                ImageDrawCircleLines(&image, 32, 32, 18, outline);
                ImageDrawCircle(&image, 32, 32, 10, iris);
                ImageDrawCircle(&image, 32, 32, 4, Color{255, 255, 255, 255});
            }));
    }

    void CenteredToolbar::draw(const renderer::IRenderer& renderer) const
    {
        if (_toolbarButtons.empty())
            return;

        const size_t buttonCount = _toolbarButtons.size();
        float        totalWidth  = static_cast<float>(buttonCount) * buttonSize_;
        if (buttonCount > 1)
            totalWidth += static_cast<float>(buttonCount - 1) * spacing_;

        const float startX = (static_cast<float>(GetScreenWidth()) - totalWidth) * 0.5f;
        const float y      = topMargin_;

        for (size_t i = 0; i < buttonCount; ++i)
        {
            Rectangle bounds{
                startX + static_cast<float>(i) * (buttonSize_ + spacing_),
                y,
                buttonSize_,
                buttonSize_};

            const auto& button = _toolbarButtons[i];

            if (GuiButton(bounds, "") && button.onClick)
                button.onClick();

            if (button.hasIcon)
                renderer.drawIconCentered(bounds, button.icon, 0.55f);
        }
    }
} // namespace gui