# sound — Audio engine
#
# Two parallel backends, selected by SWINE_AUDIO_BACKEND in the root CMake:
#   dsound    → concert.cpp (Win32 DirectSound + in-tree mdec/ MP3 decoder).
#               Pixel-parity oracle on x86 Windows. Broken on x64 (MP3 stub).
#   miniaudio → concert_ma.cpp + miniaudio_impl.cpp. Cross-platform, decodes
#               MP3 + WAV (incl. ADPCM / float) via miniaudio's bundled dr_libs.
# Both implement the same SIConcert (sound/iconcert.h) factory CreateConcert().

if(SWINE_AUDIO_BACKEND STREQUAL "miniaudio")
    add_library(sound STATIC concert_ma.cpp miniaudio_impl.cpp)
    target_include_directories(sound PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(sound PRIVATE ${CMAKE_SOURCE_DIR}/third_party/miniaudio)
    target_link_libraries(sound PUBLIC core)
    if(WIN32)
        # miniaudio's WASAPI/DSound/WinMM backends pull in these system libs.
        target_link_libraries(sound PRIVATE ole32 winmm)
    elseif(UNIX AND NOT APPLE)
        # ALSA + Pulse + JACK auto-detected at runtime; the link side just needs
        # libdl + libpthread + libm. Distros without ALSA-dev still build (rt
        # detection); fail loud if dl/pthread are missing.
        target_link_libraries(sound PRIVATE dl pthread m)
    elseif(APPLE)
        find_library(COREAUDIO_LIB CoreAudio)
        find_library(COREFOUNDATION_LIB CoreFoundation)
        find_library(AUDIOTOOLBOX_LIB AudioToolbox)
        find_library(AUDIOUNIT_LIB AudioUnit)
        target_link_libraries(sound PRIVATE
            ${COREAUDIO_LIB} ${COREFOUNDATION_LIB}
            ${AUDIOTOOLBOX_LIB} ${AUDIOUNIT_LIB})
    endif()
else()
    add_library(sound STATIC concert.cpp)
    target_include_directories(sound PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(sound PRIVATE ${CMAKE_SOURCE_DIR}/mdec)
    target_link_libraries(sound PUBLIC core dsound dxguid)
endif()
