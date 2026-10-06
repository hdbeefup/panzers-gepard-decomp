# terrainview — draws a map terrain with pz::STerrain (agent B, M1).
# Windowed; reads the map from the paks of <rundir>. See terrainview.cpp.
add_executable(terrainview WIN32
    terrainview.cpp
)

set(TERRAINVIEW_LIBS pz3d core 3dengine window common sound platform)
if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
    list(APPEND TERRAINVIEW_LIBS mdec)
endif()

target_include_directories(terrainview PRIVATE ${PANZERS_SRC_DIR}/window ${PANZERS_SRC_DIR}/sound)
target_link_libraries(terrainview PRIVATE ${TERRAINVIEW_LIBS} stubs)

if(WIN32)
    target_link_libraries(terrainview PRIVATE
        winmm ws2_32 ole32 oleaut32 uuid shell32 advapi32 dbghelp
    )
endif()

if(DXSDK_DIR)
    target_link_libraries(terrainview PRIVATE d3d9 d3dx9 dinput8)
    if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
        target_link_libraries(terrainview PRIVATE dsound dxguid)
    endif()
endif()
