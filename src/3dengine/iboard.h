// 3dengine/iboard.h
// SIBoard — 2D board interface
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_IBOARD_H
#define DENGINE3_IBOARD_H

#include "igepard.h"  // for SFrameType, HDMode, TypeFace, FontEffect, SCustomGlyph

// SIBoard — abstract base for 2D rendering board
// Virtual methods converted from SIBoard_vtbl in PDB
struct SIBoard {
    virtual int CreateFrame(SFrameType type, int parent, int x, int y, int gravity, bool ontop) = 0;
    virtual void DestroyFrame(int frame) = 0;
    virtual void MoveFrame(int frame, int x, int y) = 0;
    virtual void MoveFrameFloat(int frame, float x, float y) = 0;
    virtual void ResizeFrame(int frame, int w, int h) = 0;
    virtual void ShowFrame(int frame, bool visible) = 0;
    virtual void GravitateFrame(int frame, int gravity) = 0;
    virtual void GetFrameSize(int frame, int* w, int* h) = 0;
    virtual void SetSpriteGlyph(int frame, int font, int glyph) = 0;
    virtual void SetSpriteSepiaFilter(int frame, bool sepia, unsigned int color) = 0;
    virtual void SetTextColor(int frame, unsigned int color) = 0;
    virtual void SetText(int frame, int font, int size, const char* text) = 0;
    virtual void SetTextF(int frame, int font, int size, const char* fmt, ...) = 0;
    virtual void SetTextV(int frame, int font, int size, const char* fmt, char* args) = 0;
    virtual void SetBoxColor(int frame, unsigned int color) = 0;
    virtual void StartAnim(int frame, const char* name, int a, int b, int c) = 0;
    virtual void StopAnim(int frame) = 0;
    virtual bool IsAnimPlaying(int frame) = 0;
    virtual void SetMinimapFont(int frame, int font) = 0;
    virtual void SetMinimapRotation(int frame, float rotation) = 0;
    virtual float GetMinimapRotation(int frame) = 0;
    virtual void SetScaleFactor(int frame, float scale) = 0;
    virtual void SetPixelRounding(bool round) = 0;
    virtual int LoadProportionalFont(const char* name, int size, int flags, unsigned char* charset, int a, int b, int c, int d) = 0;
    virtual int LoadTrueTypeFont(int size, TypeFace face, FontEffect effect, int weight, bool italic, bool underline) = 0;
    virtual int LoadFixedFont(const char* name, int w, int h, int cols, int rows, unsigned char* charset, HDMode mode) = 0;
    virtual int LoadCustomFont(const char* name, int count, SCustomGlyph* glyphs, HDMode mode) = 0;
    virtual int LoadSingleFont(const char* name, HDMode mode) = 0;
    // Panzers: ".font" chunk files (menu/fonts/*.font + matching .tga).
    // HD SBoard vtable slot +0x6C (0x6c61a0). Not in SWINE.
    virtual int LoadFontFileFont(const char* filename) = 0;
    virtual void ReleaseFont(int font) = 0;
    virtual int GetFontTexture(int font) = 0;
    virtual float GetTextEffectiveScaleFactor(int font) = 0;
    virtual void GetTextExtent(int font, const char* text, int size, int* w, int* h, float scale) = 0;
    virtual void LoadCursorSet(float scale) = 0;
    virtual void UnloadCursorSet() = 0;
    virtual void SetCursor(int cursor, int x, int y) = 0;
    virtual void ApplyHardwareCursor() = 0;
};

#endif // DENGINE3_IBOARD_H
