########################################

set(SPLINTER3D_TARGET_NAME splinter3D)
if(TARGET ${SPLINTER3D_TARGET_NAME})
    set(SPLINTER3D_TARGET_NAME splinter3D-app)
    message(WARNING "Target 'splinter3D' already exists. Using '${SPLINTER3D_TARGET_NAME}' as the executable target name to avoid collision.")
endif()

########################################

add_executable(${SPLINTER3D_TARGET_NAME} ${SRC_SPLINTER3D})
target_include_directories(${SPLINTER3D_TARGET_NAME} PRIVATE ${INCLUDE_SPLINTER3D})

if(DEFINED THIRD_PARTY_LIBS)
    target_link_libraries(${SPLINTER3D_TARGET_NAME} PRIVATE ${THIRD_PARTY_LIBS})
endif()

#######################################

apply_compiler_warnings(${SPLINTER3D_TARGET_NAME})
apply_linker_optimizations(${SPLINTER3D_TARGET_NAME})

# Ensure translations are built before splinter executable
if(TARGET translations)
    add_dependencies(${SPLINTER3D_TARGET_NAME} translations)

    # Copy .mo files to source tree locale/ directory for dev
    add_custom_command(TARGET ${SPLINTER3D_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_BINARY_DIR}/locale"
            "${CMAKE_CURRENT_SOURCE_DIR}/locale"
    )
endif()

#######################################
