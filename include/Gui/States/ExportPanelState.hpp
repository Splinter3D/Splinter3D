#pragma once

#include <Gui/Utils/FileDialog.hpp>
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gui::states
{
    class ExportPanelState : public splinter3D::utils::Singleton<ExportPanelState>
    {
      public:
        enum class TargetMode
        {
            SelectedObject,
            AllAsOne
        };

        enum class Format
        {
            BinarySTL,
            AsciiSTL
        };

        bool exportSelectedChecked{true};
        bool exportAllChecked{false};
        bool dropdownOpen{false};

        TargetMode target{TargetMode::SelectedObject};
        Format     format{Format::BinarySTL};

        void selectTarget(TargetMode mode)
        {
            target                = mode;
            exportSelectedChecked = (mode == TargetMode::SelectedObject);
            exportAllChecked      = !exportSelectedChecked;
        }

        void handleTargetToggle(TargetMode toggled, bool nowChecked)
        {
            if (!nowChecked)
            {
                // Keep a valid selection; revert checkbox to the active target
                selectTarget(target);
                return;
            }
            selectTarget(toggled);
        }

        void setFormat(Format fmt)
        {
            format = fmt;
        }

        std::string formatLabel(Format fmt) const
        {
            switch (fmt)
            {
            case Format::BinarySTL:
                return "Binary STL (.stl)";
            case Format::AsciiSTL:
                return "ASCII STL (.stl)";
            default:
                return "Unknown";
            }
        }

        std::string currentFormatLabel() const
        {
            return formatLabel(format);
        }

        bool exportCurrentSelection()
        {
            auto path = gui::utils::saveSTLFile();
            if (!path.has_value())
                return false;

            bool exported = false;

            if (target == TargetMode::SelectedObject)
            {
                std::unique_ptr<geometry::Mesh> mesh = scene::Scene::getInstance().getSelectedMesh(true);
                if (!mesh)
                {
                    std::cout << "[Toolbar] Export failed: no object selected\n";
                    return false;
                }
                exported = writeMesh(*mesh, *path);
            }
            else
            {
                std::vector<objects3D::Object3D> objects = scene::Scene::getInstance().getAllObjects3D();
                if (objects.empty())
                {
                    std::cout << "[Toolbar] Export failed: no objects in scene\n";
                    return false;
                }
                objects3D::Object3D combined(objects, true);
                geometry::Mesh*     mesh = combined.getMesh();
                if (!mesh)
                {
                    std::cout << "[Toolbar] Export failed: unable to combine meshes\n";
                    return false;
                }
                exported = writeMesh(*mesh, *path);
            }

            if (!exported)
                std::cout << "[Toolbar] Export failed while writing file\n";
            else
                std::cout << "[Toolbar] Exported " << currentFormatLabel() << " to " << *path << "\n";
            return exported;
        }

      private:
        bool writeMesh(geometry::Mesh& mesh, const std::string& path) const
        {
            switch (format)
            {
            case Format::BinarySTL:
                return mesh.toBinarySTL(path);
            case Format::AsciiSTL:
                return mesh.toAsciiSTL(path);
            default:
                return false;
            }
        }

        friend class splinter3D::utils::Singleton<ExportPanelState>;
    };
} // namespace gui::states
