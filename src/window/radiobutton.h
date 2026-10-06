// window/radiobutton.h
// SRadioButton header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SRADIOBUTTON_H
#define WINDOW_SRADIOBUTTON_H

#include "dxwidget.h"
#include "string2.h"

struct SRadioButton : SDXWidget {
    int Font;
    int RadioFont;
    int RadioHLFont;
    int RadioDisabledFont;
    int TextFrame;
    int RadioFrame;
    SString Text;
    unsigned int NormalColor;
    unsigned int ActiveColor;
    unsigned int DisabledColor;
    bool Active;
    bool Pressed;
    bool bChecked;
    int HdControlsFont;   // >= 0: HD skin (CreateHD 0x53eaf0 / UpdateHD 0x53ed30)

    SRadioButton();
    ~SRadioButton() override;

    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void SetVisible(bool visible) override;
    void Update() override;

    void Create(int a2, int font, unsigned int normal_color, unsigned int active_color, unsigned int disabled_color);
    bool GetCheck();
    char *GetText();
    void SetActive(bool active);
    void SetCheck(bool checked);
    void SetText(const char *text);
    void CreateHD(int a2, int controlsFont);   // 0x53eaf0
    void UpdateHD();                            // 0x53ed30
};

#endif // WINDOW_SRADIOBUTTON_H
