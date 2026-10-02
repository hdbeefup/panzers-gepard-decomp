// tools/soundtest/soundtest.cpp
// Smoke test for the lifted Panzers sound path (SMilesConcert on mss32.dll)
// and the Bink intro player (SBinkVideo on binkw32.dll).
//
// Run it from a directory that has the game's mss32.dll, binkw32.dll, the
// miles\ provider folder and intro.bik, plus loose files extracted from
// panzers.pak:
//   sounds/menu/button_down.wav   (menu click, a 2D sample)
//   music/menu.mp3                (menu music, a Miles MP3 stream)
//
//   soundtest.exe [-frames N] [-wav name] [-mp3 name] [-bik name]
//
// Results go to soundtest.log (and stdout); the engine log (SLogger) goes to
// log\soundtest <time>.txt. Audible output can't be checked automatically,
// so success is judged by Miles/Bink return values and the frame counter.
// Exit code 0 = every check passed.

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <logger.h>
#include <stream.h>
#include <timer.h>
#include <milesconcert.h>
#include <bink.h>

// Globals the core library expects the game to define (see src/stubs).
STimer Timer;

static FILE* g_out = nullptr;
static int g_failures = 0;

static void Out(const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fputs(buf, stdout);
    fputs("\n", stdout);
    if (g_out) {
        fputs(buf, g_out);
        fputs("\n", g_out);
        fflush(g_out);
    }
}

static void Check(bool ok, const char* what)
{
    Out("[%s] %s", ok ? "OK  " : "FAIL", what);
    if (!ok)
        ++g_failures;
}

static void PumpMessages()
{
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static void PumpFor(DWORD ms, SIConcert* concert)
{
    DWORD end = GetTickCount() + ms;
    while ((int)(end - GetTickCount()) > 0) {
        PumpMessages();
        if (concert)
            concert->Update(false);
        Sleep(10);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int main(int argc, char** argv)
{
    int frames_to_step = 60;
    const char* wav = "menu/button_down.wav";
    const char* mp3 = "music/menu.mp3";
    const char* bik = "intro.bik";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-frames") && i + 1 < argc) frames_to_step = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-wav") && i + 1 < argc) wav = argv[++i];
        else if (!strcmp(argv[i], "-mp3") && i + 1 < argc) mp3 = argv[++i];
        else if (!strcmp(argv[i], "-bik") && i + 1 < argc) bik = argv[++i];
    }

    g_out = fopen("soundtest.log", "w");
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    Out("soundtest: Panzers SMilesConcert + SBinkVideo smoke test");
    Out("cwd: %s", cwd);

    // Engine logger + file system, the way WinMain/SSuperWindow set them up
    // (search path = current directory, no paks).
    Logger.g = new SLogger((char*)"soundtest", (char*)"soundtest", true, 2);
    FileSystem.AddSearchPath("");

    HMODULE mss = LoadLibraryA("mss32.dll");
    HMODULE bnk = LoadLibraryA("binkw32.dll");
    Check(mss != nullptr, "mss32.dll found");
    Check(bnk != nullptr, "binkw32.dll found");
    if (!mss || !bnk) {
        Out("RESULT: FAIL (%d failures)", g_failures);
        return 1;
    }

    // A plain window for the BinkBuffer, shown without activation so the
    // test does not steal focus.
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = "PanzersSoundTest";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowExA(WS_EX_NOACTIVATE, wc.lpszClassName, "soundtest", WS_OVERLAPPEDWINDOW,
                                40, 40, 640 + 16, 480 + 39, nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    Check(hwnd != nullptr, "test window created");

    // --- Miles: SMilesConcert the Panzers way (factory 0x684fb0) ---------
    SIPanzersConcert* concert = CreateMilesConcert(hwnd, "Miles Fast 2D Positional Audio", 0);
    Check(concert != nullptr && g_MilesConcert == concert, "Miles init: SMilesConcert created");
    Out("  digital driver = %p, 3D provider = 0x%X (fallback 0x%X), listener = %p, providers = %d",
        g_MilesConcert->DigitalDriver, g_MilesConcert->Provider, g_MilesConcert->FallbackProvider,
        g_MilesConcert->Listener, g_MilesConcert->Providers.size);
    for (int i = 0; i < g_MilesConcert->Providers.size; i++)
        Out("    provider 0x%02X: %s", g_MilesConcert->Providers.array[i].Id,
            g_MilesConcert->Providers.array[i].Name.buf ? g_MilesConcert->Providers.array[i].Name.buf : "");
    Check(concert->GetDigitalDriver() != nullptr, "Miles digital driver open (44100 Hz, 16 bit, stereo)");
    Check(g_MilesConcert->Listener != nullptr, "Miles 3D listener open");

    // options.ini defaults: Music 6, Sound effect 9, Voice 8 (0..10)
    concert->SetVolume(0, 6);
    concert->SetVolume(1, 9);
    concert->SetVolume(2, 8);

    // --- 2D sample: the menu button click --------------------------------
    int cacheidx = concert->PrecacheSound(wav, false);
    Out("  PrecacheSound(\"%s\") = %d", wav, cacheidx);
    Check(cacheidx >= 0, "sample loaded (PrecacheSound via AIL_file_read + engine file callbacks)");
    if (cacheidx >= 0)
        Out("  cached size = %.0f bytes", g_MilesConcert->CacheAt(cacheidx).Size);
    float duration = concert->PlaySound(wav, -12.0f, 0.0f, -1);
    Out("  PlaySound(\"%s\", -12 dB, pan 0, channel -1) = %.3f (approx. seconds)", wav, duration);
    Check(duration > 0.0f && g_MilesConcert->TransientSounds == 1, "sample started (AIL_start_sample, 1 transient sound)");
    int playing_id = g_MilesConcert->Sounds.GetNext(-1);
    if (playing_id >= 0) {
        U32 st = AIL_sample_status(g_MilesConcert->SoundAt(playing_id).Sample);
        Out("  AIL_sample_status = %u (4 = playing)", st);
        Check(st == SMP_PLAYING, "sample status is SMP_PLAYING");
    }
    PumpFor(1200, concert);
    Check(g_MilesConcert->TransientSounds == 0, "finished sample reclaimed by Update()");

    // --- Stream: the menu music, as SSuperWindow::Initialize does --------
    concert->ClearPlaylist();
    concert->AddToPlaylist(mp3);
    concert->StartPlaylist(true);
    Out("  StartPlaylist(\"%s\"): HSTREAM = %p", mp3, g_MilesConcert->Stream);
    Check(concert->IsStreamPlaying(), "stream started (AIL_open_stream on MP3 via mssmp3.asi)");
    PumpFor(2000, concert);
    Check(concert->IsStreamPlaying(), "stream still open after 2 s");
    concert->StopStream(false);
    Check(!concert->IsStreamPlaying(), "stream stopped");

    // --- Bink intro ------------------------------------------------------
    SBinkVideo video;
    bool skip = BinkShouldSkipIntro(GetCommandLineA(), bik);
    Out("  BinkShouldSkipIntro = %d", skip);
    bool opened = video.Open(bik, hwnd, 640, 480, false, concert->GetDigitalDriver());
    Check(opened, "Bink opened (BinkOpen + BinkBufferOpen, sound via BinkOpenMiles)");
    if (opened) {
        Out("  Bink: %ux%u, %u frames, buffer %ux%u surface type 0x%X",
            video.Bink->Width, video.Bink->Height, video.FrameCount(),
            video.Buffer->Width, video.Buffer->Height, video.Buffer->SurfaceType);
        int decoded = 0;
        unsigned int last = video.CurrentFrame();
        DWORD t0 = GetTickCount();
        while (decoded < frames_to_step && video.IsOpen() && GetTickCount() - t0 < 30000) {
            PumpMessages();
            if (!video.Step())
                break;   // video ended (closed by Step)
            if (video.IsOpen() && video.CurrentFrame() != last) {
                last = video.CurrentFrame();
                ++decoded;
            }
            Sleep(1);
        }
        DWORD ms = GetTickCount() - t0;
        Out("  stepped %d frames in %lu ms, Bink frame counter = %u", decoded, ms, video.CurrentFrame());
        char what[64];
        sprintf(what, "Bink stepped %d frames", frames_to_step);
        Check(decoded == frames_to_step && video.CurrentFrame() == (unsigned)frames_to_step + 1, what);
        video.Close();
        Check(!video.IsOpen(), "Bink closed");
    }

    // --- Shutdown ---------------------------------------------------------
    concert->RemoveAllSounds();
    int kept = concert->CleanupSoundCache();
    Out("  CleanupSoundCache kept %d", kept);
    concert->Release();
    Check(g_MilesConcert == nullptr, "SMilesConcert released (AIL_shutdown)");

    DestroyWindow(hwnd);
    Out("RESULT: %s (%d failures)", g_failures ? "FAIL" : "PASS", g_failures);
    if (g_out)
        fclose(g_out);
    return g_failures ? 1 : 0;
}
