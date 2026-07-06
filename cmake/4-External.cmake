#######################################

find_package(glfw3 CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(Intl REQUIRED)

# fix: removes duplicate link libraries for raylib when using vcpkg
if(WIN32 AND TARGET raylib)
    set_property(TARGET raylib PROPERTY INTERFACE_LINK_LIBRARIES "")
endif()

if(WIN32)
    # Windows-specific external library handling
    set(THIRD_PARTY_LIBS raylib glfw Intl::Intl nfd::nfd)

    # Add Windows-specific libraries for nativefiledialog-extended
    list(APPEND THIRD_PARTY_LIBS ole32 shell32 uuid)
else()
    set(THIRD_PARTY_LIBS
        raylib
        glfw
        Intl::Intl
        nfd::nfd
    )
endif()

#######################################
