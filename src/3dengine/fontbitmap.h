// 3dengine/fontbitmap.h
// FontBitmap — grayscale bitmap for text effect processing
// Decompiled from: gameSplit/fontbitmap.c
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_FONTBITMAP_H
#define DENGINE3_FONTBITMAP_H

struct SBitmap;

struct FontBitmap {
    int X;
    int Y;
    int Width;
    int Height;
    unsigned char *Data;

    FontBitmap();
    FontBitmap(int width, int height);
    ~FontBitmap();

    void Blur(FontBitmap *result, float sigma, float dilute);
    void BoxBlur(FontBitmap *result, int khalf);
    void Add(const FontBitmap *source, int offset);

    static void CombineLightAndShadow(SBitmap *result, const FontBitmap *light,
                                       const FontBitmap *shadow, int shadowShift);
    static void CombineEmboss(SBitmap *result, const FontBitmap *normal,
                               const FontBitmap *blurry, float gradientScale);
};

#endif // DENGINE3_FONTBITMAP_H
