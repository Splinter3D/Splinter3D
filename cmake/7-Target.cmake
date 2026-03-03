########################################

set(SPLINTER_TARGET_NAME splinter)
if(TARGET ${SPLINTER_TARGET_NAME})
    set(SPLINTER_TARGET_NAME splinter-app)
    message(WARNING "Target 'splinter' already exists. Using '${SPLINTER_TARGET_NAME}' as the executable target name to avoid collision.")
endif()

########################################

add_executable(${SPLINTER_TARGET_NAME} ${SRC_SPLINTER})
target_include_directories(${SPLINTER_TARGET_NAME} PRIVATE ${INCLUDE_SPLINTER})

if(DEFINED THIRD_PARTY_LIBS)
    target_link_libraries(${SPLINTER_TARGET_NAME} PRIVATE ${THIRD_PARTY_LIBS})
endif()

#######################################

apply_compiler_warnings(${SPLINTER_TARGET_NAME})
apply_linker_optimizations(${SPLINTER_TARGET_NAME})

# Ensure translations are built before splinter executable
if(TARGET translations)
    add_dependencies(${SPLINTER_TARGET_NAME} translations)

    # Copy .mo files to source tree locale/ directory for dev
    add_custom_command(TARGET ${SPLINTER_TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_BINARY_DIR}/locale"
            "${CMAKE_CURRENT_SOURCE_DIR}/locale"
    )
endif()

#######################################
