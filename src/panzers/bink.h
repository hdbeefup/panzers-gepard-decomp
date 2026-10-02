// panzers/bink.h
// Bink intro video player, lifted from SSuperWindow in PANZERS.exe (HD, 2016).
//
// In the original the two handles live in SSuperWindow (+0x1B4 HBINK,
// +0x1B8 HBINKBUFFER) and the code is spread over:
//   0x657ef0 SSuperWindow::LoadBinkVideo  -> SBinkVideo::Open
//   0x65ba00 Bink frame step              -> SBinkVideo::Step (+ Draw)
//   0x65b830 Bink close                   -> SBinkVideo::Close
//   0x657540 (end of window creation)     -> SBinkVideo::SetSoundSystem
// The video is drawn with a BinkBuffer straight onto the window (DirectDraw/
// DIB blit by binkw32.dll), not into a D3D9 surface: the D3D device exists
// but nothing is rendered through it while the intro plays.
//
// Usage from SSuperWindow (P2-D):
//   SuperWindow::Play():   if (BinkShouldSkipIntro(cmdline, path) ||
//                              !intro.Open(path, hwnd, w, h, fullscreen, driver))
//                              { Initialize(); LoadMainMenu(); }
//   per frame (vtbl+0x90): if (intro.IsOpen() && !intro.Step())
//                              { Initialize(); LoadMainMenu(); }  // Step closed it
//   WM_PAINT while open:   intro.Draw();

#ifndef PANZERS_BINK_H
#define PANZERS_BINK_H

#include <windows.h>
#include <binkw32.h>
#include <mss.h>

struct SBinkVideo {
    HBINK       Bink;     // SSuperWindow+0x1B4
    HBINKBUFFER Buffer;   // SSuperWindow+0x1B8
    bool        Fullscreen;  // copy of the global fullscreen flag (0x929dfc) given to Open

    // Called by Close() before the buffer is released when the original would
    // call SSuperWindow vtbl+0xA4(1, 0) (fullscreen on Windows major > 9, see
    // BinkCloseNeedsDisplayRestore). Optional.
    void (*OnRestoreDisplay)(void* ctx);
    void* RestoreDisplayCtx;

    SBinkVideo();
    ~SBinkVideo();

    // BinkSetSoundSystem(BinkOpenMiles, driver). 0x657540 does it once after
    // the window and SMilesConcert exist; Open() repeats it, as 0x657ef0 does.
    static void SetSoundSystem(HDIGDRIVER miles_driver);

    // 0x657ef0. Returns false if BinkOpen fails; the original then goes
    // straight to SSuperWindow::Initialize + LoadMainMenu (caller's job here).
    // Panics (like the original) if BinkBufferOpen fails twice.
    bool Open(const char* filename, HWND hwnd, int screen_w, int screen_h,
              bool fullscreen, HDIGDRIVER miles_driver);

    // 0x65ba00. Returns true while the video is playing (also when it is not
    // time for the next frame yet). Returns false after the last frame was
    // shown; the video has then been closed (0x65b830) and the caller must
    // run SSuperWindow::Initialize + LoadMainMenu, as 0x65ba00 does.
    bool Step();

    // BinkBufferBlit(buffer, 0, 1): the blit 0x65ba00 does each decoded frame.
    void Draw();

    // 0x65b830
    void Close();

    bool IsOpen() const { return Bink != nullptr; }
    unsigned int FrameCount() const { return Bink ? Bink->Frames : 0; }
    unsigned int CurrentFrame() const { return Bink ? Bink->FrameNum : 0; }
};

// True when 0x65b830 would ask the window to restore the display mode:
// NetWkstaGetInfo(level 100).wki100_ver_major > 9 and fullscreen.
bool BinkCloseNeedsDisplayRestore(bool fullscreen);

// Skip policy (not in the original, which always plays the intro and only
// falls back when BinkOpen fails): skip if the command line contains
// "-nointro" or the file does not exist.
bool BinkShouldSkipIntro(const char* cmdline, const char* filename);

#endif // PANZERS_BINK_H
