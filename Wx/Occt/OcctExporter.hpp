#pragma once

#include <BRepMesh_IncrementalMesh.hxx>
#include <StlAPI_Writer.hxx>
#include <TopoDS_Shape.hxx>
#include <stdexcept>
#include <string>

namespace wx::occt
{

    class OcctExporter
    {
      public:
        // ascii=true  → STL ASCII  (lisible, plus lourd)
        // ascii=false → STL binaire (compact, standard)
        static void exportStl(const TopoDS_Shape& shape,
                              const std::string&  path,
                              bool                ascii = false)
        {
            if (shape.IsNull())
                throw std::runtime_error("Shape vide — rien à exporter.");

            // Tessellation nécessaire avant export STL
            BRepMesh_IncrementalMesh mesh(shape, 0.1, false, 0.5);
            mesh.Perform();

            StlAPI_Writer writer;
            writer.ASCIIMode() = ascii;

            if (!writer.Write(shape, path.c_str()))
                throw std::runtime_error("Échec écriture STL : " + path);
        }
    };

} // namespace wx::occt