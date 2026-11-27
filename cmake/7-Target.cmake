########################################

set(SPLINTER3D_TARGET_NAME splinter3D)
if(TARGET ${SPLINTER3D_TARGET_NAME})
    set(SPLINTER3D_TARGET_NAME splinter3D-app)
    message(WARNING "Target 'splinter3D' already exists. Using '${SPLINTER3D_TARGET_NAME}' as the executable target name to avoid collision.")
endif()

set(SPLINTER3D_LIB_TARGET_NAME splinter3D_lib)

########################################

# All of src/ except main.cpp, shared by the application and the test executables.
# Static so tests need no extra runtime search path and Windows needs no DLL exports.
add_library(${SPLINTER3D_LIB_TARGET_NAME} STATIC ${SRC_SPLINTER3D})
target_include_directories(${SPLINTER3D_LIB_TARGET_NAME} PUBLIC ${INCLUDE_SPLINTER3D})
if(DEFINED THIRD_PARTY_LIBS)
    target_link_libraries(${SPLINTER3D_LIB_TARGET_NAME} PUBLIC ${THIRD_PARTY_LIBS})
endif()

apply_compiler_warnings(${SPLINTER3D_LIB_TARGET_NAME})
apply_coverage(${SPLINTER3D_LIB_TARGET_NAME})

# The application version: vcpkg.json's "version-semver", which the release workflow keeps in
# step with the tags (scripts/check-versions.sh). Read by core/utils/version.hpp.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json" SPLINTER3D_MANIFEST)
string(REGEX MATCH "\"version-semver\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]+)\"" _ "${SPLINTER3D_MANIFEST}")
if(NOT CMAKE_MATCH_1)
    message(FATAL_ERROR "vcpkg.json has no \"version-semver\"")
endif()
set(SPLINTER3D_VERSION "${CMAKE_MATCH_1}")
message(STATUS "INFO: Splinter3D version ${SPLINTER3D_VERSION}")
target_compile_definitions(${SPLINTER3D_LIB_TARGET_NAME} PUBLIC SPLINTER3D_VERSION="${SPLINTER3D_VERSION}")
if(ENABLE_RELEASE_BUILD)
    target_compile_definitions(${SPLINTER3D_LIB_TARGET_NAME} PUBLIC SPLINTER3D_RELEASE_BUILD=1)
    message(STATUS "INFO: release build")
else()
    message(STATUS "INFO: dev build (settings and profiles in <data root>/dev-data)")
endif()

########################################

if(WIN32)
    # splinter3D.rc embeds the application icon into the .exe.
    enable_language(RC)
    add_executable(${SPLINTER3D_TARGET_NAME} WIN32 ${SRC_SPLINTER3D_MAIN}
                   "${CMAKE_CURRENT_SOURCE_DIR}/src/app/splinter3D.rc")
else()
    add_executable(${SPLINTER3D_TARGET_NAME} ${SRC_SPLINTER3D_MAIN})
endif()

target_link_libraries(${SPLINTER3D_TARGET_NAME} PRIVATE ${SPLINTER3D_LIB_TARGET_NAME})

#######################################

add_custom_command(
    TARGET ${SPLINTER3D_TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -DSRC="${CMAKE_CURRENT_SOURCE_DIR}/assets"
            -DDST="$<TARGET_FILE_DIR:${SPLINTER3D_TARGET_NAME}>/assets"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/commands/CopyAssets.cmake"
    COMMENT "Copying runtime assets (optional large STLs are not copied)"
)

#######################################

apply_compiler_warnings(${SPLINTER3D_TARGET_NAME})
apply_linker_optimizations(${SPLINTER3D_TARGET_NAME})

#######################################
