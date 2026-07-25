#pragma once

// ─────────────────────────────────────────────
//  SelectionManager
//
//  Gère la sélection de faces et d'arêtes via
//  OCCT AIS_InteractiveContext.
//
//  Usage:
//    SelectionManager sel(m_context, m_view);
//    sel.onMouseMove(x, y);          // hover highlight
//    sel.onMouseClick(x, y);         // sélection clic
//    sel.clearSelection();
//    auto info = sel.getSelectionInfo(); // texte descriptif
// ─────────────────────────────────────────────

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <V3d_View.hxx>
#include <sstream>
#include <string>

namespace wx::occt
{

    // Ce que l'utilisateur a sélectionné
    struct SelectionInfo
    {
        bool        hasSelection = false;
        std::string type;    // "Face", "Edge", "Vertex", ...
        std::string detail;  // type géométrique OCCT ("Plane", "Cylinder", "Line"...)
        std::string summary; // texte complet pour la status bar
    };

    class SelectionManager
    {
      public:
        SelectionManager(Handle(AIS_InteractiveContext) ctx,
                         Handle(V3d_View) view)
            : m_context(ctx), m_view(view)
        { }

        // ── Active les modes de sélection sur une shape ───────────────────
        void activateOn(Handle(AIS_Shape) shape)
        {
            if (shape.IsNull())
                return;

            // Mode 0 = shape entière, 2 = faces, 1 = arêtes, 3 = vertices
            m_context->Activate(shape, 2); // faces
            m_context->Activate(shape, 1); // arêtes
        }

        // ── Hover — appeler sur wxEVT_MOTION ─────────────────────────────
        void onMouseMove(int x, int y)
        {
            if (m_context.IsNull() || m_view.IsNull())
                return;
            m_context->MoveTo(x, y, m_view, true);
        }

        // ── Clic — appeler sur wxEVT_LEFT_DOWN ───────────────────────────
        SelectionInfo onMouseClick(int x, int y, bool addToSelection = false)
        {
            if (m_context.IsNull() || m_view.IsNull())
                return {};

            if (addToSelection)
                m_context->ShiftSelect(true);
            else
                m_context->Select(true);

            return getSelectionInfo();
        }

        void clearSelection()
        {
            if (!m_context.IsNull())
                m_context->ClearSelected(true);
        }

        // ── Lit la sélection courante et retourne une description ─────────
        SelectionInfo getSelectionInfo() const
        {
            SelectionInfo info;
            if (m_context.IsNull())
                return info;

            m_context->InitSelected();
            if (!m_context->MoreSelected())
                return info;

            info.hasSelection = true;

            Handle(SelectMgr_EntityOwner) owner = m_context->SelectedOwner();
            Handle(StdSelect_BRepOwner) brepOwner =
                Handle(StdSelect_BRepOwner)::DownCast(owner);

            if (brepOwner.IsNull())
                return info;

            const TopoDS_Shape& shape = brepOwner->Shape();
            switch (shape.ShapeType())
            {
                case TopAbs_FACE:
                    info.type   = "Face";
                    info.detail = describeFace(TopoDS::Face(shape));
                    break;
                case TopAbs_EDGE:
                    info.type   = "Edge";
                    info.detail = describeEdge(TopoDS::Edge(shape));
                    break;
                case TopAbs_VERTEX:
                    info.type   = "Vertex";
                    info.detail = "";
                    break;
                default:
                    info.type = "Shape";
                    break;
            }

            info.summary = info.type;
            if (!info.detail.empty())
                info.summary += " — " + info.detail;

            return info;
        }

      private:
        Handle(AIS_InteractiveContext) m_context;
        Handle(V3d_View) m_view;

        static std::string describeFace(const TopoDS_Face& face)
        {
            try
            {
                BRepAdaptor_Surface surf(face);
                switch (surf.GetType())
                {
                    case GeomAbs_Plane:
                        return "Plane";
                    case GeomAbs_Cylinder:
                        return "Cylinder";
                    case GeomAbs_Cone:
                        return "Cone";
                    case GeomAbs_Sphere:
                        return "Sphere";
                    case GeomAbs_Torus:
                        return "Torus";
                    case GeomAbs_BezierSurface:
                        return "Bezier";
                    case GeomAbs_BSplineSurface:
                        return "BSpline";
                    default:
                        return "Surface";
                }
            }
            catch (...)
            {
                return "Surface";
            }
        }

        static std::string describeEdge(const TopoDS_Edge& edge)
        {
            try
            {
                BRepAdaptor_Curve curve(edge);
                switch (curve.GetType())
                {
                    case GeomAbs_Line:
                        return "Line";
                    case GeomAbs_Circle:
                        return "Circle";
                    case GeomAbs_Ellipse:
                        return "Ellipse";
                    case GeomAbs_BezierCurve:
                        return "Bezier";
                    case GeomAbs_BSplineCurve:
                        return "BSpline";
                    default:
                        return "Curve";
                }
            }
            catch (...)
            {
                return "Curve";
            }
        }
    };

} // namespace wx::occt