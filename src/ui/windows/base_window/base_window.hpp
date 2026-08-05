#pragma once

#include <wx/wx.h>

namespace ui::windows
{
    /**
     * @brief Base class for every top-level application window.
     *
     * BaseWindow defines a fixed initialization sequence shared by all
     * application windows. Derived classes override only the initialization
     * hooks they need while the others remain no-ops.
     *
     * The initialization order is guaranteed to be:
     *
     * 1. InitMenuBar()
     * 2. InitToolBars()
     * 3. InitStatusBar()
     * 4. InitLayout()
     * 5. InitPanels()
     * 6. BindEvents()
     *
     * Because virtual dispatch is disabled while constructing the base class,
     * the constructor does not invoke these hooks automatically.
     * Each derived window must call InitializeAll() as the final step of its
     * own constructor.
     */
    class BaseWindow : public wxFrame
    {
      public:
        BaseWindow(const BaseWindow&)            = delete;
        BaseWindow& operator=(const BaseWindow&) = delete;

      protected:
        /**
         * @brief Constructs the base window.
         *
         * The constructor only initializes the underlying wxFrame.
         * Initialization hooks are intentionally not executed here.
         *
         * @param parent Parent window.
         * @param id Window identifier.
         * @param title Window title.
         * @param pos Initial window position.
         * @param size Initial window size.
         * @param style wxWidgets window style flags.
         */
        BaseWindow(wxWindow*       parent,
                   wxWindowID      id,
                   const wxString& title,
                   const wxPoint&  pos   = wxDefaultPosition,
                   const wxSize&   size  = wxDefaultSize,
                   long            style = wxDEFAULT_FRAME_STYLE);

        /**
         * @brief Creates the window menu bar.
         */
        virtual void InitMenuBar()
        { }

        /**
         * @brief Creates the window toolbars.
         */
        virtual void InitToolBars()
        { }

        /**
         * @brief Creates the status bar.
         */
        virtual void InitStatusBar()
        { }

        /**
         * @brief Creates the window layout.
         */
        virtual void InitLayout()
        { }

        /**
         * @brief Creates and inserts the window panels.
         */
        virtual void InitPanels()
        { }

        /**
         * @brief Binds every event handled by the window.
         */
        virtual void BindEvents()
        { }

        /**
         * @brief Executes the complete initialization sequence.
         *
         * This method must be called exactly once by every concrete window,
         * typically as the final line of its constructor.
         */
        void InitializeAll();
    };
} // namespace ui::windows
