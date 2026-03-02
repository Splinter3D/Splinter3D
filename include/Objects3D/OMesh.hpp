#pragma once

#include <Objects3D/OTriangle.hpp>
#include <Objects3D/OVec3.hpp>
#include <string>
#include <vector>

namespace objects3D
{
    struct OMesh
    {
        std::vector<OTriangle> triangles;

        OMesh() = default;

        OMesh(const std::vector<OTriangle>& tris) : triangles(tris)
        { }

        /**
         * @brief Loads a 3D mesh from an STL file. The STL (Stereolithography) format is commonly used for 3D printing and computer-aided design. This method supports both ASCII and binary STL formats. It determines the format by reading the first few bytes of the file and then delegates to the appropriate parsing method.
         * @param filename The path to the STL file to be loaded.
         * @return An OMesh containing the triangles defined in the STL file.
         */
        static OMesh fromSTL(const std::string& filename);

      private:
        /**
         * @brief Reads an ASCII STL file and constructs an OMesh. The ASCII STL format consists of lines of text, where each triangle is defined by three "vertex" lines containing the vertex coordinates. The method reads the file line by line, extracts vertex data, and assembles triangles accordingly.
         * @param file An open ifstream positioned at the start of the ASCII STL data.
         * @return An OMesh containing the triangles from the STL file.
         */
        static OMesh fromAsciiSTL(std::ifstream& file);

        /**
         * @brief Reads a binary STL file and constructs an OMesh.
         * @param file An open ifstream positioned at the start of the binary STL data (after the header).
         * @return An OMesh containing the triangles from the STL file.
         */
        static OMesh fromBinarySTL(std::ifstream& file);
    };
} // namespace objects3D
