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
    // Panzers: cursor set from one fixed-grid TGA (size x size cells, 256/size
    // per row) with a caller-supplied hotspot table. HD SBoard vtable slot
    // +0x94 (0x6c59e0). SWINE's LoadCursorSet(scale) loads its own atlas PNG
    // and .cur files instead.
    virtual void LoadCursorSetFile(const char* filename, int size, int count, const POINT* hotspots) = 0;
    // Panzers: virtual size of a scaler frame (HD SBoard vtable slot +0x58,
    // 0x6cb0c0). Children of the scaler are laid out in width x height
    // units and stretched to the scaler's frame size, separately in x and y.
    virtual void SetVirtualSize(int frame, int width, int height) = 0;
    // Panzers HD minimap frame (type 6) and its overlay. HD SBoard slots:
    //  +0x4c SetMinimapGlyph 0x6caca0: the frame's texture and glyph rect (size from the glyph);
    //  +0x54 SetMinimapTerrain 0x6c4f50: frame +0x54, the map image (else drawn black);
    //  +0xc0 SetMinimapCompass 0x6cac00: frame +0x58, the compass font drawn over the map;
    //  +0xa4 AddMinimapDot 0x6c2e70 / +0xa8 ClearMinimapDots 0x6c3a90: unit dots
    //        (minimap pixels from the centre, y up);
    //  +0xac SetMinimapViewCorner 0x6cabd0: the camera's view on the ground (4 corners);
    //  +0xb0 ClearMinimapBlinks 0x6c3a80 / +0xb4 AddMinimapBlink 0x6c2db0: "under attack"
    //        triangles, 2 s each;
    //  +0xb8 SetMinimapMarkCorner 0x6cae60 / +0xbc ClearMinimapMarkCorners 0x6c3aa0.
    // The rotation +0x50 (0x6caea0) is SetMinimapRotation.
    virtual void SetMinimapGlyph(int frame, int font, int glyph) = 0;
    virtual void SetMinimapTerrain(int frame, bool on) = 0;
    virtual void SetMinimapCompass(int frame, int font) = 0;
    virtual void AddMinimapDot(float x, float y, unsigned int color) = 0;
    virtual void ClearMinimapDots() = 0;
    virtual void SetMinimapViewCorner(int corner, float x, float y) = 0;
    virtual void ClearMinimapBlinks() = 0;
    virtual void AddMinimapBlink(float x, float y, unsigned int color) = 0;
    virtual void SetMinimapMarkCorner(int corner, float x, float y, unsigned int color) = 0;
    virtual void ClearMinimapMarkCorners() = 0;
    // Panzers: the cursor colour (HD board +0x9c 4th argument, the owning
    // widget's SWidget +0x40). HD tints only the D3D hardware cursor (+0xc4
    // 0x6ca4e0); the recompile's software cursor takes it (white = none).
    virtual void SetCursorColor(unsigned int color) = 0;
};

#endif // DENGINE3_IBOARD_H
