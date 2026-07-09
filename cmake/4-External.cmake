#######################################

find_package(raylib CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)

set(GLFW_LINK_TARGET "")

find_package(glfw3 CONFIG QUIET)
if(TARGET glfw)
    set(GLFW_LINK_TARGET glfw)
elseif(TARGET glfw3)
    set(GLFW_LINK_TARGET glfw3)
elseif(TARGET glfw::glfw)
    set(GLFW_LINK_TARGET glfw::glfw)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GLFW3 QUIET IMPORTED_TARGET glfw3)
        if(TARGET PkgConfig::GLFW3)
            set(GLFW_LINK_TARGET PkgConfig::GLFW3)
        endif()
    endif()

    if(NOT GLFW_LINK_TARGET)
        find_library(GLFW_LIBRARY NAMES glfw glfw3 REQUIRED)
        set(GLFW_LINK_TARGET "${GLFW_LIBRARY}")
    endif()
endif()

# fix: removes duplicate link libraries for raylib when using vcpkg
if(WIN32 AND TARGET raylib)
    set_property(TARGET raylib PROPERTY INTERFACE_LINK_LIBRARIES "")
endif()

set(THIRD_PARTY_LIBS raylib ${GLFW_LINK_TARGET} nfd::nfd)

if(WIN32)
    # Add Windows-specific libraries for nativefiledialog-extended
    list(APPEND THIRD_PARTY_LIBS ole32 shell32 uuid)
endif()

#######################################
