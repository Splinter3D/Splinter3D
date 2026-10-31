#######################################

find_package(wxWidgets CONFIG REQUIRED COMPONENTS core base gl)

find_package(OpenCASCADE REQUIRED)
set(OCCT_LIBRARIES
    TKernel TKMath TKG3d TKBRep TKGeomBase TKTopAlgo    # geometry and topology
    TKService TKOpenGl TKV3d                            # display
    TKPrim TKBO TKFillet TKOffset TKHLR TKFeat TKMesh   # modelisation
    TKDESTEP TKDESTL)                                   # import/export

set(GLFW_LINK_TARGET "")

set(THIRD_PARTY_LIBS wx::core wx::base wx::gl ${OCCT_LIBRARIES})

if(WIN32)
    # Add Windows-specific libraries for nativefiledialog-extended
    list(APPEND THIRD_PARTY_LIBS ole32 shell32 uuid)
endif()

#######################################
