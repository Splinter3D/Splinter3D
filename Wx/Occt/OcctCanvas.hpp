#pragma once

// ── Ordre critique : OCCT OpenGL AVANT wx ────────────────────────────────────
#include <OpenGl_GraphicDriver.hxx>

// wx — après OCCT OpenGL
#include <wx/glcanvas.h>
#include <wx/wx.h>

// GTK/GDK pour récupérer le Display X11
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

// OCCT — visualisation
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <Xw_Window.hxx>

// OCCT — loader
#include "OcctLoader.hpp"

namespace wx::occt
{

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
            Bind(wxEVT_MIDDLE_DOWN, &OcctCanvas::onMouseDown, this);
            Bind(wxEVT_MIDDLE_UP, &OcctCanvas::onMouseUp, this);
            Bind(wxEVT_MOTION, &OcctCanvas::onMouseMove, this);
            Bind(wxEVT_MOUSEWHEEL, &OcctCanvas::onMouseWheel, this);
            Bind(wxEVT_CHAR_HOOK, &OcctCanvas::onKey, this);
        }

        ~OcctCanvas()
        {
            delete m_glContext;
        }

        bool loadFile(const std::string& path)
        {
            try
            {
                wx::vtk::OcctLoader loader;
                m_shape = loader.loadShape(path);
                if (m_initialized)
                    displayShape();
                return true;
            }
            catch (const std::exception& ex)
            {
                wxMessageBox(ex.what(), _("Erreur chargement"), wxOK | wxICON_ERROR);
                return false;
            }
        }

        void fitAll()
        {
            if (m_view.IsNull())
                return;
            m_view->FitAll(0.01, false);
            m_view->ZFitAll();
            m_view->Redraw();
        }

        void viewFront()
        {
            setStdView(V3d_Yneg);
        }

        void viewBack()
        {
            setStdView(V3d_Ypos);
        }

        void viewRight()
        {
            setStdView(V3d_Xpos);
        }

        void viewLeft()
        {
            setStdView(V3d_Xneg);
        }

        void viewTop()
        {
            setStdView(V3d_Zpos);
        }

        void viewBottom()
        {
            setStdView(V3d_Zneg);
        }

      private:
        wxGLContext* m_glContext = nullptr;
        Handle(V3d_Viewer) m_viewer;
        Handle(V3d_View) m_view;
        Handle(AIS_InteractiveContext) m_context;
        Handle(AIS_Shape) m_aisShape;
        TopoDS_Shape m_shape;
        bool         m_initialized = false;
        wxPoint      m_lastMouse;
        bool         m_rotating = false;
        bool         m_panning  = false;

        static wxGLAttributes makeAttribs()
        {
            wxGLAttributes a;
            a.PlatformDefaults().RGBA().DoubleBuffer().Depth(24).EndList();
            return a;
        }

        void initOcct()
        {
            if (m_initialized)
                return;
            m_initialized = true;

            // Active le contexte GL wx — obligatoire avant tout appel OCCT OpenGL
            SetCurrent(*m_glContext);

            // Récupère le Display X11 via GDK (fiable sous GTK3)
            GdkDisplay* gdkDisp  = gdk_display_get_default();
            Display*    xDisplay = GDK_DISPLAY_XDISPLAY(gdkDisp);

            // Crée la connexion OCCT avec le Display X11 existant — pas de new Display
            Handle(Aspect_DisplayConnection) disp =
                new Aspect_DisplayConnection(xDisplay);

            Handle(OpenGl_GraphicDriver) driver =
                new OpenGl_GraphicDriver(disp, false);
            driver->ChangeOptions().buffersNoSwap = false;

            m_viewer = new V3d_Viewer(driver);
            m_viewer->SetDefaultLights();
            m_viewer->SetLightOn();
            m_viewer->SetDefaultBackgroundColor(Quantity_NOC_GRAY15);

            m_context = new AIS_InteractiveContext(m_viewer);
            m_context->SetDisplayMode(AIS_Shaded, false);

            // Récupère la X11 Window via GDK — évite le crash du GLXFBConfig nul
            GdkWindow* gdkWin = gtk_widget_get_window(
                static_cast<GtkWidget*>(GetHandle()));
            ::Window xwin = GDK_WINDOW_XID(gdkWin);

            Handle(Xw_Window) xwWin = new Xw_Window(disp, xwin);

            m_view = m_viewer->CreateView();
            m_view->SetWindow(xwWin);
            if (!xwWin->IsMapped())
                xwWin->Map();

            m_view->SetBackgroundColor(Quantity_NOC_GRAY15);
            m_view->MustBeResized();
            m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER,
                                    Quantity_NOC_GOLD, 0.08, V3d_ZBUFFER);

            if (!m_shape.IsNull())
                displayShape();
        }

        void displayShape()
        {
            if (!m_aisShape.IsNull())
                m_context->Remove(m_aisShape, false);

            m_aisShape = new AIS_Shape(m_shape);

            Graphic3d_MaterialAspect mat(Graphic3d_NOM_METALIZED);
            mat.SetColor(Quantity_Color(0.75, 0.75, 0.80, Quantity_TOC_RGB));
            m_aisShape->SetMaterial(mat);

            m_context->Display(m_aisShape, AIS_Shaded, 0, false);
            m_context->UpdateCurrentViewer();
            fitAll();
        }

        void setStdView(V3d_TypeOfOrientation orient)
        {
            if (m_view.IsNull())
                return;
            m_view->SetProj(orient);
            fitAll();
        }

        void onPaint(wxPaintEvent&)
        {
            wxPaintDC dc(this);
            if (!m_initialized)
                initOcct();
            if (!m_view.IsNull())
                m_view->Redraw();
        }

        void onSize(wxSizeEvent& e)
        {
            if (!m_view.IsNull())
                m_view->MustBeResized();
            e.Skip();
        }

        void onMouseDown(wxMouseEvent& e)
        {
            SetFocus();
            m_lastMouse = e.GetPosition();
            if (e.MiddleDown())
            {
                if (e.ShiftDown())
                    m_panning = true;
                else
                    m_rotating = true;
                if (!m_view.IsNull())
                    m_view->StartRotation(e.GetX(), e.GetY());
            }
            e.Skip();
        }

        void onMouseUp(wxMouseEvent& e)
        {
            m_rotating = false;
            m_panning  = false;
            e.Skip();
        }

        void onMouseMove(wxMouseEvent& e)
        {
            if (m_view.IsNull())
            {
                e.Skip();
                return;
            }
            const int dx = e.GetX() - m_lastMouse.x;
            const int dy = e.GetY() - m_lastMouse.y;
            m_lastMouse  = e.GetPosition();
            if (m_rotating)
            {
                m_view->Rotation(e.GetX(), e.GetY());
                m_view->Redraw();
            }
            else if (m_panning)
            {
                m_view->Pan(dx, -dy);
                m_view->Redraw();
            }
            e.Skip();
        }

        void onMouseWheel(wxMouseEvent& e)
        {
            if (m_view.IsNull())
            {
                e.Skip();
                return;
            }
            m_view->SetZoom(e.GetWheelRotation() > 0 ? 1.1 : 0.9);
            m_view->Redraw();
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
                case WXK_NUMPAD9:
                    viewBack();
                    break;
                default:
                    e.Skip();
                    return;
            }
        }
    };

} // namespace wx::occt