// window/editboxwithtext.h
// SEditBoxWithText header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SEDITBOXWITHTEXT_H
#define WINDOW_SEDITBOXWITHTEXT_H

#include "dxwidget.h"
#include "editbox.h"

struct SEditBoxWithText : SDXWidget {
    int Font;
    int Prompt;
    SEditBox EditBox;

    SEditBoxWithText();
    ~SEditBoxWithText() override;

    void Create(int a2, int promptfont, int editfont, const char *textprompt);
    void SetFocusToInnerEditBox();
    void SetBackgroundColor(unsigned int color);
    char *GetText();
    void SetText(const char *text);
    void SetTooltipText(const char *tooltiptext);
};

#endif // WINDOW_SEDITBOXWITHTEXT_H
