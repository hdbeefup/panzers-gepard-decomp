# modelview (agent M1-A): loads one .4d through the HD 3D path (Gepard
# facade, SScene, SModel, SPModel, SPAnim) from the paks of panzers.ini,
# orbits a camera around it, plays a sequence and can save screenshots.
add_executable(modelview WIN32
    modelview.cpp
)

set(MODELVIEW_LIBS pz3d core 3dengine window common sound platform)
if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
    list(APPEND MODELVIEW_LIBS mdec)
endif()

target_include_directories(modelview PRIVATE ${PANZERS_SRC_DIR}/window ${PANZERS_SRC_DIR}/sound
                                             ${PANZERS_SRC_DIR}/3dengine/pz ${PANZERS_SRC_DIR}/world)
target_link_libraries(modelview PRIVATE ${MODELVIEW_LIBS} stubs)
target_compile_options(modelview PRIVATE /FI"${PANZERS_SRC_DIR}/3dengine/dxsdk_math_fix.h")

if(WIN32)
    target_link_libraries(modelview PRIVATE
        winmm ws2_32 ole32 oleaut32 uuid shell32 advapi32 dbghelp
    )
endif()

if(DXSDK_DIR)
    target_link_libraries(modelview PRIVATE d3d9 d3dx9 dinput8)
    if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
        target_link_libraries(modelview PRIVATE dsound dxguid)
    endif()
endif()
