cmake_minimum_required(VERSION 3.16)
project(splinter3D-project LANGUAGES CXX)

########################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

########################################

option(Windows "Use Windows-specific cmake fragments (3-WarningsWindows/4-ExternalWindows)" OFF)

include(1-Options)
include(2-Optimizations)
if(Windows)
    include(3-WarningsWindows)
    include(4-ExternalWindows)
else()
    include(3-Warnings)
    include(4-External)
endif()

include(5-Sources)
include(6-Linker)
include(7-Locale)
include(8-Target)

########################################
