// sound/miniaudio_impl.cpp
// Translation unit holding the miniaudio implementation. Isolated here so the
// ~95k-line single-header library is compiled exactly once and the heavy
// compile cost doesn't bleed into other TUs.
//
// Selected by SWINE_AUDIO_BACKEND=miniaudio (see sound/CMakeLists.txt).

#define MINIAUDIO_IMPLEMENTATION

// Cut features we don't need to keep compile time and binary size down.
// We only need playback (no capture) and the resource-manager-backed
// ma_engine + ma_sound + ma_sound_group APIs. dr_wav / dr_mp3 / dr_flac /
// stb_vorbis stay enabled — those are the decoders the existing assets need.
#define MA_NO_GENERATION       // no waveform/noise generators
#define MA_NO_ENCODING         // no encoder (we never write audio)
#define MA_NO_NULL              // keep the no-op fallback backend disabled

#include "miniaudio.h"
