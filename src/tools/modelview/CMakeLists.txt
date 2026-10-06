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

# animview (agent M2-A): drives a unit animation class of src/game
# (SVehicleAnimation + SRunningGear, SWalkerAnimation, ...) with a synthetic
# movement at the 20 Hz logic step and renders it with tick interpolation.
# The animation sources are compiled in directly (no SWorld / SGameLogic:
# the tool binds its own g_UnitAnimEnv).
set(ANIMVIEW_GAME_DIR ${PANZERS_SRC_DIR}/game)
add_executable(animview WIN32
    animview.cpp
    animview_flying.cpp
    ${ANIMVIEW_GAME_DIR}/unitanim.cpp
    ${ANIMVIEW_GAME_DIR}/unitanim_vehicle.cpp
    ${ANIMVIEW_GAME_DIR}/unitanim_model.cpp
    ${ANIMVIEW_GAME_DIR}/unitanim_props.cpp
    ${ANIMVIEW_GAME_DIR}/m2trace.cpp
)
target_include_directories(animview PRIVATE ${PANZERS_SRC_DIR}/window ${PANZERS_SRC_DIR}/sound
                                            ${PANZERS_SRC_DIR}/3dengine ${PANZERS_SRC_DIR}/3dengine/pz
                                            ${ANIMVIEW_GAME_DIR} ${PANZERS_SRC_DIR}/stubs)
target_link_libraries(animview PRIVATE ${MODELVIEW_LIBS} stubs)
target_compile_options(animview PRIVATE /FI"${PANZERS_SRC_DIR}/3dengine/dxsdk_math_fix.h")
if(WIN32)
    target_link_libraries(animview PRIVATE winmm ws2_32 ole32 oleaut32 uuid shell32 advapi32 dbghelp)
endif()
if(DXSDK_DIR)
    target_link_libraries(animview PRIVATE d3d9 d3dx9 dinput8)
    if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
        target_link_libraries(animview PRIVATE dsound dxguid)
    endif()
endif()
