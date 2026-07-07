#######################################

find_package(glfw3 CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)

# fix: removes duplicate link libraries for raylib when using vcpkg
if(WIN32 AND TARGET raylib)
    set_property(TARGET raylib PROPERTY INTERFACE_LINK_LIBRARIES "")
endif()

set(THIRD_PARTY_LIBS raylib glfw nfd::nfd)

if(WIN32)
    # Add Windows-specific libraries for nativefiledialog-extended
    list(APPEND THIRD_PARTY_LIBS ole32 shell32 uuid)
endif()

#######################################
