#######################################
#######################################

file(GLOB_RECURSE SRC_SPLINTER3D CONFIGURE_DEPENDS "src/*.cpp")
if(NOT SRC_SPLINTER3D)
    message(FATAL_ERROR "No source files found under src/ - check path or globs.")
endif()

# main.cpp only holds wxIMPLEMENT_APP (the process entry point). Everything else goes
# into splinter3D_lib so test executables can link the same code without a second main().
set(SRC_SPLINTER3D_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cpp")
list(REMOVE_ITEM SRC_SPLINTER3D "${SRC_SPLINTER3D_MAIN}")

#######################################

set(INCLUDE_SPLINTER3D
    "${CMAKE_CURRENT_SOURCE_DIR}/src" ${OpenCASCADE_INCLUDE_DIR}
)

#######################################

message(STATUS "INFO: found ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp sources")

#######################################
