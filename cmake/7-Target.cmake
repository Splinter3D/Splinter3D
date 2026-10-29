########################################

set(SPLINTER3D_TARGET_NAME splinter3D)
if(TARGET ${SPLINTER3D_TARGET_NAME})
    set(SPLINTER3D_TARGET_NAME splinter3D-app)
    message(WARNING "Target 'splinter3D' already exists. Using '${SPLINTER3D_TARGET_NAME}' as the executable target name to avoid collision.")
endif()

########################################

if(WIN32)
    add_executable(${SPLINTER3D_TARGET_NAME} WIN32 ${SRC_SPLINTER3D})
else()
    add_executable(${SPLINTER3D_TARGET_NAME} ${SRC_SPLINTER3D})
endif()

target_include_directories(${SPLINTER3D_TARGET_NAME} PRIVATE ${INCLUDE_SPLINTER3D})

#######################################

add_custom_command(
    TARGET ${SPLINTER3D_TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${SPLINTER3D_TARGET_NAME}>/assets"
    COMMENT "Copying runtime assets"
)

########################################

if(DEFINED THIRD_PARTY_LIBS)
    target_link_libraries(${SPLINTER3D_TARGET_NAME} PRIVATE ${THIRD_PARTY_LIBS})
endif()

#######################################

apply_compiler_warnings(${SPLINTER3D_TARGET_NAME})
apply_linker_optimizations(${SPLINTER3D_TARGET_NAME})

#######################################
