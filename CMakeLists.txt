cmake_minimum_required(VERSION 3.16)
project(splinter3D-project LANGUAGES CXX)

########################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

########################################

include(1-Options)
include(2-Optimizations)
include(3-Warnings)
include(4-External)
include(5-Sources)
include(6-Linker)
include(7-Language)
include(8-Target)

########################################
