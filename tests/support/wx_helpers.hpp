#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/utils.h>
#include <wx/window.h>

namespace tests::support
{
    /**
     * @brief Processes pending wx events and idle work (including deferred
     * window destruction), since tests never run wx's main loop.
     */
    inline void pumpEvents()
    {
        wxTheApp->ProcessPendingEvents();
        wxYield();
        wxTheApp->ProcessIdle();
    }

    /**
     * @brief Pumps events until condition() holds or the timeout expires.
     *
     * @return The final value of condition().
     */
    inline bool pumpUntil(const std::function<bool()>& condition,
                          std::chrono::milliseconds    timeout = std::chrono::seconds(5))
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!condition() && std::chrono::steady_clock::now() < deadline)
        {
            pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return condition();
    }

    /**
     * @brief Returns the first descendant of root (depth first) of type T, or
     * nullptr.
     */
    template <typename T>
    T* findDescendant(wxWindow* root)
    {
        for (wxWindow* child : root->GetChildren())
        {
            if (auto* match = dynamic_cast<T*>(child))
                return match;
            if (auto* match = findDescendant<T>(child))
                return match;
        }
        return nullptr;
    }

    /**
     * @brief Owns a top-level frame for one test: destroys it (and lets wx
     * delete it) on scope exit.
     */
    template <typename FrameT = wxFrame>
    class ScopedFrame
    {
      public:
        template <typename... Args>
        explicit ScopedFrame(Args&&... args)
            : _frame(new FrameT(std::forward<Args>(args)...))
        { }

        ~ScopedFrame()
        {
            _frame->Destroy();
            pumpEvents();
        }

        ScopedFrame(const ScopedFrame&)            = delete;
        ScopedFrame& operator=(const ScopedFrame&) = delete;

        FrameT* operator->() const noexcept
        {
            return _frame;
        }

        FrameT* get() const noexcept
        {
            return _frame;
        }

        /// Shows the frame and waits until it is actually on screen (needed
        /// before creating an OpenGL context in it).
        void showAndWait()
        {
            _frame->Show();
            pumpUntil([this] { return _frame->IsShownOnScreen(); });
            pumpEvents();
        }

      private:
        FrameT* _frame;
    };

    /**
     * @brief Sends a menu command to a frame, as clicking the menu item would.
     */
    inline void sendMenuCommand(wxFrame* frame, int id)
    {
        wxCommandEvent event(wxEVT_MENU, id);
        event.SetEventObject(frame);
        frame->ProcessWindowEvent(event);
    }
} // namespace tests::support
