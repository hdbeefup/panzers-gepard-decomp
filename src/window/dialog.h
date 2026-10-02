// window/dialog.h
// SDialog header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SDIALOG_H
#define WINDOW_SDIALOG_H

#include "dxwidget.h"

struct SDialog : SDXWidget {
    int ModalResult;

    SDialog();
    ~SDialog() override = default;

    virtual void Cancel();
    void Create();
    int DoModal();
};

#endif // WINDOW_SDIALOG_H
