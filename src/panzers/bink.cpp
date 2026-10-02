// panzers/bink.cpp
// Bink intro video player, lifted from SSuperWindow in PANZERS.exe (HD, 2016).
// See bink.h for the mapping onto the original functions.

#include "bink.h"
#include <lm.h>
#include <string.h>
#include <logger.h>

// The SDK version string PANZERS.exe logs (0x80a748). The shipped
// binkw32.dll is 1.5q; the exe still prints the version it was built with.
static const char kBinkVersion[] = "1.6g";

// BinkBufferOpen flags pushed by 0x657ef0. First attempt 0x44800000,
// second attempt 0x44000000 (the first without bit 0x00800000).
static const U32 kBinkBufferFlagsFirst  = 0x44800000;
static const U32 kBinkBufferFlagsSecond = 0x44000000;

SBinkVideo::SBinkVideo()
    : Bink(nullptr), Buffer(nullptr), Fullscreen(false),
      OnRestoreDisplay(nullptr), RestoreDisplayCtx(nullptr)
{
}

SBinkVideo::~SBinkVideo()
{
    if (Bink || Buffer)
        Close();
}

// PANZERS 0x657540 (tail: BinkSetSoundSystem)
void SBinkVideo::SetSoundSystem(HDIGDRIVER miles_driver)
{
    BinkSetSoundSystem((BINKSNDSYSOPEN)BinkOpenMiles, (U32)miles_driver);
}

// PANZERS 0x657ef0
bool SBinkVideo::Open(const char* filename, HWND hwnd, int screen_w, int screen_h,
                      bool fullscreen, HDIGDRIVER miles_driver)
{
    Fullscreen = fullscreen;   // the original reads the global fullscreen flag 0x929dfc
    SetSoundSystem(miles_driver);
    Logger.g->Log(0, "[BINK] Starting Bink version %s", kBinkVersion);
    Bink = BinkOpen(filename, 0);
    if (!Bink) {
        Logger.g->Log(0, "SSuperWindow::LoadBinkVideo: BinkOpen(%s) failed.", filename);
        return false;   // original: SSuperWindow::Initialize(); LoadMainMenu();
    }
    Logger.g->Log(0, "[BINK] resolutions: Bink %dx%d, screen %dx%d",
                  Bink->Width, Bink->Height, screen_w, screen_h);
    if (fullscreen)
        BinkBufferSetResolution(screen_w, screen_h, 32);

    Buffer = BinkBufferOpen(hwnd, Bink->Width, Bink->Height, kBinkBufferFlagsFirst);
    if (!Buffer) {
        Logger.g->Log(0, "[BINK] BinkBufferOpen - second attempt");
        Buffer = BinkBufferOpen(hwnd, Bink->Width, Bink->Height, kBinkBufferFlagsSecond);
    }
    if (!Buffer) {
        BinkClose(Bink);
        Bink = nullptr;
        Logger.g->Panic("SSuperWindow::LoadBinkVideo: BinkBufferOpen() failed.");
    }
    if (!BinkBufferSetScale(Buffer, screen_w, screen_h))
        Logger.g->Warning("SSuperWindow::LoadBinkVideo: BinkBufferSetScale() failed.");
    return true;
}

void SBinkVideo::Draw()
{
    if (Buffer)
        BinkBufferBlit(Buffer, nullptr, 1);
}

// PANZERS 0x65ba00
bool SBinkVideo::Step()
{
    if (!Bink)
        return true;
    if (BinkWait(Bink)) {
        // Original: Timer.Wait(0.0005, true) (0x661970). It sleeps only when
        // the remaining time exceeds 0.001 s, so with 0.0005 it never sleeps;
        // it just re-marks the timer. Nothing to do here.
        return true;
    }
    BinkDoFrame(Bink);
    if (BinkBufferLock(Buffer)) {
        BinkCopyToBuffer(Bink, Buffer->Buffer, Buffer->BufferPitch, Buffer->Height,
                         0, 0, Buffer->SurfaceType);
        BinkBufferUnlock(Buffer);
    }
    Draw();
    if (Bink->FrameNum == Bink->Frames) {
        Close();
        return false;   // original: SSuperWindow::Initialize(); LoadMainMenu();
    }
    BinkNextFrame(Bink);
    return true;
}

// PANZERS 0x65b830 (the window-mode restore is delegated to OnRestoreDisplay)
bool BinkCloseNeedsDisplayRestore(bool fullscreen)
{
    WKSTA_INFO_100* info = nullptr;
    DWORD major = 0;
    if (NetWkstaGetInfo(nullptr, 100, (LPBYTE*)&info) == NERR_Success && info)
        major = info->wki100_ver_major;
    if (info)
        NetApiBufferFree(info);
    return major > 9 && fullscreen;
}

void SBinkVideo::Close()
{
    if (OnRestoreDisplay && BinkCloseNeedsDisplayRestore(Fullscreen))
        OnRestoreDisplay(RestoreDisplayCtx);
    if (Buffer) {
        BinkBufferClose(Buffer);
        Buffer = nullptr;
    }
    if (Bink) {
        BinkClose(Bink);
        Bink = nullptr;
    }
}

bool BinkShouldSkipIntro(const char* cmdline, const char* filename)
{
    if (cmdline && strstr(cmdline, "-nointro"))
        return true;
    if (!filename || GetFileAttributesA(filename) == INVALID_FILE_ATTRIBUTES)
        return true;
    return false;
}
