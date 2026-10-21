#######################################
#######################################

if(NOT ENABLE_TESTS)
    return()
endif()

enable_testing()
include(Catch)

# Exposes test-only APIs (e.g. EventBus::clear()) wrapped in #if defined(SPLINTER3D_TESTING).
# Set on the library, PUBLIC, so the library and every test see the same class definitions.
target_compile_definitions(${SPLINTER3D_LIB_TARGET_NAME} PUBLIC SPLINTER3D_TESTING=1)

#######################################

set(SPLINTER3D_TEST_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(SPLINTER3D_TEST_LOCALE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/locale")

# Builds every suite at once: `cmake --build build --target splinter3D_tests`
# (what scripts/build.sh --tests builds).
add_custom_target(splinter3D_tests)

# Helpers and Catch2 listeners compiled into every suite (see tests/support/).
file(GLOB_RECURSE SPLINTER3D_TEST_SUPPORT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/support/*.cpp")

# Builds one test executable from every .cpp under tests/<dir>/ and registers each Catch2
# test case with CTest under <label> (run a single suite with `ctest -L <label>`).
# Suites without sources yet are skipped so empty folders don't break the build.
function(splinter3d_add_test_suite target dir label)
    file(GLOB_RECURSE suite_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${dir}/*.cpp")
    if(NOT suite_sources)
        message(STATUS "INFO: no sources under tests/${dir}/, skipping ${target}")
        return()
    endif()

    add_executable(${target} ${suite_sources} ${SPLINTER3D_TEST_SUPPORT_SOURCES})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(${target} PRIVATE ${SPLINTER3D_LIB_TARGET_NAME} Catch2::Catch2WithMain)
    target_compile_definitions(${target} PRIVATE
        SPLINTER3D_TEST_ASSETS_DIR="${SPLINTER3D_TEST_ASSETS_DIR}"
        SPLINTER3D_TEST_LOCALE_DIR="${SPLINTER3D_TEST_LOCALE_DIR}")
    add_dependencies(splinter3D_tests ${target})

    apply_compiler_warnings(${target})

    # PRE_TEST defers test listing to `ctest` time, so the build never has to execute
    # the binary. (Listing never starts wx, so it works without a display; running
    # the integration suite on Linux does need one - use xvfb-run.)
    catch_discover_tests(${target}
        TEST_PREFIX "${label}."
        PROPERTIES LABELS ${label}
        DISCOVERY_MODE PRE_TEST)
    message(STATUS "INFO: added test suite ${target} (label: ${label})")
endfunction()

#######################################

splinter3d_add_test_suite(splinter3D_unit_tests        unit        unit)
splinter3d_add_test_suite(splinter3D_integration_tests integration integration)
splinter3d_add_test_suite(splinter3D_e2e_tests         end_to_end  e2e)

#######################################

find_package(Python3 COMPONENTS Interpreter QUIET)

# End-to-end tests (Python's built-in unittest, no extra dependency) run the real
# application binary, so building the test suites also builds the app.
add_dependencies(splinter3D_tests ${SPLINTER3D_TARGET_NAME})
if(Python3_Interpreter_FOUND)
    add_test(NAME e2e.python
             COMMAND ${Python3_EXECUTABLE} -m unittest discover -s tests/end_to_end -v
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(e2e.python PROPERTIES
        LABELS e2e
        # Raw coverage profiles of the (instrumented) app go to build/test-output/, not
        # to the source tree this test runs from.
        ENVIRONMENT "SPLINTER3D_BIN=$<TARGET_FILE:${SPLINTER3D_TARGET_NAME}>;LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/test-output/tests-%p.profraw")
endif()

# The locale files' key/placeholder consistency is checked by an existing script;
# register it with the integration suite so `ctest -L integration` covers it too.
if(Python3_Interpreter_FOUND)
    add_test(NAME integration.check-locales
             COMMAND ${Python3_EXECUTABLE} scripts/check-locales.py
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(integration.check-locales PROPERTIES LABELS integration)
endif()

#######################################
