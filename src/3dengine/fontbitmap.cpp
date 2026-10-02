// 3dengine/fontbitmap.cpp
// FontBitmap — grayscale bitmap for text effect processing
// Decompiled from: gameSplit/fontbitmap.c (addresses 0x41B700 - 0x41C700)
// Part of S.W.I.N.E. HD Remaster decompilation

#include "fontbitmap.h"
#include "texture.h"
#include <math.h>
#include <string.h>
#include <new>

// Static kernel buffers (original: global arrays)
static float kernel[256];
static int kernel8[256];

//----- (0041B700) --------------------------------------------------------

FontBitmap::FontBitmap()
{
    X = 0;
    Y = 0;
    Width = 0;
    Height = 0;
    Data = 0;
}

FontBitmap::FontBitmap(int width, int height)
{
    X = 0;
    Y = 0;
    Width = width;
    Height = height;
    Data = new unsigned char[width * height];
}

//----- (0041BA40) --------------------------------------------------------

FontBitmap::~FontBitmap()
{
    if (Data) {
        operator delete(Data);
        Data = 0;
    }
}

//----- (0041BCE0) --------------------------------------------------------

void FontBitmap::Add(const FontBitmap *source, int offset)
{
    // Original formula Data[offset + Width*(y+offset)] produces negative indices
    // when offset is negative (blur padding). Clip to bounds instead.
    for (int y = 0; y < source->Height; y++) {
        int dstY = y + offset;
        if (dstY < 0 || dstY >= Height) continue;
        for (int x = 0; x < source->Width; x++) {
            int dstX = x + offset;
            if (dstX < 0 || dstX >= Width) continue;
            unsigned char *dst = &Data[dstY * Width + dstX];
            unsigned char srcVal = source->Data[y * source->Width + x];
            float a = *dst / 255.0f;
            float b = srcVal / 255.0f;
            float v = sqrtf(a * a + b * b);
            if (v > 1.0f) v = 1.0f;
            *dst = (unsigned char)(v * 255.0f);
        }
    }
}

//----- (0041BE30) --------------------------------------------------------

void FontBitmap::Blur(FontBitmap *result, float sigma, float dilute)
{
    int kRadius = (int)ceilf(sigma * 3.0f);
    int kernelSize = 2 * (kRadius - 1) + 1;
    if (kernelSize > 256) kernelSize = 256;
    int startIdx = 1 - kRadius;

    // Build Gaussian kernel
    float sum = 0.0f;
    for (int i = 0; i < kernelSize; i++) {
        float x = (float)(startIdx + i);
        kernel[i] = expf(-x * x / (2.0f * sigma * sigma));
        sum += kernel[i];
    }

    // Normalize with dilute factor
    float factor = sqrtf(dilute) / sum;
    for (int i = 0; i < kernelSize; i++) {
        kernel[i] *= factor;
        kernel8[i] = (int)(kernel[i] * 256.0f + 0.5f);
    }

    // Horizontal pass
    int hOutW = Width + kernelSize - 1;
    int hOutH = Height;
    unsigned char *hBuf = (unsigned char *)operator new(hOutH * hOutW);

    for (int y = 0; y < hOutH; y++) {
        for (int x = 0; x < hOutW; x++) {
            int acc = 0;
            for (int k = 0; k < kernelSize; k++) {
                int srcX = x - k;
                if (srcX >= 0 && srcX < Width)
                    acc += kernel8[k] * Data[y * Width + srcX];
            }
            hBuf[y * hOutW + x] = (unsigned char)(acc >> 8);
        }
    }

    // Vertical pass
    int vOutW = hOutW;
    int vOutH = hOutH + kernelSize - 1;
    result->Width = vOutW;
    result->Height = vOutH;
    result->X = 0;
    result->Y = 0;
    result->Data = new unsigned char[vOutW * vOutH];

    for (int y = 0; y < vOutH; y++) {
        for (int x = 0; x < vOutW; x++) {
            int acc = 0;
            for (int k = 0; k < kernelSize; k++) {
                int srcY = y - k;
                if (srcY >= 0 && srcY < hOutH)
                    acc += (int)hBuf[srcY * hOutW + x] * kernel8[k];
            }
            result->Data[y * vOutW + x] = (unsigned char)(acc >> 8);
        }
    }

    operator delete(hBuf);
    result->X = startIdx;
    result->Y = startIdx;
}

//----- (0041C140) --------------------------------------------------------

void FontBitmap::BoxBlur(FontBitmap *result, int khalf)
{
    int kSize = 2 * khalf + 1;
    int reciprocal = (int)(65535.0f / (float)kSize);

    // Horizontal pass
    int hOutW = Width + 2 * khalf;
    int hOutH = Height;
    unsigned char *hBuf = (unsigned char *)operator new(hOutH * hOutW);

    for (int y = 0; y < hOutH; y++) {
        for (int x = 0; x < hOutW; x++) {
            int acc = 0;
            for (int k = 0; k < kSize; k++) {
                int srcX = x - k;
                if (srcX >= 0 && srcX < Width)
                    acc += Data[y * Width + srcX];
            }
            hBuf[y * hOutW + x] = (unsigned char)((unsigned int)(reciprocal * acc) >> 16);
        }
    }

    // Vertical pass
    int vOutW = hOutW;
    int vOutH = hOutH + kSize - 1;
    result->Width = vOutW;
    result->Height = vOutH;
    result->X = 0;
    result->Y = 0;
    result->Data = new unsigned char[vOutW * vOutH];

    for (int y = 0; y < vOutH; y++) {
        for (int x = 0; x < vOutW; x++) {
            int acc = 0;
            for (int k = 0; k < kSize; k++) {
                int srcY = y - k;
                if (srcY >= 0 && srcY < hOutH)
                    acc += (int)hBuf[srcY * hOutW + x];
            }
            result->Data[y * vOutW + x] = (unsigned char)((unsigned int)(reciprocal * acc) >> 16);
        }
    }

    operator delete(hBuf);
    result->X = -khalf;
    result->Y = -khalf;
}

//----- (0041C4F0) --------------------------------------------------------

void FontBitmap::CombineLightAndShadow(SBitmap *result, const FontBitmap *light,
                                        const FontBitmap *shadow, int shadowShift)
{
    int outW = shadowShift + shadow->Width;
    int outH = shadowShift + shadow->Height;
    new (result) SBitmap(outW, outH, D3DFMT_A8R8G8B8, 0);

    unsigned int *dst = (unsigned int *)result->Data;
    int lightRow = 0;
    int shadowY = -shadowShift;

    for (int row = 0; row < outH; row++, lightRow++, shadowY++) {
        int shadowX = -shadowShift;
        for (int col = 0; col < outW; col++, shadowX++) {
            int lightX = shadowX + shadowShift;
            unsigned int pixel;

            if (lightX >= light->Width || lightRow >= light->Height) {
                // Outside light region — shadow only
                if (shadowX < 0 || shadowY < 0)
                    pixel = 0;
                else
                    pixel = (unsigned int)shadow->Data[shadowY * shadow->Width + shadowX] << 24;
            } else {
                int lightAlpha = light->Data[lightRow * light->Width + lightX];
                if (shadowX < 0 || shadowY < 0) {
                    // Light only, no shadow
                    pixel = ((unsigned int)lightAlpha << 24) | 0x00FFFFFF;
                } else {
                    int shadowAlpha = shadow->Data[shadowY * shadow->Width + shadowX];
                    int combined = lightAlpha + shadowAlpha - lightAlpha * shadowAlpha / 255;
                    if (combined <= 0) {
                        pixel = 0;
                    } else {
                        int intensity = 255 * lightAlpha / combined;
                        if (intensity > 255) intensity = 255;
                        pixel = ((unsigned int)combined << 24) | (65793 * (unsigned int)intensity);
                    }
                }
            }
            *dst++ = pixel;
        }
    }
}

//----- (0041C320) --------------------------------------------------------

void FontBitmap::CombineEmboss(SBitmap *result, const FontBitmap *normal,
                                const FontBitmap *blurry, float gradientScale)
{
    int offset = (blurry->Width - normal->Width) / 2;
    new (result) SBitmap(blurry->Width, blurry->Height, D3DFMT_A8R8G8B8, 0);

    unsigned int *dst = (unsigned int *)result->Data;

    for (int y = 0; y < result->Height; y++) {
        int normalY = y - offset;
        int normalX = -offset;
        for (int x = 0; x < result->Width; x++, normalX++) {
            // Horizontal gradient (dx)
            int dx = 0;
            if (x + 1 < blurry->Width)
                dx = blurry->Data[y * blurry->Width + x + 1];
            if (x > 0)
                dx -= blurry->Data[y * blurry->Width + x - 1];

            // Vertical gradient (dy)
            int dy = 0;
            if (y + 1 < blurry->Height)
                dy = blurry->Data[(y + 1) * blurry->Width + x];
            if (y > 0)
                dy -= blurry->Data[(y - 1) * blurry->Width + x];

            // Combined gradient, clamped to [-255, 255]
            int gradient = (int)((float)(dx + dy) * gradientScale);
            if (gradient > 255) gradient = 255;
            if (gradient < -255) gradient = -255;

            // Get normal (text) alpha at offset position
            int normalAlpha = 0;
            if (normalX >= 0 && normalY >= 0 &&
                normalX < normal->Width && normalY < normal->Height) {
                normalAlpha = normal->Data[normalY * normal->Width + normalX];
                gradient = ((255 - normalAlpha) * gradient) >> 8;
            }

            // Decompose into highlight and shadow
            int highlight = 0;
            int shadow = 0;
            if (gradient > 0) {
                shadow = gradient;
            } else if (gradient < 0) {
                highlight = -gradient;
                shadow = -gradient;
            }

            // Blend with normal alpha
            if (normalAlpha) {
                int threequarter = (3 * normalAlpha) >> 2;
                highlight = ((255 - threequarter) * highlight) >> 8;
                shadow += threequarter - ((threequarter * shadow) >> 8);
            }

            unsigned int rgb = 65793 * (unsigned int)highlight;
            *dst++ = ((unsigned int)shadow << 24) | rgb;
        }
    }
}
