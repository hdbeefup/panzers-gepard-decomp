// 3dengine/board.h
// SBoard — 2D rendering board
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_BOARD_H
#define DENGINE3_BOARD_H

#include "igepard.h"  // SFrameType, HDMode, TypeFace, FontEffect, SDrawType
#include "iboard.h"
#include "chain.h"
#include "darray.h"

#include <d3d9.h>

struct SGepard;
struct SBitmap;
struct SAnimation;

// === SRectFloat ===
struct SRectFloat {
    float Left;
    float Top;
    float Right;
    float Bottom;
};

// === SGlyph ===
struct SGlyph {
    int Width;
    SRectFloat Src;
    SRectFloat Dest;
};

// === SFrame sub-structs ===
struct SFrameSprite {
    int Font;
    SGlyph Glyph;
    unsigned int SepiaColor;
};

struct SFrameText {
    int Font;
    unsigned int Color;
    int Align;
    char *Text;
    char *ReducedText;
    int Width;
    int TrueTypeX;
    int TrueTypeY;
    int TrueTypeWidth;
    int TrueTypeHeight;
    int TrueTypeTextureIndex;
};

struct SFrameBox {
    unsigned int Color;
};

struct SFrameAnim {
    SAnimation *Anim;
    int TextureIndex;
    unsigned int LastFrame;
    unsigned int EndTime;
    unsigned int FrameTime;
    int Flags;
    float MaxU;
    float MaxV;
};

struct SFrameMinimap {
    int Font;
    SGlyph Glyph;
    float Rotation;
    // Panzers: the HD frame fields +0x54 (terrain) / +0x58 (compass) live
    // in a side table in board.cpp (the SWINE SFrame size is asserted).
};

struct SFrameScaler {
    float ScaleFactor;
    // Panzers (HD SBoard +0x58, 0x6cb0c0): virtual size of the scaler. When
    // set (> 0), children are scaled by Width/VirtualWidth and
    // Height/VirtualHeight (non-uniform), as HD's board render 0x6c7150 does;
    // 0 keeps SWINE's uniform ScaleFactor.
    int VirtualWidth;
    int VirtualHeight;
};

// === SFrame ===
struct SFrame {
    float X;
    float Y;
    int Width;
    int Height;
    int Parent;
    int Child;
    int Sibling;
    int Flags;
    SFrameType Type;
    union {
        SFrameSprite Sprite;
        SFrameText Text;
        SFrameBox Box;
        SFrameAnim Anim;
        SFrameMinimap Minimap;
        SFrameScaler Scaler;
    };
};

// === SFontProp ===
struct SFontProp {
    int TextureIndex;
    int RefCount;
    int FirstGlyph;
    int LastGlyph;
    int TopMargin;
    int BottomMargin;
    int LeftMargin;
    int RightMargin;
    bool Italic;
    bool Bold;
    TypeFace typeFace;
    FontEffect fontEffect;
    int fontSize;
    SGlyph Glyphs[256];
};
#if defined(_M_IX86)
static_assert(sizeof(SGlyph) == 36, "SGlyph size must be 36 to match binary");
static_assert(sizeof(SFontProp) == 9264, "SFontProp size must be 9264 to match binary");
#endif

// === SBoardFrameData ===
struct SBoardFrameData {
    float xabs;
    float yabs;
    float scaleFactor;
};

// Unicode decode table (global, initialized in SBoard ctor)
extern int UnicodeDecodeTable[1120];

// === SBoard ===
struct SBoard : SIBoard {
    IDirect3DDevice9* lpD3DDev;
    SGepard* Gepard;
    SHeap<SFrame> Frames;
    bool roundPixels;
    SHeap<SFontProp> Fonts;
    POINT* CursorHotspots;
    HICON* CursorIcons;
    int CursorFont;
    int CursorGlyph;
    int CursorX;
    int CursorY;
    int NumCursors;
    int CursorSize;
    bool CursorVisible;
    bool HardwareCursor;
    bool ForceSoftwareCursor;
    SDArray<SBoardFrameData> frameStack;

    typedef SBoardFrameData FrameData;

    // Constructor / Destructor
    SBoard(SGepard *gepard);
    ~SBoard();

    // Methods
    int AddRefFont(int idx);
    void ApplyHardwareCursor();
    int CreateFrame(SFrameType type, int parent, int x, int y, int gravity, bool ontop);
    void DestroyFrame(int idx);
    int GetFontTexture(int idx);
    void GetFrameSize(int idx, int *width, int *height);
    HFONT GetHFont(SFontProp *font, bool *doubleMode, float scaleFactor);
    float GetMinimapRotation(int frameIdx);
    float GetTextEffectiveScaleFactor(int idx);
    void GetTextExtent(int font, const char *str, int nchars, int *width, int *height, float scaleFactor);
    void GravitateFrame(int idx, int gravity);
    void InitHotspots();
    bool IsAnimPlaying(int idx);
    void LoadCursorSet(float windowScale);
    void LoadCursorSetFile(const char *filename, int size, int count, const POINT *hotspots);
    int LoadCustomFont(const char *filename, int numglyphs, SCustomGlyph *glyphs, HDMode hdmode);
    int LoadFixedFont(const char *filename, int width, int height, int row, int numglyphs, unsigned char *glyphs, HDMode hdmode);
    int LoadProportionalFont(const char *filename, int lineheight, int numglyphs, unsigned char *glyphs, int top_margin, int bottom_margin, int left_margin, int right_margin);
    int LoadSingleFont(const char *filename, HDMode hdmode);
    int LoadFontFileFont(const char *filename);
    int LoadTrueTypeFont(int fontoverride, TypeFace typeface, FontEffect fontEffect, int fontsize, bool italic, bool bold);
    void MoveFrame(int idx, int x, int y);
    void MoveFrameFloat(int idx, float x, float y);
    void ReleaseFont(int idx);
    void Render(float a2, int a3, int a4);
    void RerenderText(int idx);
    void ResizeFrame(int idx, int width, int height);
    void SetBoxColor(int idx, unsigned int color);
    void SetCursor(int idx, int x, int y);
    void SetMinimapFont(int frameIdx, int font);
    void SetMinimapRotation(int frameIdx, float rotation);
    void SetPixelRounding(bool round);
    void SetScaleFactor(int idx, float scaleFactor);
    void SetVirtualSize(int idx, int width, int height);
    void SetSpriteGlyph(int idx, int font, int glyph);
    void SetSpriteSepiaFilter(int idx, bool enable, unsigned int color);
    void SetText(int idx, int font, int align, const char *text);
    void SetTextColor(int idx, unsigned int color);
    void SetTextF(int idx, int font, int align, const char *format, ...);
    void SetTextV(int idx, int font, int align, const char *format, char *args);
    void ShowFrame(int idx, bool visible);
    void StartAnim(int idx, const char *filename, int flags, int duration, int frameRate);
    void StopAnim(int idx);
    void UnloadCursorSet();

    // Panzers HD minimap (SBoard +0x4c, +0x54, +0xa4..+0xc0; see iboard.h).
    void SetMinimapGlyph(int frame, int font, int glyph);
    void SetMinimapTerrain(int frame, bool on);
    void SetMinimapCompass(int frame, int font);
    void AddMinimapDot(float x, float y, unsigned int color);
    void ClearMinimapDots();
    void SetMinimapViewCorner(int corner, float x, float y);
    void ClearMinimapBlinks();
    void AddMinimapBlink(float x, float y, unsigned int color);
    void SetMinimapMarkCorner(int corner, float x, float y, unsigned int color);
    void ClearMinimapMarkCorners();
    void SetCursorColor(unsigned int color);
    void RenderHdMinimap(SFrame& f, int frame, float fx, float fy, float scaleX, float scaleY, int& lastTextureIdx);
    // (The HD board minimap state, +0x5c..+0xd4, is a static in board.cpp:
    // the SWINE SBoard size is asserted.)
};

#endif // DENGINE3_BOARD_H
