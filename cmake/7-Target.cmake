########################################

set(SPLINTER_TARGET_NAME splinter)
if(TARGET ${SPLINTER_TARGET_NAME})
    set(SPLINTER_TARGET_NAME splinter-app)
    message(WARNING "Target 'splinter' already exists. Using '${SPLINTER_TARGET_NAME}' as the executable target name to avoid collision.")
endif()

########################################

add_executable(${SPLINTER_TARGET_NAME} ${SRC_SPLINTER})
target_include_directories(${SPLINTER_TARGET_NAME} PRIVATE ${INCLUDE_SPLINTER})

#######################################

apply_compiler_warnings(${SPLINTER_TARGET_NAME})
apply_linker_optimizations(${SPLINTER_TARGET_NAME})

#######################################
