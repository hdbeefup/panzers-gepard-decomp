// src/game/m3deepcap.cpp
// Recompile-only test hook (nothing here exists in the HD exe): the same
// per-frame memory capture as the scratch debugger tool ucapdbg2.py takes of
// the original, so that a Python comparator can diff ours with HD's blob by
// blob. PZ_M3_DEEPCAP=<file>,<first frame>,<last frame>. Format: per frame
// "PZDC" u32 frame, u32 seed, u32 live, then entries (u16 key length, key
// text such as "u/346" or "d/346/0", u32 address, u32 length, bytes), ended by
// a zero key length. Reads use SEH: the HD sizes exceed some recompile objects.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "gamelogic.h"
#include "unit.h"
#include "world.h"
#include "worldapi.h"

namespace pz {

namespace {

FILE* s_File;
int s_F0 = -1, s_F1 = -1;
bool s_Init;
unsigned char s_Buf[0x1000000];

int SafeRead(const void* p, int n)
{
    if (!p || n <= 0 || n > (int)sizeof(s_Buf))
        return 0;
    __try {
        memcpy(s_Buf, p, n);
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // halve like rdbf()
    }
    while (n >= 0x20) {
        n /= 2;
        __try {
            memcpy(s_Buf, p, n);
            return n;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }
    return 0;
}

void Put(const char* key, const void* p, int n)
{
    int got = SafeRead(p, n);
    if (!got)
        return;
    unsigned short kl = (unsigned short)strlen(key);
    unsigned a = (unsigned)(size_t)p, l = (unsigned)got;
    fwrite(&kl, 2, 1, s_File);
    fwrite(key, 1, kl, s_File);
    fwrite(&a, 4, 1, s_File);
    fwrite(&l, 4, 1, s_File);
    fwrite(s_Buf, 1, got, s_File);
}

unsigned U32(const void* b, int off) { return *(const unsigned*)((const char*)b + off); }
int I32(const void* b, int off) { return *(const int*)((const char*)b + off); }

// Copies the (ptr, count) array at b+off; returns the count (0 when absent).
int Arr(const char* key, const void* b, int off, int es, int cap)
{
    unsigned p = U32(b, off);
    int n = I32(b, off + 4);
    if (!p || n <= 0)
        return 0;
    int len = n * es < cap ? n * es : cap;
    Put(key, (const void*)(size_t)p, len);
    return n;
}

} // namespace

void M3DeepCapture(SGameLogic* gl, int frame)
{
    if (!s_Init) {
        s_Init = true;
        const char* e = getenv("PZ_M3_DEEPCAP");
        if (e && *e) {
            char tmp[512];
            strncpy(tmp, e, sizeof(tmp) - 1);
            tmp[sizeof(tmp) - 1] = 0;
            char* c1 = strchr(tmp, ',');
            if (c1) {
                *c1 = 0;
                s_F0 = atoi(c1 + 1);
                char* c2 = strchr(c1 + 1, ',');
                s_F1 = c2 ? atoi(c2 + 1) : s_F0;
            }
            s_File = fopen(tmp, "wb");
        }
    }
    if (!s_File || !g_World || frame < s_F0 || frame > s_F1)
        return;
    SWorld* w = g_World;
    SUnitHeap& h = w->Units;
    unsigned live = 0;
    for (int i = 0; i < h.Size; ++i)
        if (h.Array[i].Next == kHeapLive)
            ++live;
    unsigned hdr[4] = { 0x43445a50u, (unsigned)frame, w->RandomSeed, live };
    fwrite(hdr, 4, 4, s_File);
    char key[64];
    const unsigned char* wb = (const unsigned char*)w;
    const unsigned char* glb = (const unsigned char*)gl;
    Put("W", w, 0x7538);
    Put("GL", gl, 0x310);
    {
        unsigned p = U32(glb, 0x2dc);
        int n = I32(glb, 0x2e0);
        if (p && n > 0 && n < 4096) {
            Put("mg", (const void*)(size_t)p, n * 0x28);
            for (int k = 0; k < n; ++k) {
                const unsigned char* e = (const unsigned char*)(size_t)p + k * 0x28;
                if (U32(e, 0) == 0x7fffffff) {
                    sprintf(key, "mgm/%d", k);
                    Arr(key, e, 4, 0x10, 0x2000);
                }
            }
        }
        Arr("rt", glb, 0x268, 0x34, 0x2000);
        Arr("aig", wb, 0x4f4, 0x54, 0x10000);
        Arr("tvar", wb, 0x74a8, 0x14, 0x10000);
    }
    int bs = I32(wb, 0x74f4);
    if (bs > 0 && bs <= 0x1000000) {
        Put("bs", (const void*)(size_t)U32(wb, 0x74ec), bs * 4);
        Put("bd", (const void*)(size_t)U32(wb, 0x74f0), bs);
    }
    for (int i = 0; i < h.Size; ++i) {
        if (h.Array[i].Next != kHeapLive)
            continue;
        const unsigned char* ub = (const unsigned char*)(const void*)h.Array[i].Unit;
        sprintf(key, "u/%d", i);
        Put(key, ub, 0x480);
        static const int kArr[] = { 0x16c, 0x178, 0x19c, 0x1a8, 0x1b4, 0x1c0, 0x1fc, 0x308 };
        for (int off : kArr) {
            sprintf(key, "ua/%d/%d", i, off);
            Arr(key, ub, off, 0x40, 0x2000);
        }
        unsigned gp = U32(ub, 0x1d8);
        int gn = I32(ub, 0x1dc), gmax = I32(ub, 0x1e0);
        if (gp && gn > 0 && gmax > 0 && gmax <= 0x400) {
            sprintf(key, "gh/%d", i);
            Put(key, (const void*)(size_t)gp, gmax * 0x74);
        }
        static const int kTgt[] = { 0x1f4, 0x1f8 };
        for (int off : kTgt) {
            unsigned tp = U32(ub, off);
            if (tp) {
                sprintf(key, "t/%d/%d", i, off);
                Put(key, (const void*)(size_t)tp, 0x38);
            }
        }
        unsigned dp = U32(ub, 0x38);
        int dn = I32(ub, 0x3c);
        if (dp && dn > 0 && dn <= 16) {
            for (int k = 0; k < dn; ++k) {
                const unsigned char* db = (const unsigned char*)(size_t)((const unsigned*)(size_t)dp)[k];
                if (!db)
                    continue;
                sprintf(key, "d/%d/%d", i, k);
                Put(key, db, 0x100);
                sprintf(key, "dl/%d/%d", i, k);
                int ln = Arr(key, db, 0x88, 0x3c, 0x4000);
                const unsigned char* lb = (const unsigned char*)(size_t)U32(db, 0x88);
                for (int wi = 0; wi < ln && wi * 0x3c < 0x4000; ++wi) {
                    sprintf(key, "dlm/%d/%d/%d", i, k, wi);
                    int mn = Arr(key, lb, wi * 0x3c, 0x40, 0x4000);
                    const unsigned char* mb = (const unsigned char*)(size_t)U32(lb, wi * 0x3c);
                    for (int mi = 0; mi < mn && mi * 0x40 < 0x4000; ++mi) {
                        sprintf(key, "dlp/%d/%d/%d/%d", i, k, wi, mi);
                        Arr(key, mb, mi * 0x40 + 8, 0x14, 0x1000);
                    }
                }
                sprintf(key, "dg/%d/%d", i, k);
                Arr(key, db, 0x94, 8, 0x4000);
                static const int kDt[] = { 0xc0, 0xc4, 0xc8 };
                for (int off : kDt) {
                    unsigned tp = U32(db, off);
                    if (tp) {
                        sprintf(key, "dt/%d/%d/%d", i, k, off);
                        Put(key, (const void*)(size_t)tp, 0x38);
                    }
                }
            }
        }
        unsigned gup = U32(ub, 0x48);
        int gun = I32(ub, 0x4c);
        if (gup && gun > 0 && gun <= 16) {
            for (int k = 0; k < gun; ++k) {
                unsigned q = ((const unsigned*)(size_t)gup)[k];
                if (q) {
                    sprintf(key, "g/%d/%d", i, k);
                    Put(key, (const void*)(size_t)q, 0x80);
                }
            }
        }
    }
    unsigned short z = 0;
    fwrite(&z, 2, 1, s_File);
    fflush(s_File);
}

} // namespace pz
