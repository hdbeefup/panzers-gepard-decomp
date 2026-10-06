# fxview — plays the menu map's three .fx effects through pz::SPixie at the
# menu camera and saves screenshots (agent C). See fxview.cpp for usage.
add_executable(fxview WIN32
    fxview.cpp
)

set(FXVIEW_LIBS pz3d core 3dengine window common sound platform)
if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
    list(APPEND FXVIEW_LIBS mdec)
endif()

target_include_directories(fxview PRIVATE ${PANZERS_SRC_DIR}/window ${PANZERS_SRC_DIR}/sound)
target_link_libraries(fxview PRIVATE ${FXVIEW_LIBS} stubs)
target_compile_options(fxview PRIVATE /FI"${PANZERS_SRC_DIR}/3dengine/dxsdk_math_fix.h")

if(WIN32)
    target_link_libraries(fxview PRIVATE
        winmm ws2_32 ole32 oleaut32 uuid shell32 advapi32 dbghelp
    )
endif()

if(DXSDK_DIR)
    target_link_libraries(fxview PRIVATE d3d9 d3dx9 dinput8)
    if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
        target_link_libraries(fxview PRIVATE dsound dxguid)
    endif()
endif()
