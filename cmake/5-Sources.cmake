#######################################

file(GLOB_RECURSE SRC_SPLINTER3D "src/*.cpp")
if(NOT SRC_SPLINTER3D)
    message(FATAL_ERROR "No source files found under src/ — check path or globs.")
endif()

#######################################

set(INCLUDE_SPLINTER3D
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#######################################

message(STATUS "INFO: found ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp sources")

#######################################

# Compile .po -> .mo translations
find_program(MSGFMT_EXECUTABLE msgfmt)
if(NOT MSGFMT_EXECUTABLE)
    message(FATAL_ERROR "msgfmt not found. Please install gettext (msgfmt).")
endif()

file(GLOB_RECURSE PO_FILES "locale/*/LC_MESSAGES/*.po")
# Exclude English (default language) - not needed for translation
list(FILTER PO_FILES EXCLUDE REGEX "locale/en/")

list(LENGTH PO_FILES PO_COUNT)
message(STATUS "INFO: found ${PO_COUNT} translation files (*.po) in locale/")
if(PO_FILES)
    message(STATUS "      PO files: ${PO_FILES}")
    set(MO_FILES)
    foreach(PO ${PO_FILES})
        get_filename_component(PO_NAME_WE ${PO} NAME_WE)
        get_filename_component(LC_DIR ${PO} DIRECTORY)
        get_filename_component(LANG_DIR ${LC_DIR} DIRECTORY)
        get_filename_component(LANG_NAME ${LANG_DIR} NAME)
        set(MO_OUT "${CMAKE_CURRENT_BINARY_DIR}/locale/${LANG_NAME}/LC_MESSAGES/${PO_NAME_WE}.mo")

        add_custom_command(
            OUTPUT "${MO_OUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/locale/${LANG_NAME}/LC_MESSAGES"
            COMMAND ${MSGFMT_EXECUTABLE} -o "${MO_OUT}" "${PO}"
            DEPENDS "${PO}"
            COMMENT "Building translation ${LANG_NAME}/LC_MESSAGES/${PO_NAME_WE}.mo"
            VERBATIM
        )

        list(APPEND MO_FILES "${MO_OUT}")
    endforeach()

    if(MO_FILES)
        add_custom_target(translations ALL DEPENDS ${MO_FILES})

        # Install each .mo into share/locale/<lang>/LC_MESSAGES
        foreach(MO ${MO_FILES})
            get_filename_component(MO_DIR ${MO} DIRECTORY)
            file(RELATIVE_PATH REL_TO_LOCALE "${CMAKE_CURRENT_BINARY_DIR}/locale" "${MO_DIR}")
            string(REGEX MATCH "^[^/]+" MO_LANG "${REL_TO_LOCALE}")
            if(MO_LANG)
                install(FILES "${MO}" DESTINATION "share/locale/${MO_LANG}/LC_MESSAGES")
            endif()
        endforeach()
    endif()
endif()

#######################################
