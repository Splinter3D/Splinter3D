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
                -Wcast-align -Wcast-qual
                -Wmissing-declarations -Wswitch-default
                -Wdouble-promotion -Wformat=2 -Wwrite-strings
        )
    endif ()
    message(STATUS "INFO: applied compiler warnings to target ${target}")
endfunction()

#######################################
