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

########################################

if(WIN32)
    add_executable(${SPLINTER3D_TARGET_NAME} WIN32 ${SRC_SPLINTER3D_MAIN})
else()
    add_executable(${SPLINTER3D_TARGET_NAME} ${SRC_SPLINTER3D_MAIN})
endif()

target_link_libraries(${SPLINTER3D_TARGET_NAME} PRIVATE ${SPLINTER3D_LIB_TARGET_NAME})

#######################################

add_custom_command(
    TARGET ${SPLINTER3D_TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${SPLINTER3D_TARGET_NAME}>/assets"
    COMMENT "Copying runtime assets"
)

#######################################

apply_compiler_warnings(${SPLINTER3D_TARGET_NAME})
apply_linker_optimizations(${SPLINTER3D_TARGET_NAME})

#######################################
