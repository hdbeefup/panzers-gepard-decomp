// window/scaler.h
// SScaler header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SSCALER_H
#define WINDOW_SSCALER_H

#include "widget.h"

struct SScaler : SWidget {
    int BackFrame;
    float ScaleFactor;
    bool acceptEventsWhenHaveChildren;

    SScaler();
    ~SScaler() override;

    bool CanAcceptEvents() override;
    void ChildToParent(int *x, int *y) override;
    int GetFrame() override;
    bool IsScaler() override;
    void ParentToChild(int *x, int *y) override;
    void Resize(int width, int height) override;
    void SetGravity(int gravity) override;
    void SetPosition(int x, int y, int width, int height) override;
    void SetVisible(bool visible) override;

    void Create();
    void SetScaleFactor(float scaleFactor);
};

#endif // WINDOW_SSCALER_H
