// 3dengine/panzers_hd_sizes.h
// x86 object sizes of renderer/window classes in the HD PANZERS.exe (2016),
// taken from operator new / scalar-deleting-destructor sizes, and the sizes
// the SWINE-derived structs have today. See docs/ENGINE_DIFF.md.
//
// The SWINE structs do NOT match the HD layout yet. Two kinds of check use
// these numbers:
//   - PANZERS_LAYOUT_SWINE_*: tripwire on the current (SWINE) size, so any
//     unintended layout change breaks the build. Update it together with
//     docs/ENGINE_DIFF.md when a struct is changed on purpose.
//   - PANZERS_LAYOUT_HD_*: the HD size. Checked only when building with
//     -DPANZERS_STRICT_LAYOUT=1 (fails today; it is the target).

#ifndef DENGINE3_PANZERS_HD_SIZES_H
#define DENGINE3_PANZERS_HD_SIZES_H

#ifndef PANZERS_STRICT_LAYOUT
#define PANZERS_STRICT_LAYOUT 0
#endif

// HD sizes and their evidence
#define PANZERS_LAYOUT_HD_SWIDGET    0x48   // SWidget dtor 0x543220: operator delete(this, 0x48)
#define PANZERS_LAYOUT_HD_SWINDOW    0x8C   // SWindow dtor 0x5445f0: operator delete(this, 0x8c)
#define PANZERS_LAYOUT_HD_SDXWINDOW  0xE4   // SDXWindow dtor 0x539d20: operator delete(this, 0xe4)
#define PANZERS_LAYOUT_HD_SDXWIDGET  0x58   // SDXWidget dtor 0x539990: operator delete(this, 0x58)
#define PANZERS_LAYOUT_HD_SGEPARD    0x810  // ::CreateGepard 0x678ba0: operator new(0x810)
#define PANZERS_LAYOUT_HD_SBOARD     0xE8   // SViewport::CreateWindowedViewport 0x68ada0: operator new(0xe8)
#define PANZERS_LAYOUT_HD_SFONTPROP  0x4860 // SHeap<SFontProp> stride 0x4864 (LoadFontFileFont 0x6c61a0)
#define PANZERS_LAYOUT_HD_SFRAME     0x58   // SHeap<SFrame> stride 0x5c (SBoard::CreateFrame 0x6c3ab0)

// Current SWINE-derived x86 sizes (measured, rendertest.log)
#define PANZERS_LAYOUT_SWINE_SWIDGET   0x44
#define PANZERS_LAYOUT_SWINE_SWINDOW   0xD0
#define PANZERS_LAYOUT_SWINE_SDXWINDOW 0x120
#define PANZERS_LAYOUT_SWINE_SGEPARD   0x1260
#define PANZERS_LAYOUT_SWINE_SBOARD    0x68
#define PANZERS_LAYOUT_SWINE_SFONTPROP 0x2430
#define PANZERS_LAYOUT_SWINE_SFRAME    0x50

#if defined(_M_IX86)
#define PANZERS_LAYOUT_CHECK(T, NAME)                                              \
    static_assert(sizeof(T) == PANZERS_LAYOUT_SWINE_##NAME,                          \
                  #T " size changed: update panzers_hd_sizes.h and ENGINE_DIFF.md"); \
    static_assert(!PANZERS_STRICT_LAYOUT || sizeof(T) == PANZERS_LAYOUT_HD_##NAME,  \
                  #T " does not match the HD PANZERS.exe size")
#else
#define PANZERS_LAYOUT_CHECK(T, NAME) static_assert(true, "")
#endif

#endif // DENGINE3_PANZERS_HD_SIZES_H
