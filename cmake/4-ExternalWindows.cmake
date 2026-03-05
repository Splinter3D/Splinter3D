#######################################

find_package(glfw3 CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)

# Third-party libs; `intl` handled below via an imported target when available
set(THIRD_PARTY_LIBS raylib glfw)

# Try to locate the vcpkg-provided intl import library and DLL (vcpkg_installed/*/{lib,bin})
file(GLOB VCPKG_INTL_LIBS "${CMAKE_SOURCE_DIR}/vcpkg_installed/*/lib/intl.lib")
if(VCPKG_INTL_LIBS)
    list(GET VCPKG_INTL_LIBS 0 INTL_LIB_PATH)
    get_filename_component(INTL_LIB_DIR ${INTL_LIB_PATH} DIRECTORY)
    # vcpkg_installed/<triplet>/lib -> bin next to it
    get_filename_component(VCPKG_INSTALLED_DIR ${INTL_LIB_DIR} DIRECTORY)
    file(GLOB VCPKG_INTL_DLLS "${VCPKG_INSTALLED_DIR}/bin/intl-*.dll")
    if(VCPKG_INTL_DLLS)
        list(GET VCPKG_INTL_DLLS 0 INTL_DLL_PATH)
    else()
        set(INTL_DLL_PATH "${INTL_LIB_PATH}")
    endif()

    # Create an imported SHARED library so MSVC links the IMPORTED_IMPLIB (the .lib)
    add_library(intl_lib SHARED IMPORTED)
    set_target_properties(intl_lib PROPERTIES
        IMPORTED_IMPLIB "${INTL_LIB_PATH}"
        IMPORTED_LOCATION "${INTL_DLL_PATH}"
        INTERFACE_INCLUDE_DIRECTORIES "${VCPKG_INSTALLED_DIR}/include"
    )

    list(APPEND THIRD_PARTY_LIBS intl_lib)
else()
    # Fallback to normal find_library if vcpkg layout isn't present
    find_library(INTL_LIB NAMES intl libintl)
    if(INTL_LIB)
        list(APPEND THIRD_PARTY_LIBS ${INTL_LIB})
    endif()
endif()

#######################################