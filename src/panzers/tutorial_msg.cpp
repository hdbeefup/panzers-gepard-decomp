// src/panzers/tutorial_msg.cpp
// Mission texts of the trigger actions (M4 agent T):
//  - SPanzersCampaign::InitTriggerTexts 0x594540: "maps/<map>.txt", "#key"
//    blocks of lines with <left>/<center>/<white>/<red>/<yellow>/<green>/
//    <blue> tags (campaign +0x120 in HD; kept here);
//  - SGameLogic 0x5815e0: the two static lines above the HUD (action 0x40
//    "echo"), cleared by 0x5638b0;
//  - SGameLogic::StaticMessage 0x56a480: the fading message lines (actions
//    0x23 / 0x41, "Loading cutscene..."), cleared by 0x563860, aged by
//    0x578b00.
// Board text frames only: nothing here touches the game state.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "pzboard.h"
#include "board.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "campaign.h"
#include "core_common.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"
#include "iconcert.h"
#include "timer.h"
#include "cutscene.h"

extern SIBoard* Board;
extern SIConcert* Concert;

namespace pz {

namespace {

struct STextLine { int Align; int Color; char* Text; };   // 0x10 in HD (+0, +4, +8 SString)
struct STextBlock { char* Key; STextLine* Lines; int Count; };   // 0x14 in HD

STextBlock* g_Blocks;
int g_BlockCount;
char g_LoadedFor[260];

// HD 0x8dbacc (static lines) and 0x8dbae0 (fading lines): white, red,
// yellow, green, blue.
const unsigned kColors[5] = {0xffffffff, 0xffff3f3f, 0xffffff3f, 0xff3fff3f, 0xff3fffff};

// SGameLogic +0x84 / +0x88 (static lines) and +0x90 / +0x94 / +0xf0 (fading).
int g_StaticCount;
int g_Static[2] = {-1, -1};
int g_FadeCount;
int g_Fade[0x17];
int g_FadeTimer[0x17];
bool g_FadeInit;

void FreeBlocks()
{
    for (int i = 0; i < g_BlockCount; ++i) {
        for (int k = 0; k < g_Blocks[i].Count; ++k)
            free(g_Blocks[i].Lines[k].Text);
        free(g_Blocks[i].Lines);
        free(g_Blocks[i].Key);
    }
    free(g_Blocks);
    g_Blocks = nullptr;
    g_BlockCount = 0;
}

int Font()
{
    return g_PzFont[5];                                           // SGameLogic +0x80 = 5
}

// HD board +0x90: the longest head of `text` that fits `width` pixels
// (broken at a space), the rest in *rest. Returns true while text remains.
bool SplitLine(const char* text, int width, char* head, int headSize, const char** rest)
{
    int n = (int)strlen(text);
    int best = n;
    int w = 0, h = 0;
    if (Board)
        Board->GetTextExtent(Font(), text, n, &w, &h, 1.0f);
    if (w > width) {
        best = 0;
        for (int i = 1; i <= n; ++i) {
            if (i < n && text[i] != ' ')
                continue;
            Board->GetTextExtent(Font(), text, i, &w, &h, 1.0f);
            if (w > width)
                break;
            best = i;
        }
        if (best == 0)
            best = n;
    }
    int c = best < headSize - 1 ? best : headSize - 1;
    memcpy(head, text, c);
    head[c] = 0;
    const char* r = text + best;
    while (*r == ' ')
        ++r;
    *rest = r;
    return *r != 0;
}

void InitFade()
{
    if (g_FadeInit)
        return;
    g_FadeInit = true;
    for (int i = 0; i < 0x17; ++i) {
        g_Fade[i] = -1;
        g_FadeTimer[i] = -2;
    }
}

} // namespace

// PANZERS 0x594540 (SPanzersCampaign::InitTriggerTexts)
void PzLoadTriggerTexts(const char* file)
{
    FreeBlocks();                                                 // 0x591c90(0)
    char* buf = nullptr;
    unsigned size = 0;
    if (FileSystem.Stat(file, nullptr) != 0)                      // 0x65f420(file, 0) == 0: no texts
        return;
    FileSystem.ReadFile(file, &buf, &size, "SPanzersCampaign::InitTriggerTexts");   // 0x65f8e0
    Logger.g->Log(0, "Parsing %s", file);
    const char* p = buf;
    const char* end = buf + size;
    if (p >= end || *p != '#')
        Logger.g->Panic("SPanzersCampaign::InitTriggerTexts: %s: Syntax error", file);
    int align = 0, color = 0;
    STextBlock* cur = nullptr;
    while (p < end) {
        // one line
        const char* e = p;
        while (e < end && *e != '\n')
            ++e;
        int n = (int)(e - p);
        while (n > 0 && (p[n - 1] == '\r'))
            --n;
        if (n > 0 && p[0] == '#') {
            // "#key": a new block (0x591660); the key ends at a space / CR / LF
            int k = 1;
            while (k < n && p[k] != ' ' && p[k] != '\r')
                ++k;
            g_Blocks = (STextBlock*)realloc(g_Blocks, (g_BlockCount + 1) * sizeof(STextBlock));
            cur = &g_Blocks[g_BlockCount++];
            cur->Key = (char*)malloc(k);
            memcpy(cur->Key, p + 1, k - 1);
            cur->Key[k - 1] = 0;
            cur->Lines = nullptr;
            cur->Count = 0;
            align = 0;
            color = 0;
        } else if (n > 0 && p[0] == '<') {
            const char* t = p + 1;
            const char* te = (const char*)memchr(t, '>', n - 1);
            if (!te)
                Logger.g->Panic("SPanzersCampaign::InitTutorialTexts: %s: Syntax error", file);
            char tag[32];
            int tl = (int)(te - t) < 31 ? (int)(te - t) : 31;
            memcpy(tag, t, tl);
            tag[tl] = 0;
            if (_stricmp(tag, "left") == 0) align = 0;
            else if (_stricmp(tag, "center") == 0) align = 1;
            else if (_stricmp(tag, "white") == 0) color = 0;
            else if (_stricmp(tag, "red") == 0) color = 1;
            else if (_stricmp(tag, "yellow") == 0) color = 2;
            else if (_stricmp(tag, "green") == 0) color = 3;
            else if (_stricmp(tag, "blue") == 0) color = 4;
            else
                Logger.g->Panic("SPanzersCampaign::InitTutorialTexts: %s: Unsupported tag: %s", file, tag);
            // HD reads the rest of the tag line as a text line.
            const char* r = te + 1;
            int rn = (int)(p + n - r);
            if (cur && rn > 0) {
                cur->Lines = (STextLine*)realloc(cur->Lines, (cur->Count + 1) * sizeof(STextLine));
                STextLine& l = cur->Lines[cur->Count++];
                l.Align = align;
                l.Color = color;
                l.Text = (char*)malloc(rn + 1);
                memcpy(l.Text, r, rn);
                l.Text[rn] = 0;
            }
        } else if (cur) {
            cur->Lines = (STextLine*)realloc(cur->Lines, (cur->Count + 1) * sizeof(STextLine));
            STextLine& l = cur->Lines[cur->Count++];
            l.Align = align;
            l.Color = color;
            l.Text = (char*)malloc(n + 1);
            memcpy(l.Text, p, n);
            l.Text[n] = 0;
        }
        p = e + 1;
    }
    free(buf);
}

// The texts of the running mission ("maps/<map>.txt", 0x5940ec "txt"). The
// recompile loads them on first use instead of in the map load.
static void EnsureTexts()
{
    if (!g_Campaign)
        return;
    const char* map = g_Campaign->GetMapName();
    if (!map)
        return;
    if (_stricmp(g_LoadedFor, map) == 0)
        return;
    strncpy(g_LoadedFor, map, sizeof(g_LoadedFor) - 1);
    char file[300];
    strncpy(file, map, sizeof(file) - 5);
    file[sizeof(file) - 5] = 0;
    char* dot = strrchr(file, '.');
    if (dot)
        strcpy(dot + 1, "txt");                                   // 0x597390(".txt")
    else
        strcat(file, ".txt");
    PzLoadTriggerTexts(file);
}

// PANZERS 0x5638b0
void PzMessagesClearStatic(SGameLogic* gl)
{
    (void)gl;
    for (int i = 0; i < 2; ++i) {
        if (g_Static[i] >= 0 && Board)
            Board->DestroyFrame(g_Static[i]);                     // board +0x0c
        g_Static[i] = -1;
    }
    g_StaticCount = 0;                                            // +0x84
}

// PANZERS 0x563860
void PzMessagesClearFading(SGameLogic* gl)
{
    (void)gl;
    InitFade();
    for (int i = 0; i < 0x17; ++i) {
        if (g_Fade[i] >= 0) {
            if (Board)
                Board->DestroyFrame(g_Fade[i]);
            g_FadeTimer[i] = -1;
            g_Fade[i] = -1;
        }
    }
    g_FadeCount = 0;                                              // +0x90
}

// PANZERS 0x5815e0
// A static line above the HUD (at most two, x 0x14, y 0x212 + n * 0x12,
// wrapped at 0x3d8 pixels).
void PzMessageStatic(SGameLogic* gl, const char* text, int color)
{
    (void)gl;
    if (color < 0 || color > 4) {
        Logger.g->Warning("SGameLogic::StaticMessage(): Invalid coloridx.");
        color = 0;
    }
    if (!Board || !g_World)
        return;
    const char* rest = text ? text : "";
    bool more;
    do {
        if (g_StaticCount > 1)
            break;
        char line[512];
        more = SplitLine(rest, 0x3d8, line, sizeof(line), &rest);   // board +0x90(font, 0x3d8, ...)
        int f = Board->CreateFrame(FT_TEXT, g_World->LoadIconParent, 0x14, g_StaticCount * 0x12 + 0x212, 0, true);
        g_Static[g_StaticCount] = f;
        Board->SetText(f, Font(), 0, line);                       // board +0x34
        Board->SetTextColor(f, kColors[color]);                   // board +0x2c (0x8dbacc)
        ++g_StaticCount;
    } while (more);
}

// PANZERS 0x56a480 (SGameLogic::StaticMessage)
// A fading line: the older lines move up 0x12 pixels, the new one at y
// 0x1f9 (wrapped at 0x180 pixels); 400 ticks of life (+0xf0).
void PzMessageFading(SGameLogic* gl, const char* text, int color)
{
    (void)gl;
    InitFade();
    if (color < 0 || color > 4) {
        Logger.g->Warning("SGameLogic::StaticMessage(): Invalid coloridx.");
        color = 0;
    }
    Logger.g->Log(1, "FadingMessage: '%s'.", text ? text : "");
    if (!Board || !g_World)
        return;
    const char* rest = text ? text : "";
    bool more;
    do {
        char line[512];
        more = SplitLine(rest, 0x180, line, sizeof(line), &rest);
        if (g_FadeCount == 0x17) {
            Board->DestroyFrame(g_Fade[0]);
            for (int i = 0; i < g_FadeCount - 1; ++i) {
                g_Fade[i] = g_Fade[i + 1];
                g_FadeTimer[i] = g_FadeTimer[i + 1];
            }
            --g_FadeCount;
            g_Fade[g_FadeCount] = -1;
            g_FadeTimer[g_FadeCount] = -2;
        }
        for (int i = 0; i < g_FadeCount; ++i)
            Board->MoveFrame(g_Fade[i], 0x14, (i - g_FadeCount) * 0x12 + 0x1f9);   // board +0x10
        g_FadeTimer[g_FadeCount] = 400;
        int f = Board->CreateFrame(FT_TEXT, g_World->LoadIconParent, 0x14, 0x1f9, 0, true);
        g_Fade[g_FadeCount] = f;
        Board->SetText(f, Font(), 0, line);
        Board->SetTextColor(f, kColors[color]);                   // 0x8dbae0
        ++g_FadeCount;
    } while (more);
}

// PANZERS 0x578b00 (the fading-line part; HD also fades the alpha over the
// last ticks: here the line goes when its time is up)
void PzMessagesTick(SGameLogic* gl)
{
    (void)gl;
    InitFade();
    if (!Board)
        return;
    while (g_FadeCount > 0) {
        bool removed = false;
        for (int i = 0; i < g_FadeCount; ++i) {
            if (g_FadeTimer[i] > 0)
                --g_FadeTimer[i];
        }
        if (g_FadeTimer[0] == 0) {
            Board->DestroyFrame(g_Fade[0]);
            for (int i = 0; i < g_FadeCount - 1; ++i) {
                g_Fade[i] = g_Fade[i + 1];
                g_FadeTimer[i] = g_FadeTimer[i + 1];
            }
            --g_FadeCount;
            g_Fade[g_FadeCount] = -1;
            g_FadeTimer[g_FadeCount] = -2;
            for (int i = 0; i < g_FadeCount; ++i)
                Board->MoveFrame(g_Fade[i], 0x14, (i - g_FadeCount + 1) * 0x12 + 0x1f9 - 0x12);
            removed = true;
        }
        if (!removed)
            break;
        break;
    }
}

// RunTriggers 0x40 (echo) / 0x41 (message): the lines of the text block
// `key` (campaign +0x120) as static (0x5815e0) or fading (0x56a480) lines.
// 0x40 clears the static lines first (0x5638b0).
void PzTriggerTextBlock(SGameLogic* gl, const char* key, bool fading)
{
    if (!g_Campaign)
        return;
    EnsureTexts();
    if (!fading)
        PzMessagesClearStatic(gl);
    for (int i = 0; i < g_BlockCount; ++i) {
        if (_stricmp(g_Blocks[i].Key, key) != 0)                  // 0x55cbd0
            continue;
        for (int k = 0; k < g_Blocks[i].Count; ++k) {
            const STextLine& l = g_Blocks[i].Lines[k];
            if (fading)
                PzMessageFading(gl, l.Text, l.Color);
            else
                PzMessageStatic(gl, l.Text, l.Color);
        }
        break;
    }
}

// PANZERS 0x58c5d0 (SInGameAnimLogic +0x24 box frame)
// The cut-scene colour over the whole view: no frame while the colour is 0,
// else a box frame (board +0x08 type 4) of the viewport's size in that
// colour (+0x3c). The recompile sizes it to cover any window.
static int g_ColourBox = -1;
void PzCutsceneOverlay(unsigned argb)
{
    if (!Board)
        return;
    if (argb == 0) {
        if (g_ColourBox != -1) {
            Board->DestroyFrame(g_ColourBox);                     // board +0x0c
            g_ColourBox = -1;
        }
        return;
    }
    if (g_ColourBox == -1)
        g_ColourBox = Board->CreateFrame(FT_BOX, 0, 0, 0, 0, true);   // board +0x08(4, 0, 0, 0, 0, 1)
    Board->ResizeFrame(g_ColourBox, 4096, 4096);                  // board +0x14 (viewport +0x10 size in HD)
    Board->SetBoxColor(g_ColourBox, argb);                        // board +0x3c
}

// ---------------------------------------------------------------------------
// The trigger speech queue (action 0x42): HD World +0x7280 SDArray<SString>
// and the time +0x7298 until which the current file plays.

static char** g_Speech;
static int g_SpeechCount;
static double g_SpeechUntil;

// PANZERS 0x600770 (+ 0x5d8a60, SDArray<SString>::Add)
void PzSpeechQueue(const char* file)
{
    g_Speech = (char**)realloc(g_Speech, (g_SpeechCount + 1) * sizeof(char*));
    g_Speech[g_SpeechCount++] = _strdup(file ? file : "");
}

// PANZERS 0x607f50 (the trigger speech part; the unit speech heap +0x726c
// stays as before)
// When the previous file has ended (wall clock), the first queued file plays
// (Concert +0x50(name, 0, 0, -2) returns its length) and leaves the queue
// (0x5f7220(0)).
void PzSpeechTick()
{
    if (!Concert) {
        Logger.g->Warning("SWorld::UpdateSpeech(): Concert is NULL.");
        return;
    }
    double now = (double)(float)((double)Timer.GetTickValue() / 1000.0);   // 0x661800, kept as a float
    if (g_SpeechUntil <= now && g_SpeechCount > 0) {
        float len = Concert->PlaySound(g_Speech[0], 0.0f, 0.0f, -2);    // concert +0x50
        g_SpeechUntil = (double)len + now;                        // +0x7298
        free(g_Speech[0]);
        memmove(g_Speech, g_Speech + 1, (g_SpeechCount - 1) * sizeof(char*));
        --g_SpeechCount;
    }
}

} // namespace pz
