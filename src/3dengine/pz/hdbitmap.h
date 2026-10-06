// src/3dengine/pz/hdbitmap.h
// The HD SBitmap layout (no other dependencies, so game / UI code can use it).

#ifndef PZ_HDBITMAP_H
#define PZ_HDBITMAP_H

namespace pz {

// HD SBitmap (0x20 bytes; ctor 0x669ca0, load 0x66ea40, copy 0x669be0,
// pixel format 0x66e990). Format 1: 3 bytes a pixel (B, G, R); 2 / 3: 4
// bytes (B, G, R, X / A); Pitch is Width * Bpp for the uncompressed ones.
struct SHdBitmap {
    int            Width;    // +0x00
    int            Height;   // +0x04
    int            Format;   // +0x08
    int            Start;    // +0x0c byte offset of the first pixel in Data
    int            Pitch;    // +0x10
    int            Size;     // +0x14
    unsigned char  Bpp;      // +0x18 (0x66e990)
    unsigned char  Compressed; // +0x19 (4 x 4 blocks)
    unsigned char  _1a[2];
    unsigned char* Data;     // +0x1c
};
static_assert(sizeof(void*) != 4 || sizeof(SHdBitmap) == 0x20, "HD SBitmap 0x20");
int HdBitmapBpp(int format);                                  // 0x66e990 (low byte; 0 = unsupported here)

} // namespace pz

#endif // PZ_HDBITMAP_H
