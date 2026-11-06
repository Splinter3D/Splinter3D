#######################################

function(apply_compiler_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE
                /W4
                /WX
                /permissive-
                /EHsc
                /Zc:__cplusplus
                /wd4275
                /wd4251
                /wd4201
        )
        # Additional Windows-specific warning suppressions
        if(WIN32)
            target_compile_options(${target} PRIVATE
                    /wd4702
                    /wd4100
                    /wd4996
                    /wd4456
                    /wd4005
                    /wd4101
            )
        endif()
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        target_compile_options(${target} PRIVATE
                -Wall -Wextra -Werror -Wpedantic
                -Wconversion -Wsign-conversion
                -Wshadow -Wnull-dereference
                -Wundef -Wuninitialized
                -Wcast-align -Wcast-qual
                -Wswitch-default
                -Wdouble-promotion
                -Wformat=2
                -Wwrite-strings
        )
    else ()
        target_compile_options(${target} PRIVATE
                -Wall -Wextra -Werror -Wpedantic
                -Wconversion -Wsign-conversion
                -Wshadow -Wnull-dereference
                -Wundef -Wuninitialized -Winit-self
                -Wredundant-decls
                -Wno-unknown-pragmas
                -Wcast-align -Wcast-qual
                -Wmissing-declarations -Wswitch-default
                -Wdouble-promotion -Wformat=2 -Wwrite-strings
        )
    endif ()
    message(STATUS "INFO: applied compiler warnings to target ${target}")
endfunction()

#######################################

#######################################
#######################################

# Instruments a target for code coverage when ENABLE_COVERAGE is ON. Flags are PUBLIC so
# every executable linking an instrumented library also links the coverage runtime.
# Clang emits .profraw files (into build/test-output/, merged by llvm-profdata, see
# scripts/Build/Actions/Tests.py);
# GCC emits .gcda/.gcno files (read by gcovr).
function(apply_coverage target)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
        target_link_options(${target} PUBLIC -fprofile-instr-generate)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target} PUBLIC --coverage)
        target_link_options(${target} PUBLIC --coverage)
    else()
        message(WARNING "ENABLE_COVERAGE is not supported with ${CMAKE_CXX_COMPILER_ID}; ignoring for ${target}")
        return()
    endif()
    message(STATUS "INFO: applied coverage instrumentation to target ${target}")
endfunction()

#######################################
