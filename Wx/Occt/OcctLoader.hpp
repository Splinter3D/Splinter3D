#pragma once

// ─────────────────────────────────────────────
//  wx::vtk::OcctLoader
//
//  Lit les fichiers CAD (STP/STEP/STL/IGES) et
//  retourne soit un TopoDS_Shape (pour OCCT/AIS)
//  soit un LoadResult avec vtkPolyData (si VTK actif).
//
//  Utilisé par OcctCanvas (option C, sans VTK).
// ─────────────────────────────────────────────

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <IGESControl_Reader.hxx>
#include <Poly_Triangulation.hxx>
#include <STEPControl_Reader.hxx>
#include <StlAPI_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace wx::vtk
{

    class OcctLoader
    {
      public:
        double linearDeflection  = 0.1;
        double angularDeflection = 0.5;

        // Retourne la shape OCCT brute — pour OcctCanvas (AIS)
        TopoDS_Shape loadShape(const std::string& path)
        {
            const std::string ext = toLower(
                std::filesystem::path(path).extension().string());

            if (ext == ".stp" || ext == ".step")
                return loadStep(path);
            if (ext == ".igs" || ext == ".iges")
                return loadIges(path);
            if (ext == ".stl")
                return loadStl(path);

            throw std::runtime_error(
                "OcctLoader: extension non supportée: " + ext);
        }

      private:
        TopoDS_Shape loadStep(const std::string& path)
        {
            STEPControl_Reader reader;
            if (reader.ReadFile(path.c_str()) != IFSelect_RetDone)
                throw std::runtime_error(
                    "OcctLoader: échec lecture STEP: " + path);
            reader.TransferRoots();
            return reader.OneShape();
        }

        TopoDS_Shape loadIges(const std::string& path)
        {
            IGESControl_Reader reader;
            if (reader.ReadFile(path.c_str()) != IFSelect_RetDone)
                throw std::runtime_error(
                    "OcctLoader: échec lecture IGES: " + path);
            reader.TransferRoots();
            return reader.OneShape();
        }

        TopoDS_Shape loadStl(const std::string& path)
        {
            TopoDS_Shape  shape;
            BRep_Builder  builder;
            StlAPI_Reader reader;
            if (!reader.Read(shape, path.c_str()))
                throw std::runtime_error(
                    "OcctLoader: échec lecture STL: " + path);
            return shape;
        }

        static std::string toLower(std::string s)
        {
            for (auto& c : s)
                c = static_cast<char>(std::tolower(c));
            return s;
        }
    };

} // namespace wx::vtk