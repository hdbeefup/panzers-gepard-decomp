// 3dengine/animation.h
// SAnimation — frame animation loaded from .ANI files
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_ANIMATION_H
#define DENGINE3_ANIMATION_H

#include "texture.h"

// SAnimation inherits from SBitmap and adds frame animation state.
struct SAnimation : SBitmap {
    unsigned char *Buffer;
    int BufferLen;
    int BufferPtr;
    int CurrentFrame;
    int Frames;
    bool Anim2Format;

    SAnimation();
    ~SAnimation();
    void LoadANI(int mode, const char *filename, const char *panicstr);
    bool NextFrame(bool looping);
};

#endif // DENGINE3_ANIMATION_H
