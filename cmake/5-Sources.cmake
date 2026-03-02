#######################################

file(GLOB_RECURSE SRC_SPLINTER "src/*.cpp")
if(NOT SRC_SPLINTER)
    message(FATAL_ERROR "No source files found under src/ — check path or globs.")
endif()

#######################################

set(INCLUDE_SPLINTER
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#######################################

message(STATUS "INFO: found ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp sources")

#######################################
