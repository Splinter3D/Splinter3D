#pragma once

// ── Ordre critique : OCCT OpenGL AVANT wx ────────────────────────────────────
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <functional>
#if defined(__WXGTK__)
#include <Xw_Window.hxx>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#elif defined(__WXMSW__)
#include <WNT_Window.hxx>
#endif
#include <map>
#include <string>
#include <wx/glcanvas.h>
#include <wx/wx.h>

namespace wx::occt
{

    // ── Objet affiché dans la scène ───────────────────────────────────────
    struct SceneObject
    {
        std::string name;
        Handle(AIS_Shape) aisShape;
        bool selected = false;
    };

    using SelectionCallback = std::function<void(const std::string& name)>;

    // ─────────────────────────────────────────────
    //  OcctCanvas
    //  wxGLCanvas + OCCT AIS
    //  Sélection : clic gauche sélectionne l'objet entier (highlight orange)
    //  Navigation : clic milieu orbite, Shift+milieu pan, scroll zoom
    // ─────────────────────────────────────────────
    class OcctCanvas : public wxGLCanvas
    {
      public:
        explicit OcctCanvas(wxWindow* parent)
            : wxGLCanvas(parent, makeAttribs(), wxID_ANY,
                         wxDefaultPosition, wxDefaultSize,
                         wxBORDER_NONE | wxWANTS_CHARS)
        {
            m_glContext = new wxGLContext(this);

            Bind(wxEVT_PAINT, &OcctCanvas::onPaint, this);
            Bind(wxEVT_SIZE, &OcctCanvas::onSize, this);
            Bind(wxEVT_LEFT_DOWN, &OcctCanvas::onLeftDown, this);
            Bind(wxEVT_MIDDLE_DOWN, &OcctCanvas::onMiddleDown, this);
            Bind(wxEVT_MIDDLE_UP, &OcctCanvas::onMiddleUp, this);
            Bind(wxEVT_MOTION, &OcctCanvas::onMotion, this);
            Bind(wxEVT_MOUSEWHEEL, &OcctCanvas::onWheel, this);
            Bind(wxEVT_CHAR_HOOK, &OcctCanvas::onKey, this);
        }

        ~OcctCanvas()
        {
            delete m_glContext;
        }

        // ── API publique ─────────────────────────────────────────────────

        // Ajoute une shape dans la scène avec un nom unique
        // Retourne false si le nom existe déjà
        bool addShape(const std::string& name, const TopoDS_Shape& shape)
        {
            if (m_objects.count(name))
                return false;

            SceneObject obj;
            obj.name     = name;
            obj.aisShape = new AIS_Shape(shape);

            // Force gris clair sur tous les objets (écrase les couleurs STEP/STL)
            Quantity_Color           grey(0.78, 0.78, 0.82, Quantity_TOC_RGB);
            Graphic3d_MaterialAspect mat(Graphic3d_NOM_METALIZED);
            mat.SetColor(grey);
            mat.SetAmbientColor(Quantity_Color(0.15, 0.15, 0.15, Quantity_TOC_RGB));
            mat.SetDiffuseColor(grey);
            mat.SetSpecularColor(Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB));
            mat.SetShininess(0.4f);
            obj.aisShape->SetMaterial(mat);
            obj.aisShape->SetColor(grey); // écrase la couleur embeddée dans le fichier

            if (m_initialized)
            {
                m_context->Display(obj.aisShape, AIS_Shaded, 0, false);
                m_context->UpdateCurrentViewer();
            }

            m_objects[name] = std::move(obj);
            return true;
        }

        // Supprime un objet par nom
        void removeShape(const std::string& name)
        {
            auto it = m_objects.find(name);
            if (it == m_objects.end())
                return;
            if (m_initialized && !it->second.aisShape.IsNull())
                m_context->Remove(it->second.aisShape, false);
            m_objects.erase(it);
            if (m_initialized)
                m_context->UpdateCurrentViewer();
        }

        void clearAll()
        {
            for (auto& [n, obj] : m_objects)
                if (m_initialized && !obj.aisShape.IsNull())
                    m_context->Remove(obj.aisShape, false);
            m_objects.clear();
            m_selectedName.clear();
            if (m_initialized)
                m_context->UpdateCurrentViewer();
            redraw();
        }

        // Retourne le nom de l'objet sélectionné ("" si aucun)
        const std::string& selectedName() const
        {
            return m_selectedName;
        }

        // Retourne la shape sélectionnée (nulle si aucune)
        TopoDS_Shape selectedShape() const
        {
            auto it = m_objects.find(m_selectedName);
            if (it == m_objects.end())
                return {};
            return it->second.aisShape->Shape();
        }

        void onSelectionChanged(SelectionCallback cb)
        {
            m_selectionCallback = std::move(cb);
        }

        void fitAll()
        {
            if (m_view.IsNull())
                return;
            m_view->FitAll(0.01, false);
            m_view->ZFitAll();
            redraw();
        }

        void viewFront()
        {
            setStdView(V3d_Yneg);
        }

        void viewRight()
        {
            setStdView(V3d_Xpos);
        }

        void viewTop()
        {
            setStdView(V3d_Zpos);
        }

      private:
        wxGLContext* m_glContext = nullptr;
        Handle(V3d_Viewer) m_viewer;
        Handle(V3d_View) m_view;
        Handle(AIS_InteractiveContext) m_context;
        bool m_initialized = false;

        std::map<std::string, SceneObject> m_objects;
        std::string                        m_selectedName;
        SelectionCallback                  m_selectionCallback;

        wxPoint m_lastMouse;
        bool    m_rotating = false;
        bool    m_panning  = false;

        static wxGLAttributes makeAttribs()
        {
            wxGLAttributes a;
            a.PlatformDefaults().RGBA().DoubleBuffer().Depth(24).EndList();
            return a;
        }

        // ── Init OCCT ─────────────────────────────────────
        void initOcct()
        {
            if (m_initialized)
                return;
            m_initialized = true;

            SetCurrent(*m_glContext);

#if defined(__WXGTK__)
            GdkDisplay* gdkDisp  = gdk_display_get_default();
            Display*    xDisplay = GDK_DISPLAY_XDISPLAY(gdkDisp);
            Handle(Aspect_DisplayConnection) disp =
                new Aspect_DisplayConnection(xDisplay);
#else
            // Windows (and other none-X11 platform) : no X11 display.
            Handle(Aspect_DisplayConnection) disp =
                new Aspect_DisplayConnection();
#endif

            Handle(OpenGl_GraphicDriver) driver =
                new OpenGl_GraphicDriver(disp, false);
            driver->ChangeOptions().buffersNoSwap = false;

            m_viewer = new V3d_Viewer(driver);
            m_viewer->SetDefaultLights();
            m_viewer->SetLightOn();
            m_viewer->SetDefaultBackgroundColor(Quantity_NOC_GRAY35);

            m_context = new AIS_InteractiveContext(m_viewer);
            m_context->SetDisplayMode(AIS_Shaded, false);

            // Selection = entire object (mode 0), orange highlight
            m_context->SetAutoActivateSelection(true);
            Quantity_Color selColor(1.0, 0.55, 0.0, Quantity_TOC_RGB);
            m_context->SelectionStyle()->SetColor(selColor);
            m_context->SelectionStyle()->SetDisplayMode(AIS_Shaded);

            // Hover = light blue
            Quantity_Color hvColor(0.3, 0.75, 1.0, Quantity_TOC_RGB);
            m_context->HighlightStyle()->SetColor(hvColor);

#if defined(__WXGTK__)
            GdkWindow* gdkWin = gtk_widget_get_window(
                static_cast<GtkWidget*>(GetHandle()));
            ::Window xwin = GDK_WINDOW_XID(gdkWin);

            Handle(Xw_Window) occtWin = new Xw_Window(disp, xwin);
#elif defined(__WXMSW__)
            // Windows : we get the HWND via wxWidgets.
            Handle(WNT_Window) occtWin =
                new WNT_Window((Aspect_Handle) GetHWND());
#else
#error "Plateform not supported by OcctCanvas : add the OCCT branch corresponding (ex: Cocoa_Window for macOS ?)."
#endif

            m_view = m_viewer->CreateView();
            m_view->SetWindow(occtWin);
            if (!occtWin->IsMapped())
                occtWin->Map();

            // Shade background : middle grey → dark grey (vertical)
            m_view->SetBgGradientColors(
                Quantity_Color(0.25, 0.25, 0.28, Quantity_TOC_RGB),
                Quantity_Color(0.12, 0.12, 0.14, Quantity_TOC_RGB),
                Aspect_GFM_VER, false);
            m_view->MustBeResized();
            m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER,
                                    Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);

            for (auto& [n, obj] : m_objects)
                m_context->Display(obj.aisShape, AIS_Shaded, 0, false);
            if (!m_objects.empty())
            {
                m_context->UpdateCurrentViewer();
                fitAll();
            }
        }

        void setStdView(V3d_TypeOfOrientation orient)
        {
            if (m_view.IsNull())
                return;
            m_view->SetProj(orient);
            fitAll();
        }

        void redraw()
        {
            if (!m_view.IsNull())
                m_view->Redraw();
        }

        // ── Sélection par objet entier ────────────────────────────────────
        void pickObject(int x, int y)
        {
            if (!m_initialized || m_context.IsNull())
                return;

            m_context->MoveTo(x, y, m_view, true);
            m_context->Select(true);

            // Désélectionne visuellement le précédent
            m_selectedName.clear();

            m_context->InitSelected();
            if (!m_context->MoreSelected())
            {
                if (m_selectionCallback)
                    m_selectionCallback("");
                return;
            }

            Handle(AIS_InteractiveObject) picked =
                m_context->SelectedInteractive();

            // Retrouve le nom dans notre map
            for (auto& [name, obj] : m_objects)
            {
                if (obj.aisShape == picked)
                {
                    m_selectedName = name;
                    break;
                }
            }

            if (m_selectionCallback)
                m_selectionCallback(m_selectedName);
        }

        // ── Events ────────────────────────────────────────────────────────

        void onPaint(wxPaintEvent&)
        {
            wxPaintDC dc(this);
            if (!m_initialized)
                initOcct();
            redraw();
        }

        void onSize(wxSizeEvent& e)
        {
            if (!m_view.IsNull())
                m_view->MustBeResized();
            e.Skip();
        }

        void onLeftDown(wxMouseEvent& e)
        {
            SetFocus();
            pickObject(e.GetX(), e.GetY());
            redraw();
            e.Skip();
        }

        void onMiddleDown(wxMouseEvent& e)
        {
            SetFocus();
            m_lastMouse = e.GetPosition();
            if (e.ShiftDown())
                m_panning = true;
            else
                m_rotating = true;
            if (!m_view.IsNull())
                m_view->StartRotation(e.GetX(), e.GetY());
            e.Skip();
        }

        void onMiddleUp(wxMouseEvent& e)
        {
            m_rotating = false;
            m_panning  = false;
            e.Skip();
        }

        void onMotion(wxMouseEvent& e)
        {
            if (m_view.IsNull())
            {
                e.Skip();
                return;
            }

            // Hover highlight
            if (!m_rotating && !m_panning && m_initialized)
            {
                m_context->MoveTo(e.GetX(), e.GetY(), m_view, true);
                redraw();
            }

            const int dx = e.GetX() - m_lastMouse.x;
            const int dy = e.GetY() - m_lastMouse.y;
            m_lastMouse  = e.GetPosition();

            if (m_rotating)
            {
                m_view->Rotation(e.GetX(), e.GetY());
                redraw();
            }
            else if (m_panning)
            {
                m_view->Pan(dx, -dy);
                redraw();
            }
            e.Skip();
        }

        void onWheel(wxMouseEvent& e)
        {
            if (m_view.IsNull())
            {
                e.Skip();
                return;
            }
            m_view->SetZoom(e.GetWheelRotation() > 0 ? 1.1 : 0.9);
            redraw();
        }

        void onKey(wxKeyEvent& e)
        {
            switch (e.GetKeyCode())
            {
                case WXK_NUMPAD0:
                case 'F':
                case 'f':
                    fitAll();
                    break;
                case WXK_NUMPAD1:
                    viewFront();
                    break;
                case WXK_NUMPAD3:
                    viewRight();
                    break;
                case WXK_NUMPAD7:
                    viewTop();
                    break;
                case WXK_ESCAPE:
                    m_context->ClearSelected(true);
                    m_selectedName.clear();
                    if (m_selectionCallback)
                        m_selectionCallback("");
                    redraw();
                    break;
                default:
                    e.Skip();
                    return;
            }
        }
    };

} // namespace wx::occt