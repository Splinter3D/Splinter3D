#include "Panels/ButtonsPanel.hpp"
#include "Panels/CheckRadioPanel.hpp"
#include "Panels/DialogsPanel.hpp"
#include "Panels/ListsPanel.hpp"
#include "Panels/SlidersPanel.hpp"
#include "Panels/TextPanel.hpp"
#include "Wx/Ids.hpp"
#include "Wx/Wx.hpp"

#include <wx/display.h>
#include <wx/notebook.h>

// Global callback — lets MainFrame trigger a language switch on MyApp
static std::function<void(const std::string&)> g_switchLanguage;

// IDs are centralized in Ids.hpp (namespace app)

// ─────────────────────────────────────────────
//  MainFrame
// ─────────────────────────────────────────────
class MainFrame : public wxFrame
{
  public:
    MainFrame() : wxFrame(nullptr, wxID_ANY, _("wxWidgets Demo"), wxDefaultPosition, wxDefaultSize)
    {
        BuildMenuBar();
        CreateStatusBar();
        SetStatusText(_("wxWidgets demo ready.   View -> Dark mode / Font   Language -> change lang"));

        m_notebook = new wxNotebook(this, wxID_ANY);
        RebuildTabs();

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_notebook, 1, wxEXPAND | wxALL, 8);
        SetSizer(sizer);

        SetSize(wxDisplay(wxDisplay::GetFromWindow(this)).GetClientArea());
        SetMinSize(wxSize(500, 400));
        Layout();
    }

    void RestoreMenuState(const wx::config::AppConfig& cfg)
    {
        auto* bar = GetMenuBar();
        if (!bar)
            return;

        bar->Check(IDs::BASIC::THEME_TOGGLE, cfg.dark);

        const int fid = cfg.fontSize == 9 ? IDs::FONT::SMALL : cfg.fontSize == 14 ? IDs::FONT::LARGE
                                                           : cfg.fontSize == 18   ? IDs::FONT::XLARGE
                                                                                  : IDs::FONT::NORMAL;
        bar->Check(fid, true);

        const int lid = cfg.lang == "en" ? IDs::LANG::EN : cfg.lang == "fr" ? IDs::LANG::FR
                                                       : cfg.lang == "es"   ? IDs::LANG::ES
                                                       : cfg.lang == "de"   ? IDs::LANG::DE
                                                                            : IDs::LANG::DEFAULT;
        bar->Check(lid, true);
    }

  private:
    wxNotebook* m_notebook = nullptr;

    void RebuildTabs()
    {
        m_notebook->DeleteAllPages();
        m_notebook->AddPage(new ButtonsPanel(m_notebook), _("Buttons"));
        m_notebook->AddPage(new TextPanel(m_notebook), _("Text inputs"));
        m_notebook->AddPage(new SlidersPanel(m_notebook), _("Sliders"));
        m_notebook->AddPage(new CheckRadioPanel(m_notebook), _("Check / Radio"));
        m_notebook->AddPage(new ListsPanel(m_notebook), _("Lists & Combo"));
        m_notebook->AddPage(new DialogsPanel(m_notebook), _("Dialogs"));
    }

    void BuildMenuBar()
    {
        auto* fileMenu = new wxMenu();
        fileMenu->Append(wxID_NEW, _("&New\tCtrl+N"));
        fileMenu->Append(wxID_OPEN, _("&Open\tCtrl+O"));
        fileMenu->AppendSeparator();
        fileMenu->Append(wxID_EXIT, _("E&xit\tAlt+F4"));

        auto* viewMenu = new wxMenu();
        viewMenu->AppendCheckItem(IDs::BASIC::FULLSCREEN, _("&Full screen\tF11"));
        viewMenu->AppendCheckItem(IDs::BASIC::THEME_TOGGLE, _("&Dark mode\tCtrl+D"));
        viewMenu->AppendSeparator();
        viewMenu->AppendRadioItem(IDs::FONT::SMALL, _("Font: Small   (9pt)"));
        viewMenu->AppendRadioItem(IDs::FONT::NORMAL, _("Font: Normal (11pt)"));
        viewMenu->AppendRadioItem(IDs::FONT::LARGE, _("Font: Large  (14pt)"));
        viewMenu->AppendRadioItem(IDs::FONT::XLARGE, _("Font: X-Large(18pt)"));
        viewMenu->Check(IDs::FONT::NORMAL, true);

        auto* langMenu = new wxMenu();
        langMenu->AppendRadioItem(IDs::LANG::DEFAULT, _("System default"));
        langMenu->AppendRadioItem(IDs::LANG::EN, "English");
        langMenu->AppendRadioItem(IDs::LANG::FR, "Francais");
        langMenu->AppendRadioItem(IDs::LANG::ES, "Espanol");
        langMenu->AppendRadioItem(IDs::LANG::DE, "Deutsch");
        langMenu->Check(IDs::LANG::DEFAULT, true);

        auto* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, _("&About"));

        auto* bar = new wxMenuBar();
        bar->Append(fileMenu, _("&File"));
        bar->Append(viewMenu, _("&View"));
        bar->Append(langMenu, _("&Language"));
        bar->Append(helpMenu, _("&Help"));
        SetMenuBar(bar);

        // Bind UI events via helper
        wx::events::BindEvents eventsBinder(this);
        eventsBinder.bindAll(g_switchLanguage);
    }
};

// ─────────────────────────────────────────────
//  MyApp
// ─────────────────────────────────────────────
class MyApp : public wxApp
{
    MainFrame* m_frame = nullptr;

  public:
    bool OnInit() override
    {
        auto  cfg   = wx::config::AppConfig::load();
        auto& theme = wx::theme::Theme::get();

        theme.setDark(cfg.dark);
        // Hook up language switch through utils
        g_switchLanguage = [this](const std::string& code) { SwitchLanguage(code); };

        wx::locale::Locale::init(cfg.lang, "splinter3D");

        m_frame = new MainFrame();
        if (cfg.dark)
            theme.apply(m_frame);
        if (cfg.fontSize != 11)
            theme.applyFontSize(m_frame, cfg.fontSize);
        m_frame->RestoreMenuState(cfg);
        m_frame->Show();
        return true;
    }

    void SwitchLanguage(const std::string& code)
    {
        auto cfg = wx::config::AppConfig::load();

        // Capture geometry and theme to apply after recreate
        const auto pos   = m_frame ? m_frame->GetPosition() : wxPoint(50, 50);
        const auto size  = m_frame ? m_frame->GetSize() : wxSize(800, 600);
        auto&      theme = wx::theme::Theme::get();

        // Delegate the heavy lifting to utils::switchLanguage. Provide factory and finalizer lambdas.
        wx::utils::switchLanguage(
            reinterpret_cast<wxFrame*&>(m_frame),
            code,
            // createFrame
            [=]() -> wxFrame* { return new MainFrame(); },
            // finalize
            [=](wxFrame* f) {
                MainFrame* mf = static_cast<MainFrame*>(f);
                mf->SetPosition(pos);
                mf->SetSize(size);
                if (cfg.dark)
                    theme.apply(mf);
                if (cfg.fontSize != 11)
                    theme.applyFontSize(mf, cfg.fontSize);
                mf->RestoreMenuState(cfg);
                mf->Show();
            },
            cfg);
    }
};

wxIMPLEMENT_APP(MyApp);