# main — the Panzers executable.
#
# The engine libraries are linked with /WHOLEARCHIVE so EVERY imported object
# is pulled into the link even though the placeholder WinMain references none
# of them. Without it a static-lib link would silently skip unreferenced
# objects and hide unresolved externals; with it, each missing world/game
# symbol must be satisfied by an explicit entry in src/stubs/.
add_executable(panzers WIN32
    panzers_main.cpp
    crashhandler.cpp
)

set(PANZERS_ENGINE_LIBS core 3dengine pz3d pzworld pzgame window common sound platform panzersgame)
if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
    list(APPEND PANZERS_ENGINE_LIBS mdec)
    target_compile_definitions(panzers PRIVATE GEPARD_AUDIO_DSOUND=1)
endif()

target_link_libraries(panzers PRIVATE ${PANZERS_ENGINE_LIBS} stubs)
if(MSVC)
    foreach(_lib IN LISTS PANZERS_ENGINE_LIBS)
        target_link_options(panzers PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${_lib}>")
    endforeach()
    target_link_options(panzers PRIVATE /MAP /DYNAMICBASE:NO)
endif()

if(WIN32)
    target_link_libraries(panzers PRIVATE
        winmm ws2_32 ole32 oleaut32 uuid shell32 advapi32 dbghelp
    )
endif()

if(DXSDK_DIR)
    target_link_libraries(panzers PRIVATE d3d9 d3dx9 dinput8)
    if(GEPARD_AUDIO_BACKEND STREQUAL "dsound")
        target_link_libraries(panzers PRIVATE dsound dxguid)
    endif()
endif()

# Tripwire census: printed after every link of the exe.
if(Python3_Interpreter_FOUND)
    add_custom_command(TARGET panzers POST_BUILD
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/census.py ${PANZERS_SRC_DIR}
        VERBATIM)
endif()
