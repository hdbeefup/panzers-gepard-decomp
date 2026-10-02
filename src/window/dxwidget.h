// window/dxwidget.h
// SDXWidget — DirectX widget base
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_DXWIDGET_H
#define WINDOW_DXWIDGET_H

#include "widget.h"

struct SDXWidget : SWidget {
    int BackFrame;
    int BackFont;
    int BackGlyph;
    unsigned int BackColor;
    bool BackNoresize;
    bool Back9Slice;
    int TooltipTimer;
    int TooltipEndTimer;
    int TooltipBackFrame;
    int InnerBackFrame;
    int TooltipFrame;
    int TooltipFont;
    int TooltipFrameLeft;
    int TooltipFrameMiddle;
    int TooltipFrameRight;
    char TooltipText[260];
    int ToolTipTextWidth;
    int ToolTipTextHeight;
    bool ToolTipFeatureEnabled;

    // Constructor / destructor
    SDXWidget();
    ~SDXWidget() override;

    // SWidget overrides
    void SetPosition(int x, int y, int w, int h) override;
    void Resize(int w, int h) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnTimer(int id, unsigned int elapsed) override;
    int GetFrame() override;
    void SetVisible(bool visible) override;
    void SetGravity(int gravity) override;
    void Update() override;

    // Non-virtual methods
    void Create(int a2);
    void HideToolTip();
    // Note: must undef Windows macro to avoid MessageBox → MessageBoxW/A substitution
#undef MessageBox
    int MessageBox(const char *text, int type);
    void SetBackgroundColor(unsigned int color);
    void SetBackgroundSprite(int font, int glyph, bool noresize, bool nine_slice);
    void SetToolTip(const char *text, int tooltipfont);
    void SetTooltipText(const char *tooltiptext);
    void SetDefaultBackgroundSprite();
    void ShowToolTip(int x, int y);
};

#endif // WINDOW_DXWIDGET_H
