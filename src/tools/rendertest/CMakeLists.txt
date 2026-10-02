# rendertest — window + D3D9 device + SBoard smoke test (agent P2-B).
# Windowed; loads a loose TGA (and optionally a Panzers .font) and runs the
# SWindow::Run pump until Esc or close. See rendertest.cpp for usage.
add_executable(rendertest WIN32
    rendertest.cpp
)

set(RENDERTEST_LIBS core 3dengine window common sound platform)
if(SWINE_AUDIO_BACKEND STREQUAL "dsound")
    list(APPEND RENDERTEST_LIBS mdec)
endif()

target_include_directories(rendertest PRIVATE ${PANZERS_SRC_DIR}/window ${PANZERS_SRC_DIR}/sound)
target_link_libraries(rendertest PRIVATE ${RENDERTEST_LIBS} stubs)

if(WIN32)
    target_link_libraries(rendertest PRIVATE
        winmm ws2_32 ole32 oleaut32 uuid shell32 advapi32 dbghelp
    )
endif()

if(DXSDK_DIR)
    target_link_libraries(rendertest PRIVATE d3d9 d3dx9 dinput8)
    if(SWINE_AUDIO_BACKEND STREQUAL "dsound")
        target_link_libraries(rendertest PRIVATE dsound dxguid)
    endif()
endif()
