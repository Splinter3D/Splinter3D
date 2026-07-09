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

#######################################
