// 3dengine/bitmap.h
// SBitmap — bitmap/texture data
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_BITMAP_H
#define DENGINE3_BITMAP_H

#include <d3d9.h>

struct SBitmap {
    int Width;
    int Height;
    D3DFORMAT Format;
    int Start;
    int Pitch;
    int Pixel;
    int Size;
    unsigned char *Data;

    SBitmap(int width, int height, D3DFORMAT format, int flags);
    ~SBitmap();
    void BitBlt(int destX, int destY, int width, int height, SBitmap *src, int srcX, int srcY);
};

#endif // DENGINE3_BITMAP_H
