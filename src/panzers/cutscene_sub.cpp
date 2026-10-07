// src/panzers/cutscene_sub.cpp
// The cut-scene subtitles (M5 agent CS): SSubtitler::ParseSubFile (HD
// 0x56fd70) reads "cutscenes/<n>/<n>.sub" into SGameLogic +0x278 (records of
// 0x10: start, end, SString text; the times in 1/25 s), and 0x5826c0 shows the
// line of a time in one board text frame (+0x274). Board only: nothing here
// touches the game state.

#include <stdlib.h>
#include <string.h>
#include "pzboard.h"
#include "board.h"
#include "worldapi.h"
#include "world.h"
#include "core_common.h"
#include "stream.h"
#include "logger.h"
#include "cutscene.h"
#include "iconcert.h"
#include "milesconcert.h"

extern SIBoard* Board;
extern SIConcert* Concert;

namespace pz {

namespace {

struct SSubLine { int Start; int End; char* Text; };              // 0x10 in HD (+0, +4, +8 SString)

SSubLine* g_Lines;                                                // SGameLogic +0x278
int       g_Count;                                                // +0x27c
int       g_Frame = -1;                                           // +0x274

// PANZERS 0x5634d0 (SDArray<SSubLine>::SetSize, here only to 0)
void ClearLines()
{
    for (int i = 0; i < g_Count; ++i)
        free(g_Lines[i].Text);
    free(g_Lines);
    g_Lines = nullptr;
    g_Count = 0;
}

void AddLine(int start, int end, const char* text)
{
    g_Lines = (SSubLine*)realloc(g_Lines, (g_Count + 1) * sizeof(SSubLine));   // 0x560ca0 add
    g_Lines[g_Count].Start = start;
    g_Lines[g_Count].End = end;
    g_Lines[g_Count].Text = _strdup(text);                        // 0x52c320
    ++g_Count;
}

} // namespace

// PANZERS 0x56fd70
// Lines "{start}{end}text" (blanks between lines skipped; the first line of
// the shipped files, "{1}{1}25.000", is the frame rate and never shows). A
// missing file: no subtitles. Any other line: Panic.
void PzSubtitlesLoad(const char* file)
{
    ClearLines();                                                 // 0x5634d0(0)
    char* buf = nullptr;
    unsigned size = 0;
    FileSystem.ReadFile(file, &buf, &size, nullptr);              // 0x65f8e0(file, &buf, &size, 0)
    if (!buf)
        return;
    char* p = buf;
    char* end = buf + size;                                       // ReadFile leaves one 0 byte after the data
    for (;;) {
        while (p < end && (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t'))
            ++p;
        if (end <= p)
            break;
        char* e;
        if (*p != '{')
            Logger.g->Panic("SSubtitler::ParseSubFile: Invalid sub file %s", file);
        ++p;
        long start = strtol(p, &e, 10);
        if (e == p || *e != '}' || e[1] != '{')
            Logger.g->Panic("SSubtitler::ParseSubFile: Invalid sub file %s", file);
        p = e + 2;
        long stop = strtol(p, &e, 10);
        if (e == p || *e != '}')
            Logger.g->Panic("SSubtitler::ParseSubFile: Invalid sub file %s", file);
        char* text = e + 1;
        for (p = text; p < end && *p != '\n' && *p != '\r'; ++p) {
        }
        *p = 0;
        ++p;
        AddLine((int)start, (int)stop, text);
    }
    operator delete[](buf);                                       // 0x76654a
    if (g_Count == 0 || !Board || !g_World)
        return;
    g_Frame = Board->CreateFrame(FT_TEXT, g_World->LoadIconParent, 0x200, 0x26e, 0, false);   // board +0x08(2, World+0x74e0, 0x200, 0x26e, 0, 0)
    Board->ShowFrame(g_Frame, true);                              // (SWINE frames start hidden)
    Logger.g->Log(0, "PZM5: cut-scene subtitles %s: %d lines", file, g_Count);
}

// PANZERS 0x5826c0
// The first line with start <= time < end, or an empty text.
void PzSubtitlesShow(int time)
{
    if (g_Count == 0 || g_Frame < 0 || !Board)
        return;
    const char* text = "";
    for (int i = 0; i < g_Count; ++i)
        if (g_Lines[i].Start <= time && time < g_Lines[i].End) {
            text = g_Lines[i].Text;
            break;
        }
    Board->SetText(g_Frame, g_PzFont[PZF_SANS28_SHADOW], 2, text);   // board +0x34(frame, 4, 2, text, 0)
}

// The end of the cut-scene (0x565390 / 0x5652d0): the frame goes, the lines
// are cleared.
void PzSubtitlesEnd()
{
    if (g_Count == 0)
        return;
    if (g_Frame >= 0 && Board)
        Board->DestroyFrame(g_Frame);                             // board +0x0c(+0x274)
    g_Frame = -1;
    ClearLines();                                                 // 0x5634d0(0)
}

// The cut-scene sound (Concert, DAT_008f1c5c): 0x56f530 resumes the sounds
// (+0x64) and starts "<name>.mp3" (+0x2c(file, 1, 0.0, 0.0, 1): looping until
// 0x565390 removes it, +0x3c). A missing file gives -1 (tutorial-01 has none,
// in the original too).
void PzCutsceneSoundResume()
{
    if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert))
        pc->ResumeAllSounds();                                    // Concert +0x64
}

int PzCutsceneSoundStart(const char* file)
{
    SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert);
    return pc ? pc->CreateSoundEx(file, true, 0.0f, 0.0f, true) : -1;   // Concert +0x2c(file, 1, 0, 0, 1)
}

void PzCutsceneSoundStop(int id)
{
    if (Concert)
        Concert->RemoveSound(id);                                 // Concert +0x3c (-1: nothing)
}

} // namespace pz
