#include "objects3D/OMesh.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace objects3D
{
    OMesh OMesh::fromSTL(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open STL file: " + filename);
        }
    }
}