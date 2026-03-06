#######################################

find_package(glfw3 CONFIG REQUIRED)
find_package(raylib CONFIG REQUIRED)
find_package(Intl REQUIRED)
find_package(nfd CONFIG REQUIRED)

# fix: removes duplicate link libraries for raylib when using vcpkg
if(TARGET raylib)
    set_property(TARGET raylib PROPERTY INTERFACE_LINK_LIBRARIES "")
endif()

set(THIRD_PARTY_LIBS
    raylib
    glfw
    Intl::Intl
    nfd::nfd
)
#######################################
