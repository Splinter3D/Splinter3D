#pragma once

namespace gui::states
{
    class ResizePanelState
    {
      public:
        static ResizePanelState& instance()
        {
            static ResizePanelState state;
            return state;
        }

        float scaleX{100.0f};
        float scaleY{100.0f};
        float scaleZ{100.0f};

      private:
        ResizePanelState() = default;
    };
} // namespace gui