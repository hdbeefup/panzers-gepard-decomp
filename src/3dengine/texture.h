// 3dengine/texture.h
// SBitmap, SShadowMask, SSurfaceBitmap, STextureBitmap
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_TEXTURE_H
#define DENGINE3_TEXTURE_H

#include <d3d9.h>
#include "hdbeefup.h"

// Forward declarations for types only used as pointers
struct SStream;

// === SBitmap ===
// Base bitmap class — holds pixel data in a given D3DFORMAT.

struct SBitmap {
    int Width;
    int Height;
    D3DFORMAT Format;
    int Start;
    int Pitch;
    int Pixel;
    int Size;
    unsigned char* Data;

    // Constructors
    SBitmap();
    SBitmap(SBitmap* src);                                      // move-style copy (nulls source Data)
    SBitmap(SBitmap* source, D3DFORMAT format);                 // convert copy
    SBitmap(int width, int height, D3DFORMAT format, int upsidedown);

    // Destructor
    ~SBitmap();

    // Assignment (move-style, nulls source Data)
    SBitmap& operator=(SBitmap* src);

    // Data management
    void AllocateData(int upsidedown);
    void InitPixelFormat();
    int  GetLogicalSize();

    // Blitting / pixel ops
    void BitBlt(int x, int y, int width, int height, SBitmap* source, int src_x, int src_y);
    void MakeOpaque();
    void MakeInverseOpaque();
    void NextMipLevel();
    void Rotate(int angle);

    // File I/O
    char LoadPNG(const char* filename, const char* panicstr);
    char LoadTGA(char* filename, char* panicstr);
    char SaveTGA(char* filename, char* panicstr);

#ifdef HDB_MISSING_ASSET_FALLBACK
    // Synthesize a magenta-and-black checker bitmap to substitute for
    // missing .png/.tga files. Width and height must be powers of two;
    // the cell size scales with min(w,h)/4.
    void FillMissingPlaceholder(int width, int height);
#endif

    // Windows interop
    HBITMAP CreateWinBitmap();
};

// === SShadowMask ===
// 1-bit-per-pixel shadow mask (packed 8 pixels per byte).

struct SShadowMask {
    int Width;
    int Height;
    int Pitch;
    int Size;
    unsigned char* Data;

    // Constructors
    SShadowMask(int width, int height, SBitmap* source);
    SShadowMask(SStream* is);

    // Destructor
    ~SShadowMask();

    // Methods
    void MakeBitmap(SBitmap* destination);
    void Save(SStream* is);
};

// === STextureBitmap ===
// SBitmap backed by a locked IDirect3DTexture9 mip level.

struct STextureBitmap : SBitmap {
    IDirect3DTexture9* lpTexture;
    int Level;

    STextureBitmap() : lpTexture(0), Level(0) {}
    STextureBitmap(IDirect3DTexture9* lpTexture, int level);
    ~STextureBitmap();
};

// === SSurfaceBitmap ===
// SBitmap backed by a locked IDirect3DSurface9.

struct SSurfaceBitmap : SBitmap {
    IDirect3DSurface9* lpSurface;

    SSurfaceBitmap() : lpSurface(0) {}
    SSurfaceBitmap(IDirect3DSurface9* lpSurface);
    ~SSurfaceBitmap();
};

#endif // DENGINE3_TEXTURE_H
