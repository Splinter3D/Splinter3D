# Copies the runtime assets next to the executable. Run with -P from a POST_BUILD step:
#   cmake -DSRC=<assets dir> -DDST=<destination> -P CopyAssets.cmake
# Optional large STL samples (assets/stl/large, see scripts/fetch-assets.py) are skipped
# to keep the build directory light.

if(NOT SRC OR NOT DST)
    message(FATAL_ERROR "CopyAssets.cmake: SRC and DST are required")
endif()

file(COPY "${SRC}/" DESTINATION "${DST}"
     REGEX "/stl/large(/|$)" EXCLUDE)
