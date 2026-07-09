#pragma once

#include <BRep_Builder.hxx>
#include <IGESControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <StlAPI_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace wx::occt
{

    class OcctLoader
    {
      public:
        // Retourne la shape OCCT — supporte STL (bin+ascii) et STEP
        TopoDS_Shape load(const std::string& path)
        {
            const std::string ext = toLower(
                std::filesystem::path(path).extension().string());

            if (ext == ".stp" || ext == ".step")
                return loadStep(path);
            if (ext == ".stl")
                return loadStl(path);

            throw std::runtime_error("Format non supporté : " + ext);
        }

      private:
        TopoDS_Shape loadStep(const std::string& path)
        {
            STEPControl_Reader reader;
            if (reader.ReadFile(path.c_str()) != IFSelect_RetDone)
                throw std::runtime_error("Échec lecture STEP : " + path);
            reader.TransferRoots();
            return reader.OneShape();
        }

        TopoDS_Shape loadStl(const std::string& path)
        {
            TopoDS_Shape  shape;
            BRep_Builder  builder;
            StlAPI_Reader reader;
            // StlAPI_Reader gère automatiquement binaire et ASCII
            if (!reader.Read(shape, path.c_str()))
                throw std::runtime_error("Échec lecture STL : " + path);
            return shape;
        }

        static std::string toLower(std::string s)
        {
            for (auto& c : s)
                c = (char) std::tolower(c);
            return s;
        }
    };

} // namespace wx::occt