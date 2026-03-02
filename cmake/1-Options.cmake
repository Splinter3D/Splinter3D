#######################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

#######################################

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

#######################################

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#######################################

message(STATUS "INFO: C++ standard set to ${CMAKE_CXX_STANDARD}")

#######################################

option(ENABLE_DEBUG "Enable SPLINTER3D_DEBUG macro" OFF)
if(ENABLE_DEBUG)
    add_compile_definitions(SPLINTER3D_DEBUG=1)
    message(STATUS "INFO: SPLINTER3D_DEBUG enabled")
endif()

#######################################

option(ENABLE_TESTS "Enable building tests" OFF)

#######################################
