// third_party/rad/mss.h
//
// HAND-WRITTEN declarations for the subset of the Miles Sound System 6.5c API
// (mss32.dll) that PANZERS.exe imports. This is NOT the RAD Game Tools SDK
// header and contains no SDK code. See third_party/rad/README.md.
//
// Sources for each declaration:
//   * the name and the stdcall argument byte count come from the decorated
//     IAT name in PANZERS.exe (HD) and the export in the shipped mss32.dll,
//     e.g. `_AIL_open_digital_driver@16` = 4 dword arguments;
//   * argument meaning/type comes from the PANZERS.exe call sites
//     (SMilesConcert, 0x684300..0x6876ff). Floats are passed where the call
//     site pushes an SSE register (volume, pan, positions).
//
// The DLL exports the decorated names WITH a leading underscore. To import
// `_AIL_x@N` the C symbol must be `_AIL_x` (the compiler adds another `_`),
// so every function is declared as `_AIL_x` and #defined back to `AIL_x`.

#ifndef THIRD_PARTY_RAD_MSS_H
#define THIRD_PARTY_RAD_MSS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int            S32;
typedef unsigned int   U32;
typedef float          F32;

// Opaque handles. In Miles 6.x the digital driver / samples / streams are
// pointers to internal structs; providers and enumeration cursors are U32.
typedef struct _MSS_DIG_DRIVER* HDIGDRIVER;
typedef struct _MSS_SAMPLE*     HSAMPLE;
typedef struct _MSS_STREAM*     HSTREAM;
typedef void*                   H3DSAMPLE;
typedef void*                   H3DPOBJECT;
typedef U32                     HPROVIDER;
typedef U32                     HPROENUM;
typedef S32                     M3DRESULT;   // 0 = M3D_NOERR

#define HPROENUM_FIRST 0

// AIL_sample_status / AIL_3D_sample_status: Panzers only tests for 4.
#define SMP_PLAYING 4

// AIL_get_preference indices used by PANZERS.exe (logged as
// "Mixer Channels" and "Output Buffer Size").
#define DIG_MIXER_CHANNELS     1
#define DIG_OUTPUT_BUFFER_SIZE 11

#define AILCALL     __stdcall
#define AILCALLBACK __stdcall
#define MSSIMPORT   __declspec(dllimport)

// File callbacks (AIL_set_file_callbacks). The handle is a U32 the
// application chooses; Panzers stores an SStream* in it.
typedef U32  (AILCALLBACK* AIL_file_open_callback)(char const* filename, U32* file_handle);
typedef void (AILCALLBACK* AIL_file_close_callback)(U32 file_handle);
typedef S32  (AILCALLBACK* AIL_file_seek_callback)(U32 file_handle, S32 offset, U32 type);
typedef U32  (AILCALLBACK* AIL_file_read_callback)(U32 file_handle, void* buffer, U32 bytes);

typedef void (AILCALLBACK* AILSTREAMCB)(HSTREAM stream);
typedef S32  (AILCALLBACK* AILLENGTHYCB)(U32 state, U32 user);

// --- driver / system ------------------------------------------------------
MSSIMPORT S32        AILCALL _AIL_startup(void);
MSSIMPORT void       AILCALL _AIL_shutdown(void);
MSSIMPORT char*      AILCALL _AIL_set_redist_directory(char const* dir);
MSSIMPORT char*      AILCALL _AIL_last_error(void);
MSSIMPORT S32        AILCALL _AIL_get_preference(U32 number);
MSSIMPORT HDIGDRIVER AILCALL _AIL_open_digital_driver(U32 frequency, S32 bits, S32 channels, U32 flags);
MSSIMPORT void       AILCALL _AIL_set_file_callbacks(AIL_file_open_callback opencb, AIL_file_close_callback closecb,
                                                     AIL_file_seek_callback seekcb, AIL_file_read_callback readcb);

// --- files / memory -------------------------------------------------------
MSSIMPORT void*      AILCALL _AIL_file_read(char const* filename, void* dest);
MSSIMPORT S32        AILCALL _AIL_file_size(char const* filename);
MSSIMPORT void       AILCALL _AIL_mem_free_lock(void* ptr);
MSSIMPORT S32        AILCALL _AIL_decompress_ASI(void const* indata, U32 insize, char const* filename_ext,
                                                 void** outdata, U32* outsize, AILLENGTHYCB callback);

// --- 2D samples -----------------------------------------------------------
MSSIMPORT HSAMPLE    AILCALL _AIL_allocate_sample_handle(HDIGDRIVER dig);
MSSIMPORT void       AILCALL _AIL_release_sample_handle(HSAMPLE s);
MSSIMPORT void       AILCALL _AIL_init_sample(HSAMPLE s);
MSSIMPORT S32        AILCALL _AIL_set_sample_file(HSAMPLE s, void const* file_image, S32 block);
MSSIMPORT void       AILCALL _AIL_start_sample(HSAMPLE s);
MSSIMPORT void       AILCALL _AIL_stop_sample(HSAMPLE s);
MSSIMPORT void       AILCALL _AIL_resume_sample(HSAMPLE s);
MSSIMPORT U32        AILCALL _AIL_sample_status(HSAMPLE s);
MSSIMPORT void       AILCALL _AIL_set_sample_volume_pan(HSAMPLE s, F32 volume, F32 pan);
MSSIMPORT void       AILCALL _AIL_sample_volume_pan(HSAMPLE s, F32* volume, F32* pan);
MSSIMPORT void       AILCALL _AIL_set_sample_loop_count(HSAMPLE s, S32 loop_count);
MSSIMPORT S32        AILCALL _AIL_sample_playback_rate(HSAMPLE s);
MSSIMPORT void       AILCALL _AIL_set_sample_playback_rate(HSAMPLE s, S32 rate);
MSSIMPORT S32        AILCALL _AIL_sample_granularity(HSAMPLE s);

// --- 3D providers / listener / samples ------------------------------------
MSSIMPORT S32        AILCALL _AIL_enumerate_3D_providers(HPROENUM* next, HPROVIDER* dest, char** name);
MSSIMPORT M3DRESULT  AILCALL _AIL_open_3D_provider(HPROVIDER lib);
MSSIMPORT void       AILCALL _AIL_close_3D_provider(HPROVIDER lib);
MSSIMPORT H3DPOBJECT AILCALL _AIL_open_3D_listener(HPROVIDER lib);
MSSIMPORT void       AILCALL _AIL_close_3D_listener(H3DPOBJECT listener);
MSSIMPORT void       AILCALL _AIL_set_3D_position(H3DPOBJECT obj, F32 x, F32 y, F32 z);
MSSIMPORT void       AILCALL _AIL_set_3D_orientation(H3DPOBJECT obj, F32 x_face, F32 y_face, F32 z_face,
                                                     F32 x_up, F32 y_up, F32 z_up);
MSSIMPORT H3DSAMPLE  AILCALL _AIL_allocate_3D_sample_handle(HPROVIDER lib);
MSSIMPORT void       AILCALL _AIL_release_3D_sample_handle(H3DSAMPLE s);
MSSIMPORT U32        AILCALL _AIL_set_3D_sample_file(H3DSAMPLE s, void const* file_image);
MSSIMPORT void       AILCALL _AIL_start_3D_sample(H3DSAMPLE s);
MSSIMPORT void       AILCALL _AIL_stop_3D_sample(H3DSAMPLE s);
MSSIMPORT void       AILCALL _AIL_resume_3D_sample(H3DSAMPLE s);
MSSIMPORT U32        AILCALL _AIL_3D_sample_status(H3DSAMPLE s);
MSSIMPORT U32        AILCALL _AIL_3D_sample_length(H3DSAMPLE s);
MSSIMPORT void       AILCALL _AIL_set_3D_sample_volume(H3DSAMPLE s, F32 volume);
MSSIMPORT void       AILCALL _AIL_set_3D_sample_playback_rate(H3DSAMPLE s, S32 rate);
// Panzers passes (min_distance, min_distance*100); the Miles 6 parameter
// order is (max_dist, min_dist). Declared by position, not by meaning.
MSSIMPORT void       AILCALL _AIL_set_3D_sample_distances(H3DSAMPLE s, F32 dist_a, F32 dist_b);
MSSIMPORT void       AILCALL _AIL_set_3D_sample_loop_count(H3DSAMPLE s, U32 loops);
MSSIMPORT void       AILCALL _AIL_set_3D_sample_loop_block(H3DSAMPLE s, S32 loop_start, S32 loop_end);
MSSIMPORT void       AILCALL _AIL_set_3D_sample_offset(H3DSAMPLE s, U32 offset);

// --- streams --------------------------------------------------------------
MSSIMPORT HSTREAM    AILCALL _AIL_open_stream(HDIGDRIVER dig, char const* filename, S32 stream_mem);
MSSIMPORT void       AILCALL _AIL_close_stream(HSTREAM stream);
MSSIMPORT void       AILCALL _AIL_start_stream(HSTREAM stream);
MSSIMPORT void       AILCALL _AIL_set_stream_loop_count(HSTREAM stream, S32 count);
MSSIMPORT void       AILCALL _AIL_set_stream_volume_pan(HSTREAM stream, F32 volume, F32 pan);
MSSIMPORT AILSTREAMCB AILCALL _AIL_register_stream_callback(HSTREAM stream, AILSTREAMCB callback);

#ifdef __cplusplus
}
#endif

// Map the SDK spelling onto the underscore-prefixed imports.
#define AIL_startup                    _AIL_startup
#define AIL_shutdown                   _AIL_shutdown
#define AIL_set_redist_directory       _AIL_set_redist_directory
#define AIL_last_error                 _AIL_last_error
#define AIL_get_preference             _AIL_get_preference
#define AIL_open_digital_driver        _AIL_open_digital_driver
#define AIL_set_file_callbacks         _AIL_set_file_callbacks
#define AIL_file_read                  _AIL_file_read
#define AIL_file_size                  _AIL_file_size
#define AIL_mem_free_lock              _AIL_mem_free_lock
#define AIL_decompress_ASI             _AIL_decompress_ASI
#define AIL_allocate_sample_handle     _AIL_allocate_sample_handle
#define AIL_release_sample_handle      _AIL_release_sample_handle
#define AIL_init_sample                _AIL_init_sample
#define AIL_set_sample_file            _AIL_set_sample_file
#define AIL_start_sample               _AIL_start_sample
#define AIL_stop_sample                _AIL_stop_sample
#define AIL_resume_sample              _AIL_resume_sample
#define AIL_sample_status              _AIL_sample_status
#define AIL_set_sample_volume_pan      _AIL_set_sample_volume_pan
#define AIL_sample_volume_pan          _AIL_sample_volume_pan
#define AIL_set_sample_loop_count      _AIL_set_sample_loop_count
#define AIL_sample_playback_rate       _AIL_sample_playback_rate
#define AIL_set_sample_playback_rate   _AIL_set_sample_playback_rate
#define AIL_sample_granularity         _AIL_sample_granularity
#define AIL_enumerate_3D_providers     _AIL_enumerate_3D_providers
#define AIL_open_3D_provider           _AIL_open_3D_provider
#define AIL_close_3D_provider          _AIL_close_3D_provider
#define AIL_open_3D_listener           _AIL_open_3D_listener
#define AIL_close_3D_listener          _AIL_close_3D_listener
#define AIL_set_3D_position            _AIL_set_3D_position
#define AIL_set_3D_orientation         _AIL_set_3D_orientation
#define AIL_allocate_3D_sample_handle  _AIL_allocate_3D_sample_handle
#define AIL_release_3D_sample_handle   _AIL_release_3D_sample_handle
#define AIL_set_3D_sample_file         _AIL_set_3D_sample_file
#define AIL_start_3D_sample            _AIL_start_3D_sample
#define AIL_stop_3D_sample             _AIL_stop_3D_sample
#define AIL_resume_3D_sample           _AIL_resume_3D_sample
#define AIL_3D_sample_status           _AIL_3D_sample_status
#define AIL_3D_sample_length           _AIL_3D_sample_length
#define AIL_set_3D_sample_volume       _AIL_set_3D_sample_volume
#define AIL_set_3D_sample_playback_rate _AIL_set_3D_sample_playback_rate
#define AIL_set_3D_sample_distances    _AIL_set_3D_sample_distances
#define AIL_set_3D_sample_loop_count   _AIL_set_3D_sample_loop_count
#define AIL_set_3D_sample_loop_block   _AIL_set_3D_sample_loop_block
#define AIL_set_3D_sample_offset       _AIL_set_3D_sample_offset
#define AIL_open_stream                _AIL_open_stream
#define AIL_close_stream               _AIL_close_stream
#define AIL_start_stream               _AIL_start_stream
#define AIL_set_stream_loop_count      _AIL_set_stream_loop_count
#define AIL_set_stream_volume_pan      _AIL_set_stream_volume_pan
#define AIL_register_stream_callback   _AIL_register_stream_callback

#endif // THIRD_PARTY_RAD_MSS_H
