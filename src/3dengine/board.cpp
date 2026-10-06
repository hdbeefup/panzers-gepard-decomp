// 3dengine/board.cpp
// 2D rendering board
// Decompiled from: gameSplit/sboard.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <intrin.h>
#include <new>

#include "board.h"
#include "fontbitmap.h"
#include "gepard.h"
#include "texture.h"
#include "logger.h"
#include "stream.h"
#include "string2.h"
#include "core_common.h"
#include "hdbeefup.h"

// Unicode decode table (global)
int UnicodeDecodeTable[1120];

// Float constants from IDA
#define FLOAT_0_55555558 0.55555558f

// LT_PRELIT lighting type (from SLightingType enum: 0=NORMAL, 1=AMBIENT, 2=PRELIT)
#define LT_PRELIT 2

#include "animation.h"

// Classes: SBoard
// Function count: 44

// TLVertex struct for 2D rendering (FVF 0x144 = XYZRHW|DIFFUSE|TEX1, 28 bytes)
struct TLVertBoard { float x, y, z, rhw; unsigned int color; float u, v; };

// Static vertex buffer used by SBoard::Render
// IDA decompiled these as individual dword_ globals at 0x58F3B0..0x58F580
static TLVertBoard vert[4]; // primary quad (triangle strip)

// The render buffer is a large array of vertices for batched rendering
// dword_58F3B0 is vert[0].x, stride 28 bytes
// Additional vertices extend beyond the initial 4
#define RENDER_VB_SIZE 128
static TLVertBoard renderVB[RENDER_VB_SIZE];
static int *dword_5976FC = (int *)&renderVB[RENDER_VB_SIZE]; // end sentinel
static bool init = true;

// Map dword_ names to renderVB fields — vert[0] starts at 0x58F3B0
// vert[0]: B0=x, B4=y, B8=z, BC=rhw, C0=color, C4=u, C8=v
// vert[1]: CC=x, D0=y, D4=z, D8=rhw, DC=color, E0=u, E4=v
// etc.
#define dword_58F3B4 (*(int*)&vert[0].y)
#define dword_58F3B8 (*(int*)&vert[0].z)
#define dword_58F3BC (*(int*)&vert[0].rhw)
#define dword_58F3C0 (*(int*)&vert[0].color)
#define dword_58F3C4 (*(int*)&vert[0].u)
#define dword_58F3C8 (*(int*)&vert[0].v)
#define dword_58F3CC (*(int*)&vert[1].x)
#define dword_58F3D0 (*(int*)&vert[1].y)
#define dword_58F3D4 (*(int*)&vert[1].z)
#define dword_58F3D8 (*(int*)&vert[1].rhw)
#define dword_58F3DC (*(int*)&vert[1].color)
#define dword_58F3E0 (*(int*)&vert[1].u)
#define dword_58F3E4 (*(int*)&vert[1].v)
#define dword_58F3E8 (*(int*)&vert[2].x)
#define dword_58F3EC (*(int*)&vert[2].y)
#define dword_58F3F0 (*(int*)&vert[2].z)
#define dword_58F3F4 (*(int*)&vert[2].rhw)
#define dword_58F3F8 (*(int*)&vert[2].color)
#define dword_58F3FC (*(int*)&vert[2].u)
#define dword_58F400 (*(int*)&vert[2].v)
#define dword_58F404 (*(int*)&vert[3].x)
#define dword_58F408 (*(int*)&vert[3].y)
#define dword_58F40C (*(int*)&vert[3].z)
#define dword_58F410 (*(int*)&vert[3].rhw)
#define dword_58F414 (*(int*)&vert[3].color)
#define dword_58F418 (*(int*)&vert[3].u)
#define dword_58F41C (*(int*)&vert[3].v)

// Additional vertex dwords mapped to renderVB (indices > 3)
// Offset from base: (addr - 0x58F3B0) / 28 = vertex index, (addr - 0x58F3B0) % 28 / 4 = field
// For simplicity, map remaining dwords to raw int array
static int renderVBraw[(0x58F580 - 0x58F420 + 4) / 4];
#define RVBOFF(addr) (((addr) - 0x58F420) / 4)
#define dword_58F420 renderVBraw[RVBOFF(0x58F420)]
#define dword_58F424 renderVBraw[RVBOFF(0x58F424)]
#define dword_58F434 renderVBraw[RVBOFF(0x58F434)]
#define dword_58F438 renderVBraw[RVBOFF(0x58F438)]
#define dword_58F43C renderVBraw[RVBOFF(0x58F43C)]
#define dword_58F440 renderVBraw[RVBOFF(0x58F440)]
#define dword_58F450 renderVBraw[RVBOFF(0x58F450)]
#define dword_58F454 renderVBraw[RVBOFF(0x58F454)]
#define dword_58F458 renderVBraw[RVBOFF(0x58F458)]
#define dword_58F45C renderVBraw[RVBOFF(0x58F45C)]
#define dword_58F46C renderVBraw[RVBOFF(0x58F46C)]
#define dword_58F470 renderVBraw[RVBOFF(0x58F470)]
#define dword_58F474 renderVBraw[RVBOFF(0x58F474)]
#define dword_58F478 renderVBraw[RVBOFF(0x58F478)]
#define dword_58F488 renderVBraw[RVBOFF(0x58F488)]
#define dword_58F48C renderVBraw[RVBOFF(0x58F48C)]
#define dword_58F490 renderVBraw[RVBOFF(0x58F490)]
#define dword_58F494 renderVBraw[RVBOFF(0x58F494)]
#define dword_58F4A4 renderVBraw[RVBOFF(0x58F4A4)]
#define dword_58F4A8 renderVBraw[RVBOFF(0x58F4A8)]
#define dword_58F4AC renderVBraw[RVBOFF(0x58F4AC)]
#define dword_58F4B0 renderVBraw[RVBOFF(0x58F4B0)]
#define dword_58F4C0 renderVBraw[RVBOFF(0x58F4C0)]
#define dword_58F4C4 renderVBraw[RVBOFF(0x58F4C4)]
#define dword_58F4C8 renderVBraw[RVBOFF(0x58F4C8)]
#define dword_58F4CC renderVBraw[RVBOFF(0x58F4CC)]
#define dword_58F4DC renderVBraw[RVBOFF(0x58F4DC)]
#define dword_58F4E0 renderVBraw[RVBOFF(0x58F4E0)]
#define dword_58F4E4 renderVBraw[RVBOFF(0x58F4E4)]
#define dword_58F4E8 renderVBraw[RVBOFF(0x58F4E8)]
#define dword_58F4F8 renderVBraw[RVBOFF(0x58F4F8)]
#define dword_58F4FC renderVBraw[RVBOFF(0x58F4FC)]
#define dword_58F500 renderVBraw[RVBOFF(0x58F500)]
#define dword_58F504 renderVBraw[RVBOFF(0x58F504)]
#define dword_58F514 renderVBraw[RVBOFF(0x58F514)]
#define dword_58F518 renderVBraw[RVBOFF(0x58F518)]
#define dword_58F51C renderVBraw[RVBOFF(0x58F51C)]
#define dword_58F520 renderVBraw[RVBOFF(0x58F520)]
#define dword_58F530 renderVBraw[RVBOFF(0x58F530)]
#define dword_58F534 renderVBraw[RVBOFF(0x58F534)]
#define dword_58F538 renderVBraw[RVBOFF(0x58F538)]
#define dword_58F53C renderVBraw[RVBOFF(0x58F53C)]
#define dword_58F54C renderVBraw[RVBOFF(0x58F54C)]
#define dword_58F550 renderVBraw[RVBOFF(0x58F550)]
#define dword_58F554 renderVBraw[RVBOFF(0x58F554)]
#define dword_58F558 renderVBraw[RVBOFF(0x58F558)]
#define dword_58F568 renderVBraw[RVBOFF(0x58F568)]
#define dword_58F56C renderVBraw[RVBOFF(0x58F56C)]
#define dword_58F580 renderVBraw[RVBOFF(0x58F580)]

//----- (0041B740) --------------------------------------------------------

SBoard::SBoard(SGepard *gepard)

{
  int v3;
  int v4;
  const char *FileInSearchPath; // eax
  const char *v6; // eax
  const char *v7; // eax
  this->Frames.array = 0;
  this->Frames.size = 0;
  this->Frames.maxsize = 0;
  this->Frames.nextempty = -1;
  this->Frames.occupied = 0;
  this->Fonts.array = 0;
  this->Fonts.size = 0;
  this->Fonts.maxsize = 0;
  this->Fonts.nextempty = -1;
  this->Fonts.occupied = 0;
  this->frameStack.array = 0;
  this->frameStack.size = 0;
  this->frameStack.maxsize = 0;
  this->Gepard = gepard;
  this->lpD3DDev = gepard->lpD3DDev;
  this->CursorHotspots = 0;
  this->CursorFont = -1;
  this->CursorIcons = 0;
  this->Frames.Add();
  this->Frames.array->data.X = 0.0;
  this->Frames.array->data.Y = 0.0;
  this->Frames.array->data.Width = 800;
  this->Frames.array->data.Height = 600;
  this->Frames.array->data.Parent = -1;
  this->Frames.array->data.Child = -1;
  this->Frames.array->data.Sibling = -1;
  this->Frames.array->data.Flags = 16;
  this->Frames.array->data.Type = FT_EMPTY;
  v3 = 0;
  this->roundPixels = 1;
  do
  {
    UnicodeDecodeTable[v3] = v3;
    ++v3;
  }
  while ( v3 < 256 );
  memset(&UnicodeDecodeTable[256], 0x3Fu, 0x360u);
  *(_DWORD *)&UnicodeDecodeTable[258] = -1314790461;
  v4 = 1025;
  *(_WORD *)&UnicodeDecodeTable[728] = -94;
  *(_DWORD *)&UnicodeDecodeTable[321] = -237915229;
  *(_WORD *)&UnicodeDecodeTable[317] = -19035;
  *(_DWORD *)&UnicodeDecodeTable[344] = -1230571304;
  *(_DWORD *)&UnicodeDecodeTable[350] = -1180058966;
  *(_DWORD *)&UnicodeDecodeTable[354] = -1146356002;
  *(_DWORD *)&UnicodeDecodeTable[377] = -1079001940;
  *(_WORD *)&UnicodeDecodeTable[381] = -16722;
  UnicodeDecodeTable[731] = -78;
  UnicodeDecodeTable[711] = -73;
  UnicodeDecodeTable[733] = -67;
  *(_WORD *)&UnicodeDecodeTable[340] = -8000;
  *(_WORD *)&UnicodeDecodeTable[313] = -6715;
  *(_WORD *)&UnicodeDecodeTable[262] = -6458;
  *(_DWORD *)&UnicodeDecodeTable[268] = -271587128;
  *(_DWORD *)&UnicodeDecodeTable[280] = -322114870;
  *(_WORD *)&UnicodeDecodeTable[272] = -3888;
  *(_WORD *)&UnicodeDecodeTable[327] = -3374;
  *(_WORD *)&UnicodeDecodeTable[336] = -2603;
  *(_DWORD *)&UnicodeDecodeTable[366] = -69469735;
  do
  {
    UnicodeDecodeTable[v4] = v4 - 80;
    ++v4;
  }
  while ( v4 < 1120 );
  // PANZERS 0x6c2500 (SBoard::SBoard, partial): the HD constructor registers
  // no TrueType files. SWINE HD panicked unless fonts/NotoSans-Condensed.ttf,
  // SWINPH.ttf and SWINRH.ttf (HD-remaster files) could be added; Panzers
  // ships none of them, so they are now registered only if present.
  // (HD instead creates a 32x32 A8R8G8B8 offscreen surface for the hardware
  // cursor and calls SetHardwareMouseCursor; not ported, see ENGINE_DIFF.md.)
  (void)FileInSearchPath; (void)v6; (void)v7;
  static const char *const kHdFonts[] = {
    "fonts/NotoSans-Condensed.ttf", "fonts/SWINPH.ttf", "fonts/SWINRH.ttf" };
  for ( int i = 0; i < 3; ++i )
  {
    struct _stat st;
    char *path = FileSystem.FindFileInSearchPath(kHdFonts[i]);
    if ( _stat(path, &st) == 0 && AddFontResourceExA(path, 0x10u, 0) )
      Logger.g->Log(0, "SBoard::SBoard: registered %s", path);
  }
}

//----- (0041BA60) --------------------------------------------------------

SBoard::~SBoard()

{
  bool v2; // cc
  int Occupied;
  int v4;
  v2 = this->Frames.occupied <= 1;
  if ( !v2 )
  {
    Occupied = this->Frames.GetOccupied();
#ifdef HD_DEBUG_BOARD
    Logger.g->Log(0, "SBoard::~SBoard: %d frames leaked on shutdown.", Occupied - 1);
#endif
  }
  if ( this->Fonts.occupied > 0 )
  {
    v4 = this->Fonts.GetOccupied();
#ifdef HD_DEBUG_BOARD
    Logger.g->Log(0, "SBoard::~SBoard: %d fonts leaked on shutdown.", v4);
#endif
  }
  if ( this->frameStack.array )
  {
    free(this->frameStack.array);
    this->frameStack.array = 0;
  }
  if ( this->Fonts.array )
    free(this->Fonts.array);
  if ( this->Frames.array )
    free(this->Frames.array);
}

//----- (0041BDC0) --------------------------------------------------------

int SBoard::AddRefFont(int idx)

{
  int result;
  SHeap<SFontProp>::__Tstruct *array; // ecx
  result = idx;
  if ( idx >= 0 && idx < this->Fonts.size )
  {
    array = this->Fonts.array;
    if ( array[idx].use == 0x7FFFFFFF )
    {
      ++array[idx].data.RefCount;
#ifdef HD_DEBUG_FONTS
      if ( idx >= 16 )
        Logger.g->Log(0, "AddRefFont(%d): RefCount now=%d", idx, array[idx].data.RefCount);
#endif
    }
  }
  return result;
}

//----- (0041BDF0) --------------------------------------------------------

void SBoard::ApplyHardwareCursor()

{
  int CursorGlyph;
  if ( !this->HardwareCursor || this->ForceSoftwareCursor || (CursorGlyph = this->CursorGlyph, CursorGlyph < 0)
       || !this->CursorIcons )   // Panzers cursor sets (LoadCursorSetFile) have no HICONs
    ::SetCursor(0);
  else
    ::SetCursor(this->CursorIcons[CursorGlyph]);
}

//----- (0041C640) --------------------------------------------------------

int SBoard::CreateFrame(SFrameType type, int parent, int x, int y, int gravity, bool ontop)

{
  SHeap<SFrame> *p_Frames; // esi
  int v9;
  int result;
  int v11;
  SHeap<SFrame>::__Tstruct *array; // ecx
  SHeap<SFrame>::__Tstruct *v13; // edi
  int Child;
  int i;
  int parenta;
  p_Frames = &this->Frames;
  if ( parent < 0 || parent >= this->Frames.size || (v9 = parent, p_Frames->array[parent].use != 0x7FFFFFFF) )
  {
#ifdef HD_DEBUG_BOARD
    Logger.g->Log(0, "SBoard::CreateFrame PANIC: type=%d parent=%d x=%d y=%d gravity=%d ontop=%d Frames.size=%d", (int)type, parent, x, y, gravity, (int)ontop, this->Frames.size);
#endif
    Logger.g->Panic("SBoard::CreateFrame: Parent id (%d) is not valid.", parent);
  }
  result = this->Frames.Add();
  v11 = result;
  parenta = result;
  p_Frames->array[v11].data.X = (float)x;
  p_Frames->array[v11].data.Y = (float)y;
  p_Frames->array[v11].data.Width = 0;
  p_Frames->array[v11].data.Height = 0;
  p_Frames->array[v11].data.Parent = parent;
  p_Frames->array[v11].data.Child = -1;
  p_Frames->array[v11].data.Flags = gravity | 0x10;
  p_Frames->array[v11].data.Type = type;
  array = p_Frames->array;
  switch ( p_Frames->array[result].data.Type )
  {
    case FT_SPRITE:
    case FT_MINIMAP:
    case FT_SPRITE_9SLICE:
      array[v11].data.Sprite.Font = -1;
      p_Frames->array[v11].data.Sprite.SepiaColor = 0;
      break;
    case FT_TEXT:
    case FT_FIXTEXT:
      array[v11].data.Sprite.Font = -1;
      p_Frames->array[v11].data.Sprite.Glyph.Width = -1;
      p_Frames->array[v11].data.Text.Align = 0;
      p_Frames->array[v11].data.Anim.EndTime = 0;
      p_Frames->array[v11].data.Anim.FrameTime = 0;
      p_Frames->array[v11].data.Text.Width = 0;
      p_Frames->array[v11].data.Text.TrueTypeWidth = 0;
      p_Frames->array[v11].data.Text.TrueTypeHeight = 0;
      p_Frames->array[v11].data.Sprite.SepiaColor = -1;
      // On x86 SFrameText.TrueTypeTextureIndex aliases Sprite.SepiaColor
      // (both at union offset 40) so the SepiaColor=-1 above doubles as
      // the "no texture" sentinel. On x64 SFrameText shifts down by 12
      // bytes (Text* and ReducedText* are now 8 bytes each, with leading
      // alignment padding) so TrueTypeTextureIndex lands at offset 52 —
      // distinct from SepiaColor — and stays at 0 from SHeap::Add()'s
      // memset. RerenderText then reads index 0 as "I already own a
      // texture" and releases slot 0 every time, eventually causing every
      // text frame to share that slot. Initialize explicitly.
      p_Frames->array[v11].data.Text.TrueTypeTextureIndex = -1;
      break;
    case FT_BOX:
      array[v11].data.Sprite.Font = -1;
      // FT_BOX shares the TEXT destroy path (DestroyFrame switch groups
      // FT_TEXT and FT_BOX together — see board.cpp:441-453), so it must
      // also leave TrueTypeTextureIndex at the -1 sentinel. On x86 this
      // aliased Sprite.SepiaColor at offset 40 and was masked; on x64
      // SFrameText shifts down (8-byte Text*/ReducedText* + alignment)
      // so TrueTypeTextureIndex lands at offset 52 and stays at 0 from
      // SHeap::Add()'s memset. Every FT_BOX destruction (e.g. ~SMainMenu,
      // ~SDXWidget tooltip teardown) then calls ReleaseTexture(0), which
      // evicts the engine-cached _folyo at slot 0 — water then samples
      // whatever subsequent texture lands in the recycled slot, and
      // shader-decals likewise show whatever was last freed/reallocated.
      array[v11].data.Text.TrueTypeTextureIndex = -1;
      break;
    case FT_ANIM:
      array[v11].data.Sprite.Font = 0;
      break;
    case FT_SCALER:
      array[v11].data.Sprite.Font = 1065353216;
      array[v11].data.Scaler.VirtualWidth = 0;   // Panzers: no virtual size (uniform ScaleFactor)
      array[v11].data.Scaler.VirtualHeight = 0;
      break;
    default:
      break;
  }
  v13 = p_Frames->array;
  Child = p_Frames->array[v9].data.Child;
  if ( ontop )
  {
    if ( Child >= 0 )
    {
      for ( i = v13[Child].data.Sibling; i >= 0; i = v13[i].data.Sibling )
        Child = i;
      v13[Child].data.Sibling = parenta;
      p_Frames->array[v11].data.Sibling = -1;
      return parenta;
    }
    else
    {
      v13[v9].data.Child = result;
      p_Frames->array[v11].data.Sibling = -1;
    }
  }
  else
  {
    v13[v11].data.Sibling = Child;
    p_Frames->array[v9].data.Child = result;
  }
  return result;
}

//----- (0041C830) --------------------------------------------------------

void SBoard::DestroyFrame(int idx)
{
  SHeap<SFrame>::__Tstruct *array = this->Frames.array;

  // Recursively destroy all children
  if (array[idx].data.Child >= 0) {
    int Child = array[idx].data.Child;
    while (Child >= 0) {
      int NextSibling = this->Frames.array[Child].data.Sibling;
      this->DestroyFrame(Child);
      Child = NextSibling;
    }
    array = this->Frames.array;
  }

  // Unlink from parent's child list
  int *p_parentChild = &array[array[idx].data.Parent].data.Child;
  if (idx == *p_parentChild) {
    *p_parentChild = array[idx].data.Sibling;
  } else {
    int prev = *p_parentChild;
    while (array[prev].data.Sibling >= 0 && array[prev].data.Sibling != idx) {
      prev = array[prev].data.Sibling;
    }
    if (array[prev].data.Sibling < 0)
      Logger.g->Panic("SBoard::DestroyFrame: Frame structure is damaged");
    array[prev].data.Sibling = array[idx].data.Sibling;
  }

  // Type-specific resource release
  switch (array[idx].data.Type) {
    case FT_SPRITE:         // 1
    case FT_MINIMAP:        // 6
    case FT_SPRITE_9SLICE:  // 8
      this->ReleaseFont(array[idx].data.Sprite.Font);
      break;
    case FT_TEXT:           // 2
    case FT_BOX:            // 3 — IDA grouped with TEXT; Text union members cleared
      this->ReleaseFont(array[idx].data.Text.Font);
      if (array[idx].data.Text.Text) {
        free(array[idx].data.Text.Text);
        array[idx].data.Text.Text = nullptr;
      }
      if (array[idx].data.Text.ReducedText) {
        ::operator delete(array[idx].data.Text.ReducedText);
        array[idx].data.Text.ReducedText = nullptr;
      }
      this->Gepard->ReleaseTexture(array[idx].data.Text.TrueTypeTextureIndex, 0);
      break;
    case FT_ANIM:           // 5
      this->StopAnim(idx);
      break;
    default:
      break;
  }

  // SHeap::Remove inlined
  if (idx < 0 || idx >= this->Frames.size || array[idx].use != 0x7FFFFFFF)
    Logger.g->Panic("SHeap::Remove: invalid index (%d)", idx);
  array[idx].use = this->Frames.nextempty;
  --this->Frames.occupied;
  this->Frames.nextempty = idx;
}

//----- (0041CAD0) --------------------------------------------------------

int SBoard::GetFontTexture(int idx)

{
  SHeap<SFontProp>::__Tstruct *array; // ecx
  if ( idx >= 0 && idx < this->Fonts.size && (array = this->Fonts.array, array[idx].use == 0x7FFFFFFF) )
    return array[idx].data.TextureIndex;
  else
    return -1;
}

//----- (0041CB00) --------------------------------------------------------

void SBoard::GetFrameSize(int idx, int *width, int *height)

{
  *width = this->Frames.array[idx].data.Width;
  *height = this->Frames.array[idx].data.Height;
}

//----- (0041CB30) --------------------------------------------------------

HFONT SBoard::GetHFont(SFontProp *font, bool *doubleMode, float scaleFactor)

{
  TypeFace typeFace;
  float fontSize; // xmm1_4
  float v6; // xmm1_4
  const char *v7; // ecx
  float v8; // xmm1_4
  int v9;
  HFONT result;
  typeFace = font->typeFace;
  fontSize = (float)font->fontSize;
  if ( typeFace == Pig )
    fontSize = fontSize * 1.3f;
  v6 = fontSize * scaleFactor;
  if ( typeFace == Small || typeFace == ChineseSmall || v6 >= 18.0 )
  {
    *doubleMode = 0;
  }
  else
  {
    v6 = v6 + v6;
    *doubleMode = 1;
  }
  v7 = "Microsoft Sans Serif";
  switch ( font->typeFace )
  {
    case Pig:
      v7 = "SwinePigHu";
      break;
    case Rabbit:
      v7 = "SwineRabbitHu";
      break;
    case Small:
      v7 = "Noto Sans Cond";
      break;
    case Arial:
      v7 = "Arial";
      break;
    case ChineseSmall:
    case ChinesePig:
      v7 = "Noto Sans CJK SC Medium";
      break;
    case ChineseRabbit:
      v7 = "Noto Serif CJK SC Medium";
      break;
    default:
      break;
  }
  v8 = v6 + 0.5f;
  v9 = 0;
  if ( font->Bold )
    v9 = 700;
  int fontHeight = -(int)v8;
  result = CreateFontA(fontHeight, 0, 0, 0, v9, font->Italic, 0, 0, 1u, 0, 0, 4u, 0x22u, v7);
  if ( !result )
    Logger.g->Panic("SBoard::SetText: CreateFont failed");
#ifdef HD_DEBUG_FONTS
  // Log actual font metrics
  {
    static int gethfont_logcount = 0;
    if (gethfont_logcount < 10) {
      HDC tmpDC = CreateCompatibleDC(0);
      if (tmpDC) {
        HFONT old = (HFONT)SelectObject(tmpDC, result);
        TEXTMETRICA tm;
        GetTextMetricsA(tmpDC, &tm);
        char actualName[64] = {0};
        GetTextFaceA(tmpDC, 64, actualName);
        Logger.g->Log(0, "GetHFont: requested='%s' h=%d, actual='%s' tmHeight=%d tmAscent=%d tmDescent=%d",
          v7, fontHeight, actualName, tm.tmHeight, tm.tmAscent, tm.tmDescent);
        SelectObject(tmpDC, old);
        DeleteDC(tmpDC);
      }
      gethfont_logcount++;
    }
  }
#endif
  return result;
}

//----- (0041CC30) --------------------------------------------------------

float SBoard::GetMinimapRotation(int frameIdx)

{
  SHeap<SFrame>::__Tstruct *array; // ecx
  array = this->Frames.array;
  if ( array[frameIdx].data.Type != FT_MINIMAP )
    Logger.g->Panic("SBoard::GetMinimapRotation: Not a minimap frame");
  return array[frameIdx].data.Minimap.Rotation;
}

//----- (0041CC80) --------------------------------------------------------

float SBoard::GetTextEffectiveScaleFactor(int idx)

{
  float v2; // xmm1_4
  int Parent;
  SHeap<SFrame>::__Tstruct *array; // ecx
  int v5;
  float scaleFactor;
  v2 = 1.0f;
  Parent = idx;
  scaleFactor = 1.0f;
  if ( idx >= 0 )
  {
    array = this->Frames.array;
    do
    {
      v5 = Parent;
      if ( array[v5].data.Type == FT_SCALER )
      {
        // Panzers: a scaler with a virtual size scales text by its height
        // ratio (the glyph quads are stretched in x by the render).
        if ( array[v5].data.Scaler.VirtualHeight > 0 )
          v2 = v2 * ((float)array[v5].data.Height / (float)array[v5].data.Scaler.VirtualHeight);
        else
          v2 = v2 * array[v5].data.Scaler.ScaleFactor;
      }
      Parent = array[v5].data.Parent;
    }
    while ( Parent >= 0 );
    return v2;
  }
  return scaleFactor;
}

//----- (0041CCD0) --------------------------------------------------------

void SBoard::GetTextExtent(int font, const char *str, int nchars, int *width, int *height, float scaleFactor)

{
  SBoard *v7; // edx
  float v8; // xmm2_4
  int *v9; // ebx
  int v10;
  SHeap<SFontProp>::__Tstruct *array; // eax
  int v12;
  const char *v13; // esi
  const char *v14; // edi
  const char *v15; // ebx
  unsigned char v16; // al
  unsigned int v17;
  char v18; // al
  unsigned int v19;
  char v20; // cl
  char v21; // bl
  char v22; // cl
  char v23; // bl
  char v24; // bh
  HDC CompatibleDC;
  const char *v26; // edi
  int v27;
  unsigned int v28;
  const char *v29; // esi
  int v30;
  const char *v31; // ecx
  unsigned int v32;
  unsigned int v33;
  char v34; // al
  char v35; // cl
  unsigned char v36; // ah
  char v37; // cl
  unsigned char v38; // ah
  wchar_t *v39; // esi
  const char *v40; // ecx
  unsigned int v41;
  unsigned int v42;
  char v43; // al
  char v44; // cl
  char v45; // ah
  char v46; // cl
  char v47; // ah
  int v48;
  int v49;
  int bottom;
  HFONT ho;
  HDC hdc;
  wchar_t *lpchText;
  const char *v54;
  const char *v55;
  const char *v56;
  unsigned int v57;
  int v59;
  bool doubleMode;
  const char *v61;
  unsigned char v62;
  tagRECT size;
  v7 = this;
  v8 = scaleFactor;
  v9 = width;
  if ( font < 0 || font >= this->Fonts.size || (v10 = font, array = v7->Fonts.array, array[font].use != 0x7FFFFFFF) )
  {
    // Return zero extents instead of panicking — font may not be loaded yet during menu setup
    *width = 0;
    *height = 0;
    return;
  }
  if ( array[v10].data.typeFace )
  {
    CompatibleDC = CreateCompatibleDC(0);
    hdc = CompatibleDC;
    if ( !CompatibleDC )
      Logger.g->Panic("SBoard::GetTextExtent: CreateCompatibleDC failed");
    ho = this->GetHFont(&this->Fonts.array[font].data, &doubleMode, scaleFactor);
    SelectObject(CompatibleDC, ho);
    v26 = " ";
    v59 = 0;
    v27 = 1;
    if ( str )
    {
      v27 = nchars;
      v26 = str;
    }
    v28 = (unsigned int)&v26[v27];
    v56 = v26;
    v29 = v26;
    if ( (unsigned int)v26 < v28 )
    {
      v30 = 0;
      while ( 1 )
      {
        v31 = v29;
        v61 = v29;
        if ( (unsigned int)v29 < v28 )
        {
          v33 = *(unsigned char *)v29++;
          if ( v33 - 193 > 0x1E )
          {
            if ( v33 - 225 > 0xE )
            {
              if ( v33 - 241 > 6
                || (v61 = v31 + 4, (unsigned int)(v31 + 4) > v28)
                || (v37 = *v29, *v29 < 0x80u)
                || (unsigned char)v37 > 0xBFu
                || (v38 = v29[1], (unsigned char)(v38 + 0x80) > 0x3Fu)
                || (v62 = v29[2], (unsigned char)(v62 + 0x80) > 0x3Fu) )
              {
LABEL_57:
                v32 = v33;
                goto LABEL_58;
              }
              v29 = v61;
              v32 = (v62 & 0x3F) + (((v38 & 0x3F) + ((((v33 & 7) << 6) + (v37 & 0x3F)) << 6)) << 6);
            }
            else
            {
              v61 = v31 + 3;
              if ( (unsigned int)(v31 + 3) > v28 )
                goto LABEL_57;
              v35 = *v29;
              if ( *v29 < 0x80u )
                goto LABEL_57;
              if ( (unsigned char)v35 > 0xBFu )
                goto LABEL_57;
              v36 = v29[1];
              if ( (unsigned char)(v36 + 0x80) > 0x3Fu )
                goto LABEL_57;
              v29 = v61;
              v32 = (v36 & 0x3F) + ((((v33 & 0xF) << 6) + (v35 & 0x3F)) << 6);
            }
          }
          else
          {
            if ( (unsigned int)(v29 + 1) > v28 )
              goto LABEL_57;
            v34 = *v29;
            if ( *v29 < 0x80u || (unsigned char)v34 > 0xBFu )
              goto LABEL_57;
            v32 = ((v33 & 0x1F) << 6) + (v34 & 0x3F);
            v29 = v61 + 2;
          }
        }
        else
        {
          v32 = 0;
        }
LABEL_58:
        v30 += 2 - (v32 < 0x10000);
        if ( (unsigned int)v29 >= v28 )
        {
          v59 = v30;
          v26 = v56;
          break;
        }
      }
    }
    v39 = (wchar_t *)operator new[](2 * v59);
    lpchText = v39;
    v57 = (unsigned int)&v39[v59];
    if ( (unsigned int)v26 >= v28 )
    {
LABEL_88:
      memset(&size, 0, sizeof(size));
      DrawTextW(hdc, lpchText, v59, &size, 0xC20u);
      v9 = width;
      if ( doubleMode )
      {
        *width = size.right >> 1;
        bottom = size.bottom >> 1;
      }
      else
      {
        *width = size.right;
        bottom = size.bottom;
      }
      *height = bottom;
      DeleteObject(ho);
      DeleteDC(hdc);
      operator delete[](lpchText);
      v12 = *width;
      v8 = scaleFactor;
      goto LABEL_92;
    }
    while ( 1 )
    {
      v40 = v26;
      v61 = v26;
      if ( (unsigned int)v26 < v28 )
      {
        v42 = *(unsigned char *)v26++;
        if ( v42 - 193 > 0x1E )
        {
          if ( v42 - 225 > 0xE )
          {
            if ( v42 - 241 > 6
              || (v61 = v40 + 4, (unsigned int)(v40 + 4) > v28)
              || (v46 = *v26, *v26 < 0x80u)
              || (unsigned char)v46 > 0xBFu
              || (v47 = v26[1], (unsigned char)(v47 + 0x80) > 0x3Fu)
              || (v62 = v26[2], (unsigned char)(v62 + 0x80) > 0x3Fu) )
            {
LABEL_81:
              v41 = v42;
              goto LABEL_82;
            }
            v26 = v61;
            v41 = (v62 & 0x3F) + (((v47 & 0x3F) + ((((v42 & 7) << 6) + (v46 & 0x3F)) << 6)) << 6);
          }
          else
          {
            v61 = v40 + 3;
            if ( (unsigned int)(v40 + 3) > v28 )
              goto LABEL_81;
            v44 = *v26;
            if ( *v26 < 0x80u )
              goto LABEL_81;
            if ( (unsigned char)v44 > 0xBFu )
              goto LABEL_81;
            v45 = v26[1];
            if ( (unsigned char)(v45 + 0x80) > 0x3Fu )
              goto LABEL_81;
            v26 = v61;
            v41 = (v45 & 0x3F) + ((((v42 & 0xF) << 6) + (v44 & 0x3F)) << 6);
          }
        }
        else
        {
          if ( (unsigned int)(v26 + 1) > v28 )
            goto LABEL_81;
          v43 = *v26;
          if ( *v26 < 0x80u || (unsigned char)v43 > 0xBFu )
            goto LABEL_81;
          v41 = ((v42 & 0x1F) << 6) + (v43 & 0x3F);
          v26 = v61 + 2;
        }
      }
      else
      {
        v41 = 0;
      }
LABEL_82:
      v48 = 2 - (v41 < 0x10000);
      if ( (unsigned int)&v39[v48] <= v57 )
      {
        v49 = v48 - 1;
        if ( v49 )
        {
          if ( v49 == 1 )
          {
            *v39 = (((v41 - 0x10000) >> 10) & 0x3FF) - 10240;
            v39[1] = (v41 & 0x3FF) - 9216;
            v39 += 2;
          }
        }
        else
        {
          *v39++ = v41;
        }
      }
      if ( (unsigned int)v26 >= v28 )
        goto LABEL_88;
    }
  }
  *height = (int)(float)((float)((float)(array[v10].data.Glyphs[32].Dest.Bottom - array[v10].data.Glyphs[32].Dest.Top)
                               - (float)array[v10].data.TopMargin)
                       - (float)array[v10].data.BottomMargin);
  // HD 0x6c5530: bitmap-font extents are in the font's own (virtual)
  // units; HD does not divide them by the text scale. Only the TrueType path
  // above measures scaled pixels and converts back.
  v8 = 1.0f;
  v12 = 0;
  v13 = str;
  v14 = &str[nchars];
  *width = 0;
  if ( str < &str[nchars] )
  {
    while ( 1 )
    {
      v15 = v13;
      v61 = (const char *)&v7->Fonts.array[v10];
      if ( v13 < v14 )
        break;
      v16 = UnicodeDecodeTable[0];
LABEL_30:
      v9 = width;
      v7 = this;
      v12 = *(_DWORD *)&v61[36 * v16 + 52] + *width;
      v10 = font;
      *width = v12;
      if ( v13 >= v14 )
        goto LABEL_92;
    }
    v17 = *(unsigned char *)v13++;
    if ( v17 - 193 > 0x1E )
    {
      if ( v17 - 225 > 0xE )
      {
        if ( v17 - 241 <= 6 )
        {
          v55 = v15 + 4;
          if ( v15 + 4 <= v14 )
          {
            v22 = *v13;
            if ( *v13 >= 0x80u && (unsigned char)v22 <= 0xBFu )
            {
              v23 = v13[1];
              if ( (unsigned char)(v23 + 0x80) <= 0x3Fu )
              {
                v24 = v13[2];
                if ( (unsigned char)(v24 + 0x80) <= 0x3Fu )
                {
                  v13 = v55;
                  v19 = (v24 & 0x3F) + (((v23 & 0x3F) + ((((v17 & 7) << 6) + (v22 & 0x3F)) << 6)) << 6);
                  goto LABEL_27;
                }
              }
            }
          }
        }
      }
      else
      {
        v54 = v15 + 3;
        if ( v15 + 3 <= v14 )
        {
          v20 = *v13;
          if ( *v13 >= 0x80u && (unsigned char)v20 <= 0xBFu )
          {
            v21 = v13[1];
            if ( (unsigned char)(v21 + 0x80) <= 0x3Fu )
            {
              v13 = v54;
              v19 = (v21 & 0x3F) + ((((v17 & 0xF) << 6) + (v20 & 0x3F)) << 6);
              goto LABEL_27;
            }
          }
        }
      }
    }
    else if ( v13 + 1 <= v14 )
    {
      v18 = *v13;
      if ( *v13 >= 0x80u && (unsigned char)v18 <= 0xBFu )
      {
        v19 = ((v17 & 0x1F) << 6) + (v18 & 0x3F);
        v13 = v15 + 2;
        goto LABEL_27;
      }
    }
    v19 = v17;
LABEL_27:
    if ( v19 >= 0x460 )
      v16 = 63;
    else
      v16 = UnicodeDecodeTable[v19];
    goto LABEL_30;
  }
LABEL_92:
  *v9 = (int)(float)((float)v12 / v8);
  *height = (int)(float)((float)*height / v8);
}

//----- (0041D2D0) --------------------------------------------------------

void SBoard::GravitateFrame(int idx, int gravity)

{
  int v3;
  v3 = idx;
  this->Frames.array[v3].data.Flags &= 0xFFFFFFFA;
  this->Frames.array[v3].data.Flags |= gravity & 5;
}

//----- (0041D2F0) --------------------------------------------------------

void SBoard::InitHotspots()

{
  // Cursor hotspot data (24 cursors, base coordinates at 32px cursor size)
  // These are the hot pixel offsets for each cursor in the atlas.
  // Values from .rdata _xmm constants at 0x540E60..0x540EF0
  static const POINT baseHotspots[24] = {
    {0,0}, {0,0},    // 0-1: normal arrow
    {0,0}, {0,0},    // 2-3
    {16,16}, {16,16}, // 4-5: crosshair-type
    {16,16}, {16,16}, // 6-7
    {16,17},          // 8
    {0,0}, {0,0},     // 9-10: doubled from _xmm+_xmm
    {0,0}, {0,0},     // 11-12
    {0,0}, {0,0},     // 13-14
    {0,0}, {0,0},     // 15-16
    {0,0}, {0,0},     // 17-18
    {0,0}, {0,0},     // 19-20
    {0,0}, {0,0},     // 21-22
    {0,32},           // 23
  };
  float scale = (float)this->CursorSize * 0.03125f; // CursorSize / 32.0
  this->NumCursors = 24;
  POINT *v6 = (POINT *)operator new[](sizeof(POINT) * 24);
  this->CursorHotspots = v6;
  for (int i = 0; i < 24; i++) {
    v6[i].x = (int)((float)baseHotspots[i].x * scale);
    v6[i].y = (int)((float)baseHotspots[i].y * scale);
  }
}

//----- (0041D550) --------------------------------------------------------

bool SBoard::IsAnimPlaying(int idx)

{
  SHeap<SFrame>::__Tstruct *array; // eax
  array = this->Frames.array;
  if ( array[idx].data.Type != FT_ANIM )
    Logger.g->Panic("SBoard::SetAnim: Not an anim frame");
  // Use Anim.Anim, not Sprite.Font. They alias at union offset 0, but on x64
  // SAnimation* is 8 bytes — Sprite.Font (4 bytes) only reads the low half of
  // the pointer and gives spurious-true gates here, so ShowEventMsg drops the
  // event (most visible on enemy reportanims via EventAnimFrames[2]). Cf. the
  // matching write-side notes in StartAnim/StopAnim below.
  return array[idx].data.Anim.Anim != nullptr;
}

// PANZERS 0x6c59e0
// SBoard::LoadCursorSet as HD Panzers has it (vtable +0x94), called once from
// SSuperWindow::Initialize 0x657910 with ("menu/cursor2_hq.tga", 0x28, 0x15,
// hotspots). HD:
//   +0x3c = new POINT[count], memcpy(hotspots)   -> CursorHotspots
//   +0x50 = count                                -> NumCursors
//   +0x40 = LoadFixedFont(file, size, size, 256/size, count, 0) -> CursorFont
//   +0x58 = 0, +0x44 = -1 (glyph), +0x54 = 0 (cursor variant)
//   +0xdc = new 0x20 SBitmap loaded from the same file, +0xe0 = size: the
//           source of the D3D hardware cursor (+0xc4 0x6ca4e0).
// The hardware-cursor bitmap is not ported (see ENGINE_DIFF.md): the
// recompile always draws the software cursor, which is what HD does with
// options.ini "Hardware Mouse Cursor" = 0 (the default).
void SBoard::LoadCursorSetFile(const char *filename, int size, int count, const POINT *hotspots)
{
  if ( this->CursorFont >= 0 || this->CursorHotspots )
    this->UnloadCursorSet();
  this->CursorHotspots = new POINT[count];
  memcpy(this->CursorHotspots, hotspots, sizeof(POINT) * count);
  this->NumCursors = count;
  this->CursorFont = this->LoadFixedFont(filename, size, size, 256 / size, count, 0, Default);
  this->CursorGlyph = -1;
  this->CursorX = 0;
  this->CursorY = 0;
  this->CursorSize = size;
  this->CursorIcons = 0;
  // SWINE flags read by Render/ApplyHardwareCursor. HD: +0xe4 (hardware
  // cursor) is set by SetHardwareMouseCursor +0xc8; forced off here.
  this->CursorVisible = true;
  this->HardwareCursor = false;
  this->ForceSoftwareCursor = false;
}

//----- (0041D580) --------------------------------------------------------

void SBoard::LoadCursorSet(float windowScale)

{
  float v3; // xmm1_4
  int v4;
  int v5;
  const char *FileInSearchPath; // eax
  int v9;
  HBITMAP__ *v10; // eax
  tagPOINT *CursorHotspots; // ecx
  HICON v12;
  HICON__ **CursorIcons; // ecx
  int v14;
  int v15;
  int v16;
  int CursorSize;
  int v18;
  SBitmap cursorBitMap;
  SBitmap atlasBitMap;
  _ICONINFO iconInfo;
  SString result;
  int v24;
  int src_x;
  char atlasFile[260];
  int v27;
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: start, windowScale=%f", windowScale);
#endif
  *(_WORD *)&this->CursorVisible = 256;
  this->ForceSoftwareCursor = 0;
  v3 = (float)(windowScale * 32.0f) + 0.5f;
  this->CursorGlyph = -1;
  if ( v3 >= 40.0 )
  {
    if ( v3 >= 56.0 )
    {
      if ( v3 >= 80.0 )
      {
        if ( v3 >= 112.0 )
          this->CursorSize = 128;
        else
          this->CursorSize = 96;
      }
      else
      {
        this->CursorSize = 64;
      }
    }
    else
    {
      this->CursorSize = 48;
    }
  }
  else
  {
    this->CursorSize = 32;
  }
  this->InitHotspots();
  sprintf(atlasFile, "menu/cursor_atlas_%d.png", this->CursorSize);
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: CursorSize=%d, calling LoadFixedFont(%s)", this->CursorSize, atlasFile);
#endif
  this->CursorFont = this->LoadFixedFont(
                       atlasFile,
                       this->CursorSize,
                       this->CursorSize,
                       8,
                       this->NumCursors,
                       0,
                       Default);
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: LoadFixedFont returned %d", this->CursorFont);
#endif
  atlasBitMap = SBitmap();
  v27 = 0;
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: loading atlas PNG");
#endif
  atlasBitMap.LoadPNG(atlasFile, "SBoard::LoadCursorSet");
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: atlas loaded W=%d H=%d, allocating CursorIcons for %d cursors", atlasBitMap.Width, atlasBitMap.Height, this->NumCursors);
#endif
  v4 = 0;
  this->CursorIcons = (HICON__ **)operator new[](sizeof(HICON__ *) * this->NumCursors);
  src_x = 0;
  v5 = 0;
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: entering cursor loop, NumCursors=%d", this->NumCursors);
#endif
  while ( v5 < this->NumCursors )
  {
    v24 = v4;
    {
      char cursorFile[260];
      sprintf(cursorFile, "menu/cursors/cursor_%02d.cur", v5);
      FileInSearchPath = FileSystem.FindFileInSearchPath(cursorFile);
    }
#ifdef HD_DEBUG_BOARD
    Logger.g->Log(0, "SBoard::LoadCursorSet: cursor %d, path=%s", v5, FileInSearchPath ? FileInSearchPath : "NULL");
#endif
    this->CursorIcons[v5] = LoadCursorFromFileA(FileInSearchPath);
    if ( this->CursorIcons[v5] )
    {
      v9 = src_x;
    }
    else
    {
      new (&cursorBitMap) SBitmap(this->CursorSize, this->CursorSize, atlasBitMap.Format, 0);
      v18 = v4;
      v9 = src_x;
      CursorSize = this->CursorSize;
      cursorBitMap.BitBlt(0, 0, CursorSize, CursorSize, &atlasBitMap, src_x, v18);
      v10 = cursorBitMap.CreateWinBitmap();
      CursorHotspots = this->CursorHotspots;
      iconInfo.hbmColor = v10;
      iconInfo.hbmMask = v10;
      iconInfo.fIcon = 0;
      iconInfo.xHotspot = CursorHotspots[v5].x;
      iconInfo.yHotspot = CursorHotspots[v5].y;
      v12 = CreateIconIndirect(&iconInfo);
      CursorIcons = this->CursorIcons;
      CursorIcons[v5] = v12;
      cursorBitMap.~SBitmap();
    }
    v14 = this->CursorSize;
    ++v5;
    v15 = v14 + v9;
    v16 = 0;
    if ( v15 < 8 * v14 )
      v16 = v15;
    src_x = v16;
    v4 = v14 + v24;
    if ( v15 < 8 * v14 )
      v4 = v24;
  }
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: cursor loop done, cleaning up");
#endif
  atlasBitMap.~SBitmap();
  this->CursorVisible = true;
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "SBoard::LoadCursorSet: done");
#endif
}

//----- (0041D860) --------------------------------------------------------

int SBoard::LoadCustomFont(const char *filename, int numglyphs, SCustomGlyph *glyphs, HDMode hdmode)

{
  SBoard *v5; // esi
  char *v6; // ebx
  char *v7; // eax
  int v8;
  char *v9; // edi
  int *p_Height; // ebx
  float v11; // xmm1_4
  float v12; // xmm2_4
  int v13;
  int v14;
  int v15;
  int v16;
  int v17;
  SBitmap v19;
  SBitmap bmap;
  SBoard *v21;
  char *v22;
  int tex_width;
  int tex_height;
  float v25;
  float v26;
  _DWORD Src[2316];
  int v28;
  v5 = this;
  v21 = this;
  v6 = (char *)filename;
  v22 = (char *)filename;
  bmap = SBitmap();
  v28 = 0;
  if ( strrchr((char *)filename, 46) && (v7 = strrchr((char *)filename, 46), !_stricmp(v7, ".png")) )
    bmap.LoadPNG((char *)filename, "SBoard::LoadCustomFont");
  else
    bmap.LoadTGA((char *)filename, "SBoard::LoadCustomFont");
  v5->Gepard->RoundToTextureSize( bmap.Width, bmap.Height, &tex_width, &tex_height);
  v8 = tex_height;
  v25 = 1.0f / (float)tex_width;
  v26 = 1.0f / (float)tex_height;
  if ( hdmode == X2 )
  {
    v25 = (float)(1.0 / (float)tex_width) + (float)(1.0 / (float)tex_width);
    v26 = (float)(1.0 / (float)tex_height) + (float)(1.0 / (float)tex_height);
  }
  memset(Src, 0, sizeof(Src));
  *(_QWORD *)((char *)Src + 4) = 1LL;
  Src[3] = numglyphs;
  if ( numglyphs > 0 )
  {
    v9 = (char *)&Src[16] + 4;
    p_Height = &glyphs->Height;
    v11 = v26;
    v12 = v25;
    do
    {
      v13 = *(p_Height - 1);
      v14 = *p_Height;
      v15 = *(p_Height - 3);
      v16 = *(p_Height - 2);
      p_Height += 4;
      *((_DWORD *)v9 - 5) = v13;
      *(_DWORD *)v9 = -1090519040;
      *((_DWORD *)v9 + 1) = -1090519040;
      *((float *)v9 + 2) = (float)v13 - 0.5f;
      *((float *)v9 + 3) = (float)v14 - 0.5f;
      *((float *)v9 - 4) = (float)v15 * v12;
      *((float *)v9 - 3) = (float)v16 * v11;
      *((float *)v9 - 2) = (float)(v13 + v15) * v12;
      *((float *)v9 - 1) = (float)(v16 + v14) * v11;
#ifdef HD_DEBUG_FONTS
      // Log last few glyphs for diagnostic
      if (numglyphs <= 3) {
        Logger.g->Log(0, "LoadCustomFont[%s] glyph: x=%d y=%d w=%d h=%d -> Src=(%.6f,%.6f,%.6f,%.6f) tex=%dx%d bmap=%dx%d",
          v22, v15, v16, v13, v14,
          (float)v15 * v12, (float)v16 * v11,
          (float)(v13+v15) * v12, (float)(v16+v14) * v11,
          tex_width, tex_height, bmap.Width, bmap.Height);
      }
#endif
      v9 += 36;
      --numglyphs;
    }
    while ( numglyphs );
    v6 = v22;
    v5 = v21;
    v8 = tex_height;
  }
  if ( bmap.Width == tex_width && bmap.Height == v8 )
  {
    Src[0] = v5->Gepard->CreateTextureFromBitmap( v6, &bmap, 0);
  }
  else
  {
    new (&v19) SBitmap(tex_width, v8, bmap.Format, 0);
    memset(v19.Data, 0, v19.Size);
    v19.BitBlt(0, 0, bmap.Width, bmap.Height, &bmap, 0, 0);
    Src[0] = v5->Gepard->CreateTextureFromBitmap( v6, &v19, 0);
    v19.~SBitmap();
  }
  memset(&Src[4], 0, 16);
  v17 = v5->Fonts.Add();
  memcpy(&v5->Fonts.array[v17].data, Src, sizeof(v5->Fonts.array[v17].data));
  bmap.~SBitmap();
  return v17;
}

//----- (0041DBA0) --------------------------------------------------------

int SBoard::LoadFixedFont(const char *filename, int width, int height, int row, int numglyphs, unsigned char *glyphs, HDMode hdmode)

{
  char *v9; // edi
  char *v10; // eax
  int v11;
  int v12;
  int v13;
  float v14; // xmm1_4
  float v15; // xmm2_4
  int v16;
  int v17;
  int v18;
  int v19;
  int v20;
  SBoard *v21; // esi
  int v22;
  SBitmap v24;
  SBitmap bmap;
  char *v26;
  SBoard *v27;
  int tex_width;
  float v29;
  float v30;
  int tex_height;
  int v32;
  int v33;
  _DWORD Src[2316];
  int v35;
  v27 = this;
  v9 = (char *)filename;
  v26 = (char *)filename;
  bmap = SBitmap();
  v35 = 0;
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: loading %s", filename);
#endif
  if ( strrchr((char *)filename, 46) && (v10 = strrchr((char *)filename, 46), !_stricmp(v10, ".png")) )
    bmap.LoadPNG((char *)filename, "SBoard::LoadFixedFont");
  else
    bmap.LoadTGA((char *)filename, "SBoard::LoadFixedFont");
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: loaded, W=%d H=%d Pixel=%d Data=%p", bmap.Width, bmap.Height, bmap.Pixel, bmap.Data);
  Logger.g->Log(0, "SBoard::LoadFixedFont: RoundToTextureSize w=%d h=%d Gepard=%p", bmap.Width, bmap.Height, this->Gepard);
#endif
  this->Gepard->RoundToTextureSize( bmap.Width, bmap.Height, &tex_width, &tex_height);
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: tex=%dx%d, numglyphs=%d row=%d hdmode=%d", tex_width, tex_height, numglyphs, row, (int)hdmode);
#endif
  v29 = 1.0f / (float)tex_width;
  v30 = 1.0f / (float)tex_height;
  if ( hdmode == X2 )
  {
    v29 = (float)(1.0 / (float)tex_width) + (float)(1.0 / (float)tex_width);
    v30 = (float)(1.0 / (float)tex_height) + (float)(1.0 / (float)tex_height);
  }
  memset(Src, 0, sizeof(Src));
  v11 = 0;
  Src[1] = 1;
  v12 = 256;
  v33 = 0;
  v13 = 0;
  Src[2] = 256;
  Src[3] = 0;
  v32 = 0;
  if ( numglyphs > 0 )
  {
    v14 = v30;
    v15 = v29;
    while ( 1 )
    {
      if ( glyphs )
        v16 = glyphs[v13];
      else
        v16 = v13;
      if ( v12 > v16 )
        v12 = v16;
      Src[2] = v12;
      if ( v11 < v16 )
        v11 = v16;
      Src[3] = v11;
      v17 = v33 + height;
      if ( v33 + height > bmap.Height )
        Logger.g->Panic("SBoard::LoadFixedFont: %s: Too few glyphs in the bitmap", v9);
      v18 = 9 * v16;
      Src[12 + v18] = width;
      v19 = 0;
      Src[16 + v18 + 1] = -1090519040;
      ++v13;
      Src[16 + v18 + 2] = -1090519040;
      v20 = v32 + width;
      *(float *)&Src[16 + v18 + 3] = (float)width - 0.5f;
      *(float *)&Src[20 + v18] = (float)height - 0.5f;
      *(float *)&Src[12 + v18 + 1] = (float)v32 * v15;
      *(float *)&Src[12 + v18 + 2] = (float)v33 * v14;
      *(float *)&Src[12 + v18 + 3] = (float)v20 * v15;
      *(float *)&Src[16 + v18] = (float)v17 * v14;
      if ( v20 < row * width )
      {
        v17 = v33;
        v19 = v20;
      }
      v32 = v19;
      v9 = v26;
      v33 = v17;
      if ( v13 >= numglyphs )
        break;
      v11 = Src[3];
      v12 = Src[2];
    }
  }
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: glyph loop done, creating texture");
#endif
  if ( bmap.Width == tex_width && bmap.Height == tex_height )
  {
    v21 = v27;
#ifdef HD_DEBUG_FONTS
    Logger.g->Log(0, "SBoard::LoadFixedFont: same-size path, calling CreateTextureFromBitmap");
#endif
    Src[0] = v27->Gepard->CreateTextureFromBitmap( v9, &bmap, 0);
  }
  else
  {
#ifdef HD_DEBUG_FONTS
    Logger.g->Log(0, "SBoard::LoadFixedFont: resize path %dx%d -> %dx%d", bmap.Width, bmap.Height, tex_width, tex_height);
#endif
    new (&v24) SBitmap(tex_width, tex_height, bmap.Format, 0);
    memset(v24.Data, 0, v24.Size);
    v24.BitBlt(0, 0, bmap.Width, bmap.Height, &bmap, 0, 0);
    v21 = v27;
    Src[0] = v27->Gepard->CreateTextureFromBitmap( v9, &v24, 0);
    v24.~SBitmap();
  }
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: texture created, adding font");
#endif
  memset(&Src[4], 0, 16);
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: Fonts.Add() sizeof(data)=%d sizeof(Src)=%d", (int)sizeof(v21->Fonts.array[0].data), (int)sizeof(Src));
#endif
  v22 = v21->Fonts.Add();
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: Fonts.Add() returned %d, array=%p", v22, v21->Fonts.array);
#endif
  memcpy(&v21->Fonts.array[v22].data, Src, sizeof(v21->Fonts.array[v22].data));
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: memcpy done, calling bmap destructor");
#endif
  bmap.~SBitmap();
#ifdef HD_DEBUG_FONTS
  Logger.g->Log(0, "SBoard::LoadFixedFont: returning %d", v22);
#endif
  return v22;
}

//----- (0041DF80) --------------------------------------------------------

int SBoard::LoadProportionalFont(const char *filename, int lineheight, int numglyphs, unsigned char *glyphs, int top_margin, int bottom_margin, int left_margin, int right_margin)

{
  int v10;
  int v11;
  int v12;
  int v13;
  signed int v14;
  int v15;
  int v16;
  int v17;
  int v18;
  int v19;
  unsigned char *v20; // ecx
  int v21;
  int v22;
  int v23;
  __m128i v24; // xmm0
  SBoard *v25; // esi
  int v26;
  SBitmap v28;
  float v29;
  float v30;
  int v31;
  int v32;
  unsigned char *v33;
  SBitmap bmap;
  SBoard *v35;
  int tex_width;
  int v37;
  int v38;
  int v39;
  char *v40;
  int tex_height;
  int v42;
  _DWORD Src[2316];
  int v44;
  v35 = this;
  v40 = (char *)filename;
  v33 = glyphs;
  bmap = SBitmap();
  v44 = 0;
  bmap.LoadTGA((char *)filename, "SBoard::LoadProportionalFont");
  this->Gepard->RoundToTextureSize( bmap.Width, bmap.Height, &tex_width, &tex_height);
  v10 = tex_width;
  v30 = 1.0f / (float)tex_width;
  v29 = 1.0f / (float)tex_height;
  memset(Src, 0, sizeof(Src));
  if ( bmap.Format == D3DFMT_A8R8G8B8 )
  {
    v11 = 4;
  }
  else
  {
    if ( bmap.Format != D3DFMT_R8G8B8 )
      Logger.g->Panic("SBoard::LoadProportionalFont: %s: Bitmap format unsupported", filename);
    v11 = 3;
  }
  v12 = 0;
  Src[4] = top_margin;
  v13 = 256;
  v14 = 0;
  Src[5] = bottom_margin;
  Src[6] = left_margin;
  Src[7] = right_margin;
  v15 = 0;
  Src[1] = 1;
  Src[2] = 256;
  Src[3] = 0;
  v42 = 0;
  v37 = 0;
  if ( numglyphs > 0 )
  {
    while ( 1 )
    {
      v16 = v33[v15];
      v31 = v16;
      if ( v13 > v16 )
        v13 = v16;
      Src[2] = v13;
      v17 = v42;
      if ( v12 < v16 )
        v12 = v16;
      Src[3] = v12;
      v18 = v42 * bmap.Pitch;
      v32 = lineheight * bmap.Pitch;
      while ( 1 )
      {
        v39 = v18;
        v38 = lineheight + v17;
        if ( lineheight + v17 > bmap.Height )
          Logger.g->Panic("SBoard::LoadProportionalFont: %s: Too few glyphs in the bitmap", v40);
        v19 = 0;
        v20 = &bmap.Data[bmap.Start + v18 + v11 * v14];
        if ( v14 < bmap.Width )
        {
          v21 = v14;
          do
          {
            if ( *v20 == 0xFF && !v20[1] && v20[2] == 0xFF )
              break;
            ++v21;
            ++v19;
            v20 += v11;
          }
          while ( v21 < bmap.Width );
        }
        if ( v19 + v14 < bmap.Width )
          break;
        v14 = 0;
        v17 = v38;
        v18 = v32 + v39;
        v42 = v38;
      }
      *(_WORD *)v20 = 0;
      v20[2] = 0;
      if ( v11 == 4 )
        v20[3] = 0;
      v22 = v19 + 1;
      v23 = 9 * v31;
      Src[v23 + 12] = v22 - left_margin - right_margin;
      v15 = ++v37;
      *(float *)&Src[v23 + 17] = -0.5f - (float)left_margin;
      *(float *)&Src[v23 + 18] = -0.5f - (float)top_margin;
      *(float *)&Src[v23 + 19] = (float)((float)v22 - 0.5) - (float)left_margin;
      *(float *)&Src[v23 + 20] = (float)((float)lineheight - 0.5) - (float)top_margin;
      v24 = _mm_cvtsi32_si128(v14);
      v14 += v22;
      *(float *)&Src[v23 + 13] = _mm_cvtepi32_ps(v24).m128_f32[0] * v30;
      *(float *)&Src[v23 + 14] = (float)v42 * v29;
      *(float *)&Src[v23 + 15] = (float)v14 * v30;
      *(float *)&Src[v23 + 16] = (float)v38 * v29;
      if ( v15 >= numglyphs )
        break;
      v12 = Src[3];
      v13 = Src[2];
    }
    v10 = tex_width;
  }
  if ( bmap.Width == v10 && bmap.Height == tex_height )
  {
    v25 = v35;
    Src[0] = v35->Gepard->CreateTextureFromBitmap( v40, &bmap, 0);
  }
  else
  {
    new (&v28) SBitmap(v10, tex_height, bmap.Format, 0);
    memset(v28.Data, 0, v28.Size);
    v28.BitBlt(0, 0, bmap.Width, bmap.Height, &bmap, 0, 0);
    v25 = v35;
    Src[0] = v35->Gepard->CreateTextureFromBitmap( v40, &v28, 0);
    v28.~SBitmap();
  }
  v26 = v25->Fonts.Add();
  memcpy(&v25->Fonts.array[v26].data, Src, sizeof(v25->Fonts.array[v26].data));
  bmap.~SBitmap();
  return v26;
}

//----- (0041E410) --------------------------------------------------------

int SBoard::LoadSingleFont(const char *filename, HDMode hdmode)

{
  char *v4; // eax
  SHeap<SFontProp> *p_Fonts; // esi
  int v6;
  _DWORD Src[2316];
  SBitmap v9;
  SBitmap bmap;
  int tex_height;
  float v12;
  int tex_width;
  float v14;
  int v15;
  bmap = SBitmap();
  v14 = 1.0f;
  v15 = 0;
  v12 = 0.0;
  switch ( hdmode )
  {
    case X2:
      v14 = 0.5f;
      break;
    case FullHD:
      v14 = FLOAT_0_55555558;
      break;
    case FullHD_Shift:
      v14 = FLOAT_0_55555558;
      v12 = -133.33334f;
      break;
  }
  if ( strrchr((char *)filename, 46) && (v4 = strrchr((char *)filename, 46), !_stricmp(v4, ".png")) )
    bmap.LoadPNG((char *)filename, "SBoard::LoadSingleFont");
  else
    bmap.LoadTGA((char *)filename, "SBoard::LoadSingleFont");
  this->Gepard->RoundToTextureSize( bmap.Width, bmap.Height, &tex_width, &tex_height);
  memset(&Src[8], 0, 16);
  memset((char *)Src + 84, 0, 0x23DCu);
  *(_QWORD *)((char *)Src + 4) = 1LL;
  Src[3] = 0;
  *(float *)&Src[17] = v12 - 0.5f;
  *(_QWORD *)&Src[12] = (unsigned int)bmap.Width;
  Src[18] = -1090519040;
  Src[14] = 0;
  *(float *)&Src[19] = (float)((float)((float)bmap.Width * v14) + v12) - 0.5f;
  *(float *)&Src[20] = (float)((float)bmap.Height * v14) - 0.5f;
  *(float *)&Src[15] = (float)bmap.Width / (float)tex_width;
  *(float *)&Src[16] = (float)bmap.Height / (float)tex_height;
  if ( bmap.Width == tex_width && bmap.Height == tex_height )
  {
    Src[0] = this->Gepard->CreateTextureFromBitmap( filename, &bmap, 0);
  }
  else
  {
    new (&v9) SBitmap(tex_width, tex_height, bmap.Format, 0);
    memset(v9.Data, 0, v9.Size);
    v9.BitBlt(0, 0, bmap.Width, bmap.Height, &bmap, 0, 0);
    Src[0] = this->Gepard->CreateTextureFromBitmap( filename, &v9, 0);
    v9.~SBitmap();
  }
  p_Fonts = &this->Fonts;
  memset(&Src[4], 0, 16);
  v6 = this->Fonts.Add();
  memcpy(&p_Fonts->array[v6].data, Src, sizeof(p_Fonts->array[v6].data));
  bmap.~SBitmap();
  return v6;
}

//----- (0041E6D0) --------------------------------------------------------

int SBoard::LoadTrueTypeFont(int fontoverride, TypeFace typeface, FontEffect fontEffect, int fontsize, bool italic, bool bold)

{
  int v7;
  SHeap<SFontProp> *p_Fonts; // edi
  int v9;
  int v11;
  _DWORD Src[2316];
  v7 = fontoverride;
  p_Fonts = &this->Fonts;
  if ( fontoverride == -1 )
  {
    memset(&Src[2], 0, 0x2428u);
    Src[1] = 1;
    Src[0] = -1;
    v7 = p_Fonts->Add();
    memcpy(&p_Fonts->array[v7].data, Src, sizeof(p_Fonts->array[v7].data));
  }
  else if ( fontoverride >= 0 && fontoverride < this->Fonts.size && p_Fonts->array[fontoverride].use == 0x7FFFFFFF )
  {
    ++p_Fonts->array[fontoverride].data.RefCount;
  }
  else
  {
    memset(&Src[2], 0, 0x2428u);
    Src[1] = 1;
    Src[0] = -1;
    v11 = p_Fonts->Add();
    memcpy(&p_Fonts->array[v11].data, Src, sizeof(p_Fonts->array[v11].data));
    if ( v11 != fontoverride )
      Logger.g->Panic("SBoard::LoadTrueTypeFont: Bad font handle.");
  }
  v9 = v7;
  p_Fonts->array[v9].data.typeFace = typeface;
  p_Fonts->array[v9].data.fontEffect = fontEffect;
  p_Fonts->array[v9].data.fontSize = fontsize;
  p_Fonts->array[v9].data.Italic = italic;
  p_Fonts->array[v9].data.Bold = bold;
  return v7;
}

//----- (0041E810) --------------------------------------------------------

void SBoard::MoveFrame(int idx, int x, int y)

{
  int v4;
  v4 = idx;
  this->Frames.array[v4].data.X = (float)x;
  this->Frames.array[v4].data.Y = (float)y;
}

//----- (0041E840) --------------------------------------------------------

void SBoard::MoveFrameFloat(int idx, float x, float y)

{
  int v4;
  v4 = idx;
  this->Frames.array[v4].data.X = x;
  this->Frames.array[v4].data.Y = y;
}

//----- (0041E870) --------------------------------------------------------

void SBoard::ReleaseFont(int idx)

{
  SHeap<SFontProp>::__Tstruct *v3; // eax
  int v4;
  SHeap<SFontProp>::__Tstruct *array; // ecx
  if ( idx >= 0 && idx < this->Fonts.size && (v3 = this->Fonts.array, v4 = idx, v3[idx].use == 0x7FFFFFFF) )
  {
    // x64 fix: hard-pin system fonts 0-4 — never free the slot regardless of
    // RefCount drain. The IDA-decomp ReleaseFont/AddRef accounting on x64
    // ends up off-by-N on these slots (frame destructions release more refs
    // than were bumped — exact source not traced); commit e2be316 added a
    // +100 boot-time anchor but that proved insufficient (slot 0 still hit
    // the free list before reaching the briefing screen, then got recycled
    // for LoadSingleFont with typeFace=0, making SListBox::Create's
    // GetTextExtent(font=0,"") return 0x0 -> RowHeight=0 -> invisible text
    // in territory-properties / mission-description listboxes).
    //
    // Decrement RefCount for accounting, but clamp at 1 minimum so the slot
    // stays loaded for the app's lifetime. Language-switch path
    // (LoadSmallFonts firstLoading=0) re-runs LoadTrueTypeFont(0..4, ...)
    // which writes typeFace/fontEffect/fontSize/Italic/Bold even when the
    // slot is already in use, so the new language renders correctly; the
    // old TrueType HFONT cache is rebuilt lazily on next text draw.
    if ( idx <= 4 )
    {
      if ( v3[v4].data.RefCount > 1 )
        --v3[v4].data.RefCount;
      return;
    }
    if ( v3[v4].data.RefCount-- == 1 )
    {
      this->Gepard->ReleaseTexture(this->Fonts.array[v4].data.TextureIndex, 0);
      if ( idx >= this->Fonts.size || (array = this->Fonts.array, array[idx].use != 0x7FFFFFFF) )
        Logger.g->Panic("SHeap::Remove: invalid index (%d)", idx);
      array[idx].use = this->Fonts.nextempty;
      --this->Fonts.occupied;
      this->Fonts.nextempty = idx;
    }
  }
  else if ( idx != -1 )
  {
  }
}

// Simplified SBoard::Render — handles sprites, text, scalers, boxes, cursor
// The original is 1170 lines of SSE-heavy IDA output; this is a clean rewrite.
void SBoard::Render(float a2, int a3, int a4)
{
#ifdef HD_DEBUG_BOARD
  static int renderCount = 0;
  if (renderCount == 0) {
    Logger.g->Log(0, "SBoard::Render: lpD3DDev=%p Frames.size=%d root.Child=%d CursorFont=%d CursorGlyph=%d CursorVisible=%d HWCursor=%d ForceSW=%d",
      lpD3DDev, Frames.size, (Frames.size > 0 ? Frames.array[0].data.Child : -99), CursorFont, CursorGlyph, (int)CursorVisible, (int)HardwareCursor, (int)ForceSoftwareCursor);
    // Dump first few frames
    for (int i = 0; i < Frames.size && i < 10; i++) {
      if (Frames.array[i].use == 0x7FFFFFFF) {
        SFrame &ff = Frames.array[i].data;
        Logger.g->Log(0, "  Frame[%d] type=%d flags=%x parent=%d child=%d sibling=%d x=%.0f y=%.0f w=%d h=%d",
          i, ff.Type, ff.Flags, ff.Parent, ff.Child, ff.Sibling, ff.X, ff.Y, ff.Width, ff.Height);
      }
    }
  }
  renderCount++;
#endif
  if (!lpD3DDev || Frames.size <= 0 || !Frames.array)
    return;
  if (Gepard->Flags[1])
    return;

  // Set up 2D render states — match original sboard.c:1901-1907 plus safety resets
  lpD3DDev->SetVertexShader(NULL);
  lpD3DDev->SetPixelShader(NULL);
  lpD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
  lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
  lpD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  lpD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
  lpD3DDev->SetRenderState(D3DRS_FOGENABLE, FALSE);
  lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  lpD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  lpD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
  // 2D default = LINEAR (matches original sboard.c, which sets nothing and inherits
  // the 3D pass's LINEAR). Per-FT_SPRITE override below upgrades multi-glyph text
  // fonts to POINT when roundPixels is on, for crisper glyphs at integer-pixel UI.
  // Single-glyph "bitmap sprite" fonts (LoadSingleFont: splash, logos, panel art)
  // stay LINEAR so upscaled fullscreen art doesn't pixelate.
  DWORD currentFilter = D3DTEXF_LINEAR;
  lpD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, currentFilter);
  lpD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, currentFilter);
  lpD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  // Stage 0: texture * diffuse for both color and alpha
  lpD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  lpD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
  lpD3DDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
  lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
  // Clear extra texture stages left by 3D rendering (shadow map on stage 2, detail on stage 1)
  lpD3DDev->SetTexture(1, NULL);
  lpD3DDev->SetTexture(2, NULL);
  lpD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
  lpD3DDev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
  Gepard->SetLightingType(LT_PRELIT);
  // SetDrawType(DT_NORMAL) is critical — without it, SetTexture() uses stale
  // DrawType from 3D rendering (DT_ADD/DT_SHADOW), breaking all blend modes
  Gepard->SetDrawType(DT_NORMAL);
  lpD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1); // 0x144

  // Initialize vertex template
  for (int i = 0; i < 4; i++) {
    vert[i].z = 0.0f;
    vert[i].rhw = 1.0f;
    vert[i].color = 0xFFFFFFFF;
  }

  // Recursive frame tree traversal using a stack
  struct RenderState {
    int frameIdx;
    float absX, absY;
    float scaleX, scaleY;   // Panzers: HD 0x6c7150 pushes separate x/y scales
  };
  RenderState stack[64];
  int stackTop = 0;

  // Push root frame
  SFrame &root = Frames.array[0].data;
  stack[0].frameIdx = root.Child;
  stack[0].absX = root.X;
  stack[0].absY = root.Y;
  stack[0].scaleX = 1.0f;
  stack[0].scaleY = 1.0f;
  stackTop = 1;

  int lastTextureIdx = -2; // track to minimize texture switches

  while (stackTop > 0)
  {
    --stackTop;
    int idx = stack[stackTop].frameIdx;
    float parentX = stack[stackTop].absX;
    float parentY = stack[stackTop].absY;
    float scaleX = stack[stackTop].scaleX;
    float scaleY = stack[stackTop].scaleY;

    while (idx > 0 && idx < Frames.size)
    {
      if (Frames.array[idx].use != 0x7FFFFFFF)
        break;
      SFrame &f = Frames.array[idx].data;

      // Check visibility flag (bit 4 = 0x10 means visible)
      if (!(f.Flags & 0x10)) {
        idx = f.Sibling;
        continue;
      }

      float fx = parentX + f.X * scaleX;
      float fy = parentY + f.Y * scaleY;
      float curScaleX = scaleX;
      float curScaleY = scaleY;

      switch (f.Type)
      {
      case FT_SCALER:
        // HD 0x6c7150: a scaler with a virtual size (board +0x58)
        // scales x by Width/VirtualWidth and y by Height/VirtualHeight.
        if (f.Scaler.VirtualWidth > 0 && f.Scaler.VirtualHeight > 0) {
          curScaleX = scaleX * ((float)f.Width / (float)f.Scaler.VirtualWidth);
          curScaleY = scaleY * ((float)f.Height / (float)f.Scaler.VirtualHeight);
        } else {
          curScaleX = scaleX * f.Scaler.ScaleFactor;
          curScaleY = scaleY * f.Scaler.ScaleFactor;
        }
        break;

      case FT_SPRITE:
      {
        int fontIdx = f.Sprite.Font;
        if (fontIdx >= 0 && fontIdx < Fonts.size && Fonts.array[fontIdx].use == 0x7FFFFFFF)
        {
          SFontProp &fp = Fonts.array[fontIdx].data;
          int texIdx = fp.TextureIndex;
#ifdef HD_DEBUG_BOARD
          {
            static int spriteLog = 0;
            if (spriteLog < 10) {
              SGlyph &gl = f.Sprite.Glyph;
              Logger.g->Log(0, "Board SPRITE: idx=%d font=%d tex=%d pos=(%.0f,%.0f) dest=(%.1f,%.1f,%.1f,%.1f) parent=%d",
                idx, fontIdx, texIdx, fx, fy, gl.Dest.Left, gl.Dest.Top, gl.Dest.Right, gl.Dest.Bottom, f.Parent);
              spriteLog++;
            }
          }
#endif
          if (texIdx != lastTextureIdx) {
            Gepard->SetTexture(0, texIdx, 1);
            lastTextureIdx = texIdx;
          }

          // Multi-glyph fonts = text atlases → POINT (crisp glyphs) when roundPixels.
          // Single-glyph fonts = bitmap sprites (splash, logos) → stay LINEAR.
          DWORD wantFilter = (roundPixels && fp.LastGlyph > fp.FirstGlyph)
                             ? D3DTEXF_POINT : D3DTEXF_LINEAR;
          if (wantFilter != currentFilter) {
            lpD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, wantFilter);
            lpD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, wantFilter);
            currentFilter = wantFilter;
          }

          SGlyph &g = f.Sprite.Glyph;
          float dx = fx + g.Dest.Left * scaleX;
          float dy = fy + g.Dest.Top * scaleY;
          float dw = (g.Dest.Right - g.Dest.Left) * scaleX;
          float dh = (g.Dest.Bottom - g.Dest.Top) * scaleY;

          // Ghidra sboard.c:1175 — snap sprite origin to int when roundPixels is on.
          // Intro crawl sets roundPixels=0 to keep sub-pixel scroll smooth across lines.
          if (roundPixels) {
            dx = (float)(int)dx;
            dy = (float)(int)dy;
          }

          vert[0].x = dx;      vert[0].y = dy;      vert[0].u = g.Src.Left;  vert[0].v = g.Src.Top;
          vert[1].x = dx + dw; vert[1].y = dy;      vert[1].u = g.Src.Right; vert[1].v = g.Src.Top;
          vert[2].x = dx;      vert[2].y = dy + dh; vert[2].u = g.Src.Left;  vert[2].v = g.Src.Bottom;
          vert[3].x = dx + dw; vert[3].y = dy + dh; vert[3].u = g.Src.Right; vert[3].v = g.Src.Bottom;

          // Apply sepia/tint filter if set (SepiaColor: 0=no tint, >0=tint with color)
          bool useSepia = (f.Sprite.SepiaColor != 0);
          if (useSepia)
          {
            unsigned int sc = f.Sprite.SepiaColor | 0xFF000000;
            for (int v = 0; v < 4; v++)
              vert[v].color = sc;
            SGepard *gep = (SGepard *)Gepard;
            if (gep->sepiaPixelShader.Ptr)
              lpD3DDev->SetPixelShader(gep->sepiaPixelShader.Ptr);
          }
          else
          {
            for (int v = 0; v < 4; v++)
              vert[v].color = 0xFFFFFFFF;
          }

          lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));

          if (useSepia)
            lpD3DDev->SetPixelShader(NULL);
        }
        break;
      }

      case FT_MINIMAP:
      {
        int fontIdx = f.Minimap.Font;
        if (fontIdx >= 0 && fontIdx < Fonts.size && Fonts.array[fontIdx].use == 0x7FFFFFFF)
        {
          int texIdx = Fonts.array[fontIdx].data.TextureIndex;
          if (texIdx != lastTextureIdx) {
            Gepard->SetTexture(0, texIdx, 1);
            lastTextureIdx = texIdx;
          }

          // Use BORDER addressing so rotated minimap shows black outside bounds
          lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
          lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
          lpD3DDev->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);

          SGlyph &g = f.Minimap.Glyph;
          float dx = fx + g.Dest.Left * scaleX;
          float dy = fy + g.Dest.Top * scaleY;
          float dw = (g.Dest.Right - g.Dest.Left) * scaleX;
          float dh = (g.Dest.Bottom - g.Dest.Top) * scaleY;

          // Vertex positions
          vert[0].x = dx;      vert[0].y = dy;
          vert[1].x = dx + dw; vert[1].y = dy;
          vert[2].x = dx;      vert[2].y = dy + dh;
          vert[3].x = dx + dw; vert[3].y = dy + dh;

          // Rotated UV coordinates around center (0.5, 0.5)
          float rotation = f.Minimap.Rotation;
          float sinR = sinf(rotation);
          float cosR = cosf(rotation);
          float halfCos = cosR * 0.5f;
          float halfSin = sinR * 0.5f;

          // UV corners rotated around (0.5, 0.5) — from sboard.c:2186-2203
          // Our vertex order: 0=TL, 1=TR, 2=BL, 3=BR
          vert[0].u = 0.5f - halfCos + halfSin; vert[0].v = 0.5f - halfCos - halfSin; // TL
          vert[1].u = 0.5f + halfCos + halfSin; vert[1].v = 0.5f - halfCos + halfSin; // TR
          vert[2].u = 0.5f - halfCos - halfSin; vert[2].v = 0.5f + halfCos - halfSin; // BL
          vert[3].u = 0.5f + halfCos - halfSin; vert[3].v = 0.5f + halfCos + halfSin; // BR

          for (int v = 0; v < 4; v++)
            vert[v].color = 0xFFFFFFFF;

          lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));

          // Restore sampler to CLAMP
          lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
          lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

          // Draw camera view boundary lines on minimap (sboard.c:2207-2360)
          Gepard->SetTexture(0, -1, 1);
          lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
          lpD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
          lpD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

          D3DXVECTOR2 viewBounds[4];
          ((SGepard *)Gepard)->GetViewBoundaries(viewBounds);

          int fw = f.Width;
          int fh = f.Height;

          // Transform view boundary corners: rotate by minimap rotation, scale to pixel coords
          float cx[4], cy[4];
          for (int i = 0; i < 4; i++) {
            float bx = viewBounds[i].x - 0.5f;
            float by = viewBounds[i].y - 0.5f;
            float rx = (cosR * bx - sinR * by + 0.5f) * (float)fw;
            float ry = (0.5f - (cosR * by + sinR * bx)) * (float)fh;
            cx[i] = rx;
            cy[i] = ry;
          }

          // Draw 4 edges of the view frustum quadrilateral
          for (int i = 0; i < 4; i++) {
            int j = (i + 1) & 3;
            float x0 = cx[i], y0 = cy[i];
            float x1 = cx[j], y1 = cy[j];
            float maxX = (float)(fw - 1);
            float maxY = (float)(fh - 1);

            if (x0 < 0.0f && x1 < 0.0f) continue;
            if (x0 > maxX && x1 > maxX) continue;
            if (y0 < 0.0f && y1 < 0.0f) continue;
            if (y0 > maxY && y1 > maxY) continue;

            if (x0 < 0.0f) { y0 = y0 + (y1 - y0) * (-x0) / (x1 - x0); x0 = 0.0f; }
            if (x1 < 0.0f) { y1 = y1 + (y0 - y1) * (-x1) / (x0 - x1); x1 = 0.0f; }
            if (x0 > maxX) { y0 = y0 + (y1 - y0) * (x0 - maxX) / (x0 - x1); x0 = maxX; }
            if (x1 > maxX) { y1 = y1 + (y0 - y1) * (x1 - maxX) / (x1 - x0); x1 = maxX; }
            if (y0 < 0.0f) { x0 = x0 + (x1 - x0) * (-y0) / (y1 - y0); y0 = 0.0f; }
            if (y1 < 0.0f) { x1 = x1 + (x0 - x1) * (-y1) / (y0 - y1); y1 = 0.0f; }
            if (y0 > maxY) { x0 = x0 + (x1 - x0) * (y0 - maxY) / (y0 - y1); y0 = maxY; }
            if (y1 > maxY) { x1 = x1 + (x0 - x1) * (y1 - maxY) / (y1 - y0); y1 = maxY; }

            if ((x0 < 0.0f && x1 < 0.0f) || (x0 > maxX && x1 > maxX)) continue;
            if ((y0 < 0.0f && y1 < 0.0f) || (y0 > maxY && y1 > maxY)) continue;

            vert[0].x = x0 * scaleX + fx; vert[0].y = y0 * scaleY + fy;
            vert[0].u = 0; vert[0].v = 0; vert[0].color = 0xC0FFFFC0;
            vert[1].x = x1 * scaleX + fx; vert[1].y = y1 * scaleY + fy;
            vert[1].u = 0; vert[1].v = 0; vert[1].color = 0xC0FFFFC0;

            lpD3DDev->DrawPrimitiveUP(D3DPT_LINELIST, 1, vert, sizeof(TLVertBoard));
          }

          lastTextureIdx = -2;
        }
        break;
      }

      case FT_SPRITE_9SLICE:
      {
        int fontIdx = f.Sprite.Font;
        if (fontIdx >= 0 && fontIdx < Fonts.size && Fonts.array[fontIdx].use == 0x7FFFFFFF)
        {
          int texIdx = Fonts.array[fontIdx].data.TextureIndex;
          if (texIdx != lastTextureIdx) {
            Gepard->SetTexture(0, texIdx, 1);
            lastTextureIdx = texIdx;
          }

          SGlyph &g = f.Sprite.Glyph;
          // Glyph dest size defines the corner dimensions
          float cornerW = (g.Dest.Right - g.Dest.Left) * scaleX * 0.5f;
          float cornerH = (g.Dest.Bottom - g.Dest.Top) * scaleY * 0.5f;
          // Frame width/height define the total fill area
          float totalW = (float)f.Width * scaleX;
          float totalH = (float)f.Height * scaleY;
          float x0 = fx - 0.5f;
          float y0 = fy - 0.5f;

          // UV midpoints
          float uMid = (g.Src.Left + g.Src.Right) * 0.5f;
          float vMid = (g.Src.Top + g.Src.Bottom) * 0.5f;

          // Build 4x4 grid: X positions, Y positions, U coords, V coords
          float xpos[4] = { x0, x0 + cornerW, x0 + totalW - cornerW, x0 + totalW };
          float ypos[4] = { y0, y0 + cornerH, y0 + totalH - cornerH, y0 + totalH };
          float ucoord[4] = { g.Src.Left, uMid, uMid, g.Src.Right };
          float vcoord[4] = { g.Src.Top, vMid, vMid, g.Src.Bottom };

          // Build 16 vertices (4x4 grid)
          TLVertBoard gridVerts[16];
          for (int gy = 0; gy < 4; gy++) {
            for (int gx = 0; gx < 4; gx++) {
              TLVertBoard &vt = gridVerts[gy * 4 + gx];
              vt.x = xpos[gx];
              vt.y = ypos[gy];
              vt.z = 0.0f;
              vt.rhw = 1.0f;
              vt.color = 0xFFFFFFFF;
              vt.u = ucoord[gx];
              vt.v = vcoord[gy];
            }
          }

          // Build 54 indices (9 quads × 2 triangles × 3 vertices)
          short gridIndices[54];
          int ii = 0;
          for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
              short tl = (short)(row * 4 + col);
              short tr = tl + 1;
              short bl = tl + 4;
              short br = bl + 1;
              gridIndices[ii++] = tl;
              gridIndices[ii++] = tr;
              gridIndices[ii++] = bl;
              gridIndices[ii++] = tr;
              gridIndices[ii++] = br;
              gridIndices[ii++] = bl;
            }
          }

          lpD3DDev->DrawIndexedPrimitiveUP(
            D3DPT_TRIANGLELIST, 0, 16, 18,
            gridIndices, D3DFMT_INDEX16,
            gridVerts, sizeof(TLVertBoard));
        }
        break;
      }

      case FT_TEXT:
      case FT_FIXTEXT:
      {
        // Check for TrueType pre-rendered text first
        if (f.Text.TrueTypeTextureIndex >= 0)
        {
          // TrueType text: render as single quad with pre-rendered texture
#ifdef HD_DEBUG_BOARD
          {
            static int ttLog = 0;
            if (ttLog < 10) {
              Logger.g->Log(0, "Board TRUETYPE: idx=%d ttTex=%d color=0x%08X w=%d h=%d xy=(%d,%d)",
                idx, f.Text.TrueTypeTextureIndex, f.Text.Color,
                f.Text.TrueTypeWidth, f.Text.TrueTypeHeight,
                f.Text.TrueTypeX, f.Text.TrueTypeY);
              ttLog++;
            }
          }
#endif
          int ttTexIdx = f.Text.TrueTypeTextureIndex;
          if (ttTexIdx != lastTextureIdx) {
            Gepard->SetTexture(0, ttTexIdx, 1);
            lastTextureIdx = ttTexIdx;
          }

          DWORD wantFilter = roundPixels ? D3DTEXF_POINT : D3DTEXF_LINEAR;
          if (wantFilter != currentFilter) {
            lpD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, wantFilter);
            lpD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, wantFilter);
            currentFilter = wantFilter;
          }

          // Handle text alignment
          float tx = fx;
          int align = f.Text.Align;
          if (align == 1)       // right-aligned
            tx -= (float)f.Text.TrueTypeWidth;
          else if (align == 2)  // center-aligned
            tx -= (float)(f.Text.TrueTypeWidth / 2);

          float ttx = tx + (float)f.Text.TrueTypeX;
          float tty = fy + (float)f.Text.TrueTypeY;
          float ttw = (float)f.Text.TrueTypeWidth;
          float tth = (float)f.Text.TrueTypeHeight;
          unsigned int col = f.Text.Color | 0xFF000000;

          // Ghidra: snap text origin to int when roundPixels is on (sharp UI text).
          // Intro disables rounding for smooth sub-pixel crawl.
          if (roundPixels) {
            ttx = (float)(int)ttx;
            tty = (float)(int)tty;
          }

          vert[0].x = ttx - 0.5f;       vert[0].y = tty - 0.5f;       vert[0].u = 0.0f; vert[0].v = 0.0f; vert[0].color = col;
          vert[1].x = ttx + ttw - 0.5f; vert[1].y = tty - 0.5f;       vert[1].u = 1.0f; vert[1].v = 0.0f; vert[1].color = col;
          vert[2].x = ttx - 0.5f;       vert[2].y = tty + tth - 0.5f; vert[2].u = 0.0f; vert[2].v = 1.0f; vert[2].color = col;
          vert[3].x = ttx + ttw - 0.5f; vert[3].y = tty + tth - 0.5f; vert[3].u = 1.0f; vert[3].v = 1.0f; vert[3].color = col;

          lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));
          break;
        }

        // Bitmap font: render character by character
        int fontIdx = f.Text.Font;
        const char *text = f.Text.ReducedText ? f.Text.ReducedText : f.Text.Text;
        if (!text || !*text || fontIdx < 0 || fontIdx >= Fonts.size || Fonts.array[fontIdx].use != 0x7FFFFFFF)
          break;

        SFontProp &font = Fonts.array[fontIdx].data;
        int texIdx = font.TextureIndex;
        if (texIdx != lastTextureIdx) {
          Gepard->SetTexture(0, texIdx, 1);
          lastTextureIdx = texIdx;
        }

        {
          DWORD wantFilter = (roundPixels && font.LastGlyph > font.FirstGlyph)
                             ? D3DTEXF_POINT : D3DTEXF_LINEAR;
          if (wantFilter != currentFilter) {
            lpD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, wantFilter);
            lpD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, wantFilter);
            currentFilter = wantFilter;
          }
        }

        float tx = fx;
        float ty = fy;
        unsigned int col = f.Text.Color | 0xFF000000;

        // Handle text alignment for bitmap fonts
        int align = f.Text.Align;
        // HD 0x6c7150: the alignment offset is scaled by the x scale.
        if (align == 1)       // right-aligned
          tx -= (float)f.Text.Width * scaleX;
        else if (align == 2)  // center-aligned
          tx -= (float)(f.Text.Width / 2) * scaleX;

        for (const unsigned char *p = (const unsigned char *)text; *p; )
        {
          // Decode UTF-8 to Unicode codepoint (matching GetTextExtent logic)
          unsigned int codepoint;
          unsigned char b0 = *p++;
          if (b0 <= 0xC0) {
            codepoint = b0;
          } else if (b0 <= 0xDF) {
            // 2-byte: 0xC1-0xDF
            if (p[0] >= 0x80 && p[0] <= 0xBF) {
              codepoint = ((b0 & 0x1F) << 6) | (p[0] & 0x3F);
              p += 1;
            } else {
              codepoint = b0;
            }
          } else if (b0 <= 0xEF) {
            // 3-byte: 0xE0-0xEF
            if (p[0] >= 0x80 && p[0] <= 0xBF && p[1] >= 0x80 && p[1] <= 0xBF) {
              codepoint = ((b0 & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
              p += 2;
            } else {
              codepoint = b0;
            }
          } else if (b0 <= 0xF7) {
            // 4-byte: 0xF0-0xF7
            if (p[0] >= 0x80 && p[0] <= 0xBF && p[1] >= 0x80 && p[1] <= 0xBF && p[2] >= 0x80 && p[2] <= 0xBF) {
              codepoint = ((b0 & 0x07) << 18) | ((p[0] & 0x3F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
              p += 3;
            } else {
              codepoint = b0;
            }
          } else {
            codepoint = b0;
          }

          // Map Unicode codepoint to glyph index via UnicodeDecodeTable
          int ch;
          if (codepoint >= 0x460)
            ch = 63; // '?' replacement
          else
            ch = UnicodeDecodeTable[codepoint];

          if ((unsigned)ch >= 256)
            continue;

          SGlyph &g = font.Glyphs[ch];
          if (g.Width == 0)
            continue;

          float dx = tx + g.Dest.Left * scaleX;
          float dy = ty + g.Dest.Top * scaleY;
          float dw = (g.Dest.Right - g.Dest.Left) * scaleX;
          float dh = (g.Dest.Bottom - g.Dest.Top) * scaleY;

          // Ghidra: snap per-glyph quad to integer pixels when roundPixels is on.
          if (roundPixels) {
            dx = (float)(int)dx;
            dy = (float)(int)dy;
          }

          vert[0].x = dx;      vert[0].y = dy;      vert[0].u = g.Src.Left;  vert[0].v = g.Src.Top;    vert[0].color = col;
          vert[1].x = dx + dw; vert[1].y = dy;      vert[1].u = g.Src.Right; vert[1].v = g.Src.Top;    vert[1].color = col;
          vert[2].x = dx;      vert[2].y = dy + dh; vert[2].u = g.Src.Left;  vert[2].v = g.Src.Bottom; vert[2].color = col;
          vert[3].x = dx + dw; vert[3].y = dy + dh; vert[3].u = g.Src.Right; vert[3].v = g.Src.Bottom; vert[3].color = col;

          lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));
          tx += g.Width * scaleX;
        }
        break;
      }

      case FT_BOX:
      {
        Gepard->SetTexture(0, -1, 1);
        lastTextureIdx = -2;
        lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        lpD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        lpD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

        float bw = (float)f.Width * scaleX;
        float bh = (float)f.Height * scaleY;
        unsigned int col = f.Box.Color;

        // Ghidra: box vertices snap to integer pixels when roundPixels is on.
        // Intro crawl disables rounding so row/background boxes ride the parent smoothly.
        float bx = fx;
        float by = fy;
        if (roundPixels) {
          bx = (float)(int)bx;
          by = (float)(int)by;
        }

        vert[0].x = bx;      vert[0].y = by;      vert[0].u = 0; vert[0].v = 0; vert[0].color = col;
        vert[1].x = bx + bw; vert[1].y = by;      vert[1].u = 0; vert[1].v = 0; vert[1].color = col;
        vert[2].x = bx;      vert[2].y = by + bh; vert[2].u = 0; vert[2].v = 0; vert[2].color = col;
        vert[3].x = bx + bw; vert[3].y = by + bh; vert[3].u = 0; vert[3].v = 0; vert[3].color = col;

        lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));
        break;
      }

      case FT_ANIM:
      {
          SAnimation *anim = f.Anim.Anim;
        if (!anim)
        {
#ifdef HD_HEADQUARTERS_3DMODEL
          // External render target texture (no SAnimation) — just draw the quad
          if (f.Anim.TextureIndex >= 0)
          {
            Gepard->SetTexture(0, f.Anim.TextureIndex, 1);
            lastTextureIdx = -1;
            float x0 = fx - 0.5f;
            float y0 = fy - 0.5f;
            float x1 = x0 + (float)f.Width * curScaleX;
            float y1 = y0 + (float)f.Height * curScaleY;
            float u1 = f.Sprite.Glyph.Dest.Left;
            float v1 = f.Sprite.Glyph.Dest.Top;
            vert[0].x = x0;  vert[0].y = y0;  vert[0].u = 0;   vert[0].v = 0;   vert[0].color = 0xFFFFFFFF;
            vert[1].x = x1;  vert[1].y = y0;  vert[1].u = u1;  vert[1].v = 0;   vert[1].color = 0xFFFFFFFF;
            vert[2].x = x0;  vert[2].y = y1;  vert[2].u = 0;   vert[2].v = v1;  vert[2].color = 0xFFFFFFFF;
            vert[3].x = x1;  vert[3].y = y1;  vert[3].u = u1;  vert[3].v = v1;  vert[3].color = 0xFFFFFFFF;
            lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));
          }
#endif
          break;
        }
        // Check end time
        if (f.Anim.EndTime && Gepard->AnimTime > f.Anim.EndTime)
        {
          this->StopAnim(idx);
          break;
        }
        // Advance frames while time allows
        // LastFrame is aliased as Text.Align, FrameTime as Anim.FrameTime
        unsigned int frameTime = f.Anim.FrameTime;
        // Use real SFrameAnim fields (not Text.Width/Text.Align x86-alias tricks
        // that collide with other SFrameAnim slots on x64 due to pointer growth).
        bool looping = (f.Anim.Flags & 1) != 0;
        while (Gepard->AnimTime > (unsigned int)f.Anim.LastFrame + frameTime)
        {
          f.Anim.LastFrame += (int)f.Anim.FrameTime;
          if (!anim->NextFrame(looping))
          {
            this->StopAnim(idx);
            anim = 0;
            break;
          }
          if (Gepard->AnimTime <= (unsigned int)f.Anim.LastFrame + frameTime)
          {
            // Frame advanced, update texture
            Gepard->UpdateTextureFromBitmap(f.Anim.TextureIndex, anim);
          }
        }
        if (!anim)
          break;
        // Render the animation quad
        {
          Gepard->SetTexture(0, f.Anim.TextureIndex, 1);
          lastTextureIdx = -1;
          float x0 = fx - 0.5f;
          float y0 = fy - 0.5f;
          float x1 = x0 + (float)f.Width * curScaleX;
          float y1 = y0 + (float)f.Height * curScaleY;
          // UV max values stored in Anim.MaxU/MaxV by StartAnim
          float u1 = f.Anim.MaxU;
          float v1 = f.Anim.MaxV;
          vert[0].x = x0;  vert[0].y = y0;  vert[0].u = 0;   vert[0].v = 0;   vert[0].color = 0xFFFFFFFF;
          vert[1].x = x1;  vert[1].y = y0;  vert[1].u = u1;  vert[1].v = 0;   vert[1].color = 0xFFFFFFFF;
          vert[2].x = x0;  vert[2].y = y1;  vert[2].u = 0;   vert[2].v = v1;  vert[2].color = 0xFFFFFFFF;
          vert[3].x = x1;  vert[3].y = y1;  vert[3].u = u1;  vert[3].v = v1;  vert[3].color = 0xFFFFFFFF;
          lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));
        }
        break;
      }

      default:
        break;
      }

      // Push sibling for later processing
      if (f.Sibling > 0 && stackTop < 63) {
        stack[stackTop].frameIdx = f.Sibling;
        stack[stackTop].absX = parentX;
        stack[stackTop].absY = parentY;
        stack[stackTop].scaleX = scaleX;
        stack[stackTop].scaleY = scaleY;
        stackTop++;
      }

      // Descend into children
      if (f.Child > 0) {
        parentX = fx;
        parentY = fy;
        scaleX = curScaleX;
        scaleY = curScaleY;
        idx = f.Child;
      } else {
        idx = 0; // no children, loop will exit and pop from stack
      }
    }
  }

  // Draw software cursor
  // HD 0x6ca240 also requires 0 <= glyph < NumCursors (glyph -1 = hidden).
  if (CursorVisible && (!HardwareCursor || ForceSoftwareCursor) && CursorFont >= 0
      && CursorGlyph >= 0 && CursorGlyph < NumCursors)
  {
    int fontIdx = CursorFont;
    if (fontIdx >= 0 && fontIdx < Fonts.size && Fonts.array[fontIdx].use == 0x7FFFFFFF)
    {
      int texIdx = Fonts.array[fontIdx].data.TextureIndex;
      Gepard->SetTexture(0, texIdx, 1);

      SGlyph &g = Fonts.array[fontIdx].data.Glyphs[CursorGlyph];
      float cx = (float)CursorX + g.Dest.Left;
      float cy = (float)CursorY + g.Dest.Top;
      float cw = g.Dest.Right - g.Dest.Left;
      float ch = g.Dest.Bottom - g.Dest.Top;

      vert[0].x = cx;      vert[0].y = cy;      vert[0].u = g.Src.Left;  vert[0].v = g.Src.Top;    vert[0].color = 0xFFFFFFFF;
      vert[1].x = cx + cw; vert[1].y = cy;      vert[1].u = g.Src.Right; vert[1].v = g.Src.Top;    vert[1].color = 0xFFFFFFFF;
      vert[2].x = cx;      vert[2].y = cy + ch; vert[2].u = g.Src.Left;  vert[2].v = g.Src.Bottom; vert[2].color = 0xFFFFFFFF;
      vert[3].x = cx + cw; vert[3].y = cy + ch; vert[3].u = g.Src.Right; vert[3].v = g.Src.Bottom; vert[3].color = 0xFFFFFFFF;

      lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertBoard));
    }
  }

  // Restore render states
  lpD3DDev->SetRenderState(D3DRS_ZENABLE, TRUE);
  lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}

// SBoard::RerenderText — TrueType text rendering with FontBitmap effects
// Decompiled from: gameSplit/sboard.c lines 2955-3571
void SBoard::RerenderText(int idx) {
  SFrame &frame = this->Frames.array[idx].data;
  int fontIdx = frame.Text.Font;
  if (fontIdx < 0)
    return;

  SFontProp &fontProp = this->Fonts.array[fontIdx].data;

  // If typeFace is None, release existing texture and bail
  if (fontProp.typeFace == TF_None) {
    if (frame.Text.TrueTypeTextureIndex >= 0) {
      this->Gepard->ReleaseTexture(frame.Text.TrueTypeTextureIndex, false);
      frame.Text.TrueTypeTextureIndex = -1;
    }
    return;
  }

  // Get effective scale factor (walks parent chain for scaler frames)
  float scaleFactor = this->GetTextEffectiveScaleFactor(idx);

  // Create GDI DC for text measurement and rendering
  HDC hdc = CreateCompatibleDC(0);
  if (!hdc)
    return;

  // Get HFONT from font properties
  bool doubleMode = false;
  HFONT hFont = this->GetHFont(&fontProp, &doubleMode, scaleFactor);
  if (!hFont) {
    DeleteDC(hdc);
    return;
  }
  HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

  // Get the text string — use ReducedText if available, else Text
  const char *text = frame.Text.ReducedText ? frame.Text.ReducedText : frame.Text.Text;
  if (!text || !text[0]) {
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
    DeleteDC(hdc);
    if (frame.Text.TrueTypeTextureIndex >= 0) {
      this->Gepard->ReleaseTexture(frame.Text.TrueTypeTextureIndex, false);
      frame.Text.TrueTypeTextureIndex = -1;
    }
    return;
  }

  // Convert UTF-8 to wide string using manual decoder matching original game
  // (raw bytes 0x80-0xC0 and 0xF8-0xFF pass through as Latin-1 codepoints,
  //  unlike MultiByteToWideChar(CP_UTF8) which rejects them as invalid)
  int textLen = (int)strlen(text);
  const unsigned char *src = (const unsigned char *)text;
  const unsigned char *srcEnd = src + textLen;

  // First pass: count wide chars needed
  int wideStrLen = 0;
  {
    const unsigned char *s = src;
    while (s < srcEnd) {
      unsigned int cp = *s++;
      if (cp - 0xC1 < 0x1F) {
        if (s + 1 <= srcEnd && s[0] >= 0x80 && s[0] <= 0xBF) {
          cp = ((cp & 0x1F) << 6) | (s[0] & 0x3F);
          s += 1;
        }
      } else if (cp - 0xE1 < 0x0F) {
        if (s + 2 <= srcEnd && s[0] >= 0x80 && s[0] <= 0xBF && (unsigned char)(s[1] + 0x80) < 0x40) {
          cp = ((cp & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F);
          s += 2;
        }
      } else if (cp - 0xF1 < 7) {
        if (s + 3 <= srcEnd && s[0] >= 0x80 && s[0] <= 0xBF && (unsigned char)(s[1] + 0x80) < 0x40 && (unsigned char)(s[2] + 0x80) < 0x40) {
          cp = ((cp & 0x07) << 18) | ((s[0] & 0x3F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
          s += 3;
        }
      }
      wideStrLen += (cp >= 0x10000) ? 2 : 1;
    }
  }

  if (wideStrLen <= 0) {
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
    DeleteDC(hdc);
    return;
  }

  // Second pass: produce wide string
  wchar_t *wideText = new wchar_t[wideStrLen + 1];
  {
    const unsigned char *s = src;
    wchar_t *dst = wideText;
    while (s < srcEnd) {
      unsigned int cp = *s++;
      if (cp - 0xC1 < 0x1F) {
        if (s + 1 <= srcEnd && s[0] >= 0x80 && s[0] <= 0xBF) {
          cp = ((cp & 0x1F) << 6) | (s[0] & 0x3F);
          s += 1;
        }
      } else if (cp - 0xE1 < 0x0F) {
        if (s + 2 <= srcEnd && s[0] >= 0x80 && s[0] <= 0xBF && (unsigned char)(s[1] + 0x80) < 0x40) {
          cp = ((cp & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F);
          s += 2;
        }
      } else if (cp - 0xF1 < 7) {
        if (s + 3 <= srcEnd && s[0] >= 0x80 && s[0] <= 0xBF && (unsigned char)(s[1] + 0x80) < 0x40 && (unsigned char)(s[2] + 0x80) < 0x40) {
          cp = ((cp & 0x07) << 18) | ((s[0] & 0x3F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
          s += 3;
        }
      }
      if (cp >= 0x10000) {
        *dst++ = (wchar_t)(((cp - 0x10000) >> 10) + 0xD800);
        *dst++ = (wchar_t)(((cp - 0x10000) & 0x3FF) + 0xDC00);
      } else {
        *dst++ = (wchar_t)cp;
      }
    }
    *dst = 0;
  }

  // Measure text
  RECT rc = {0, 0, 0, 0};
  DrawTextW(hdc, wideText, wideStrLen, &rc, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);

  int textW = rc.right - rc.left;
  int textH = rc.bottom - rc.top;
  if (textW <= 0 || textH <= 0) {
    delete[] wideText;
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
    DeleteDC(hdc);
    if (frame.Text.TrueTypeTextureIndex >= 0) {
      this->Gepard->ReleaseTexture(frame.Text.TrueTypeTextureIndex, false);
      frame.Text.TrueTypeTextureIndex = -1;
    }
    return;
  }

  // Handle doubleMode rounding (original: round up to even)
  if (doubleMode) {
    textW = textW + (textW & 1);
    textH = textH + (textH & 1);
  }

  // Create DIB section for rendering
  BITMAPINFO bmi;
  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = textW;
  bmi.bmiHeader.biHeight = textH; // bottom-up (matches original)
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void *dibBits = NULL;
  HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &dibBits, NULL, 0);
  if (!hBitmap || !dibBits) {
    delete[] wideText;
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
    DeleteDC(hdc);
    return;
  }

  HBITMAP oldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);

  // Clear to black and set background
  SetBkColor(hdc, 0);

  // Text color depends on typeface and effect (original lines 3294-3305)
  TypeFace tf = fontProp.typeFace;
  FontEffect fe = fontProp.fontEffect;
  if (tf == Small || tf == Arial || tf == ChineseSmall || fe == TF_None || fe == (FontEffect)4)
    ::SetTextColor(hdc, 0x00FFFFFFu);  // pure white
  else
    ::SetTextColor(hdc, 0x00C0C0C0u);  // light gray for Shadow/Outline/Glow on race fonts

  RECT drawRect = {0, 0, textW, textH};
  DrawTextW(hdc, wideText, wideStrLen, &drawRect, DT_NOPREFIX | DT_SINGLELINE);
  GdiFlush();

  delete[] wideText;

  // Extract grayscale glyph into FontBitmap (bottom-up DIB → top-down)
  FontBitmap glyph(textW, textH);
  unsigned char *srcBytes = (unsigned char *)dibBits;
  int dstIdx = 0;
  for (int y = textH - 1; y >= 0; y--) {
    unsigned char *row = srcBytes + y * textW * 4;
    for (int x = 0; x < textW; x++) {
      unsigned char b = row[x * 4];
      unsigned char g = row[x * 4 + 1];
      unsigned char r = row[x * 4 + 2];
      unsigned char v = r;
      if (g > v) v = g;
      if (b > v) v = b;
      glyph.Data[dstIdx++] = v;
    }
  }

  // DoubleMode: downscale 2x
  int glyphW = textW, glyphH = textH;
  FontBitmap reduced;
  FontBitmap *activeGlyph = &glyph;
  if (doubleMode) {
    glyphW = textW / 2;
    glyphH = textH / 2;
    reduced.Width = glyphW;
    reduced.Height = glyphH;
    reduced.X = 0;
    reduced.Y = 0;
    reduced.Data = new unsigned char[glyphW * glyphH];
    for (int y = 0; y < glyphH; y++) {
      for (int x = 0; x < glyphW; x++) {
        int sx = x * 2, sy = y * 2;
        unsigned int sum = (unsigned int)glyph.Data[sy * textW + sx]
                         + (unsigned int)glyph.Data[sy * textW + sx + 1]
                         + (unsigned int)glyph.Data[(sy + 1) * textW + sx]
                         + (unsigned int)glyph.Data[(sy + 1) * textW + sx + 1]
                         + 2;
        reduced.Data[y * glyphW + x] = (unsigned char)(sum >> 2);
      }
    }
    activeGlyph = &reduced;
  }

  // Compute sigma for effects
  float sigmaScale = 1.0f;
  if (tf == Small || tf == ChineseSmall)
    sigmaScale = 0.5f;
  float sigma = sigmaScale * scaleFactor;

  // Apply font effect
  SBitmap bmap32;
  int effectX = 0, effectY = 0;

  switch ((int)fe) {
    case 0: {
      // Normal — direct grayscale to ARGB
      new (&bmap32) SBitmap(glyphW, glyphH, D3DFMT_A8R8G8B8, 0);
      unsigned int *dst = (unsigned int *)bmap32.Data;
      for (int i = 0; i < glyphW * glyphH; i++)
        dst[i] = ((unsigned int)activeGlyph->Data[i] << 24) | 0x00FFFFFF;
      effectX = 0;
      effectY = 0;
      break;
    }

    case 1: {
      // Shadow — blur then combine light over shadow
      FontBitmap blurred;
      activeGlyph->Blur(&blurred, sigma, 1.0f);
      int shadowOffset = (int)(sigma * -4.0f);
      int shift = blurred.X - shadowOffset;
      FontBitmap::CombineLightAndShadow(&bmap32, activeGlyph, &blurred, shift);
      effectX = 0;
      effectY = 0;
      break;
    }

    case 2: {
      // Outline — blur+add, second blur, combine
      FontBitmap blurred1;
      activeGlyph->Blur(&blurred1, sigma * 1.5f, 0.8f);
      blurred1.Add(activeGlyph, -blurred1.X);
      FontBitmap blurred2;
      activeGlyph->Blur(&blurred2, sigma, 1.0f);
      int shadowOffset = (int)(sigma * -4.0f);
      int shift = blurred2.X - shadowOffset - blurred1.X;
      SBitmap combined;
      FontBitmap::CombineLightAndShadow(&combined, &blurred1, &blurred2, shift);
      bmap32 = &combined;
      effectX = blurred1.X;
      effectY = blurred1.Y;
      break;
    }

    case 3: {
      // Glow — blur+add, second blur, combine with extra offset
      FontBitmap blurred1;
      activeGlyph->Blur(&blurred1, sigma * 1.5f, 0.8f);
      int savedX = blurred1.X;
      blurred1.Add(activeGlyph, -blurred1.X);
      FontBitmap blurred2;
      activeGlyph->Blur(&blurred2, sigma, 1.0f);
      int doubleSigma = (int)(sigma + sigma);
      int shadowOffset = (int)(sigma * -4.0f);
      int shift = blurred2.X - shadowOffset - doubleSigma - savedX;
      SBitmap combined;
      FontBitmap::CombineLightAndShadow(&combined, &blurred1, &blurred2, shift);
      bmap32 = &combined;
      effectX = doubleSigma + savedX;
      effectY = doubleSigma + blurred1.Y;
      break;
    }

    case 4: {
      // Emboss — box blur then combine emboss
      FontBitmap boxBlurred;
      activeGlyph->BoxBlur(&boxBlurred, (int)(sigma * 1.5f));
      SBitmap combined;
      FontBitmap::CombineEmboss(&combined, activeGlyph, &boxBlurred,
                                 (float)(int)(sigma * 1.5f) * 0.7f);
      bmap32 = &combined;
      effectX = boxBlurred.X;
      effectY = boxBlurred.Y;
      break;
    }

    default:
      new (&bmap32) SBitmap(glyphW, glyphH, D3DFMT_A8R8G8B8, 0);
      memset(bmap32.Data, 0, bmap32.Size);
      effectX = 0;
      effectY = 0;
      break;
  }

  // Cleanup GDI objects
  SelectObject(hdc, oldBitmap);
  SelectObject(hdc, oldFont);
  DeleteObject(hBitmap);
  DeleteObject(hFont);
  DeleteDC(hdc);

  // Store dimensions and offset
  frame.Text.TrueTypeWidth = bmap32.Width;
  frame.Text.TrueTypeHeight = bmap32.Height;
  frame.Text.TrueTypeX = effectX;
  frame.Text.TrueTypeY = effectY;

  /* CONFORMANCE: g_SuppressMouseMessages disabled — not in original binary.
     Was: suppress mouse during D3D texture ops to prevent re-entrant RerenderText crash. */

  // Release previous texture if any
  if (frame.Text.TrueTypeTextureIndex >= 0) {
    this->Gepard->ReleaseTexture(frame.Text.TrueTypeTextureIndex, false);
    frame.Text.TrueTypeTextureIndex = -1;
  }

  // Create D3D texture from bitmap
  int texIdx = this->Gepard->CreateTextureFromBitmap("<dynamic>_hq.tga", &bmap32, false);
  frame.Text.TrueTypeTextureIndex = texIdx;

  // Emboss effect uses alpha type 4 (original line 3563)
  if (fe == (FontEffect)4)
    this->Gepard->ChangeTextureAlphaType(texIdx, 4);
  else if (texIdx >= 0)
    this->Gepard->ChangeTextureAlphaType(texIdx, 2);

}

#if 0 // Render + RerenderText — disabled pending rewrite

//----- (0041E910) --------------------------------------------------------

// bad sp value at call has been detected, the output may be wrong!

void SBoard::Render_DISABLED(float a2, int a3, int a4)

{
  SBoard *v4; // edi
  int *v5; // eax
  SHeap<SFrame>::__Tstruct *array; // esi
  int Sibling;
  SHeap<SFrame>::__Tstruct *v8; // ecx
  float X; // xmm2_4
  float Y; // xmm1_4
  int v11;
  int v12;
  int v13;
  SHeap<SFrame>::__Tstruct *v14; // eax
  int v15;
  int v16;
  float *v17; // ecx
  float v18;
  float v19; // xmm2_4
  float v20; // xmm0_4
  float v21;
  float v22;
  float v23;
  int v24; // xmm5_4
  float v25;
  float v26;
  int v27; // xmm4_4
  float v28;
  int *v29; // eax
  int v30;
  int v31;
  short v32; // ax
  short v33; // cx
  short v34; // ax
  float v35; // xmm0_4
  int v36; // xmm0_4
  int v37; // xmm0_4
  float v38; // xmm0_4
  SHeap<SFrame>::__Tstruct *v39; // eax
  __m128 v40; // xmm0
  __m128 v41; // xmm0
  float v42;
  float v43;
  float v44;
  SHeap<SFrame>::__Tstruct *v45; // eax
  signed int v46;
  signed int v47;
  __m128 v48; // xmm5
  int v49;
  __m128 v50; // xmm3
  __m128 v51; // xmm2
  __m128 v52; // xmm3
  __m128 v53; // xmm4
  __m128 v54; // xmm1
  __m128 v55; // xmm3
  __m128 v56; // xmm7
  __m128 v57; // xmm3
  __m128 v58; // xmm3
  __m128 v59; // xmm5
  __m128 v60; // xmm3
  __m128 v61; // xmm3
  __m128 v62; // xmm3
  float v63; // xmm5_4
  float v64; // xmm4_4
  int v65;
  float v66; // xmm2_4
  float v67; // xmm3_4
  float v68; // xmm7_4
  int v69;
  float v70; // xmm0_4
  float v71; // xmm0_4
  float v72; // xmm0_4
  float v73; // xmm0_4
  float v74; // xmm1_4
  float v75; // xmm0_4
  float v76; // xmm4_4
  float v77; // xmm1_4
  float v78; // xmm0_4
  float v79; // xmm0_4
  float v80; // xmm0_4
  float v81; // xmm0_4
  int v82;
  int v83;
  int v84;
  const char *v85; // ecx
  char *v86; // esi
  unsigned char v87; // al
  unsigned int v88;
  unsigned int v89;
  char v90; // cl
  unsigned char v91; // ah
  char v92; // cl
  unsigned char v93; // ah
  int v94;
  int v95;
  float v96; // xmm0_4
  float v97;
  int v98;
  float v99; // xmm3_4
  __m128i v100; // xmm0
  int v101; // kr08_4
  float v102; // xmm0_4
  SGepard *v103; // ecx
  float v104; // xmm1_4
  float v105; // xmm2_4
  float v106; // xmm0_4
  bool v107; // cc
  SGepard *v108; // ecx
  float v109; // xmm0_4
  int v110; // xmm0_4
  int v111; // xmm0_4
  float v112; // xmm0_4
  int v113; // xmm0_4
  float v114; // xmm2_4
  float v115; // xmm3_4
  float v116; // xmm1_4
  int maxsize;
  int size;
  int v119;
  SBoard::FrameData *v120; // eax
  int v121;
  int v122;
  SBoard::FrameData *v123; // ecx
  int v124;
  int v125;
  int v126;
  SBoard::FrameData *v127; // eax
  int v128;
  int CursorFont;
  int CursorGlyph;
  int v131;
  int v132;
  float CursorX; // xmm0_4
  int v134;
  SHeap<SFontProp>::__Tstruct *v135; // eax
  int v137;
  int v138;
  int v139;
  __m128 v140;
  int v141;
  int v142;
  int v143;
  int v144;
  int v145;
  int v146;
  int v147;
  int v148;
  int v149;
  int v150;
  long long v151;
  int v152;
  int v153;
  int p;
  int p_4;
  int p_8;
  int v157;
  __m128 v158;
  __m128 v159;
  int v160;
  int v161;
  double v162;
  int v163;
  int v164;
  SGepard *Gepard;
  int v166;
  int v167;
  SBoard *v168;
  int v169;
  int v170;
  int v171;
  double v172;
  int v173;
  float v174;
  int v175;
  unsigned int v176;
  unsigned int v177;
  int v178;
  int v179;
  int v180;
  int v181;
  int v182;
  unsigned char v183;
  int v184;
  signed int v185;
  int v186;
  float v187;
  int v188;
  float v189;
  int v190;
  float v191;
  int v192;
  int v193;
  signed int v194;
  const char *v195;
  char *v196;
  char *v197;
  char *v198;
  int v199;
  int v200;
  float scaleFactor;
  int v202;
  int v203;
  D3DXVECTOR2 v204;
  int v205;
  D3DXVECTOR2 bound[4];
  float retaddr = 0.0f; // IDA artifact: reads [ebp+4] (return address) as float; dead code
  bound[3].x = a2;
  bound[3].y = retaddr;
  v4 = this;
  ((void (__stdcall *)(IDirect3DDevice9 *, int, _DWORD, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, SBoard *, int, int, int, int, int, int, int, int, int, int, _DWORD, int, _DWORD, _DWORD, int, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))this->lpD3DDev->SetRenderState)(
    this->lpD3DDev,
    28,
    0,
    a3,
    a4,
    v137,
    v138,
    v139,
    v141,
    v142,
    v143,
    v144,
    v145,
    v146,
    v147,
    v148,
    v149,
    v150,
    v152,
    v153,
    p,
    p_4,
    p_8,
    v157,
    v160,
    v161,
    v163,
    v166,
    v167,
    this,
    v169,
    v171,
    v173,
    v178,
    v179,
    v182,
    v184,
    v188,
    v190,
    v192,
    LODWORD(1.0f),
    v202,
    LODWORD(v204.x),
    LODWORD(v204.y),
    v205,
    LODWORD(bound[0].x),
    LODWORD(bound[0].y),
    LODWORD(bound[1].x),
    LODWORD(bound[1].y),
    LODWORD(bound[2].x));
  v4->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 0);
  v4->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, 3u);
  v4->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, 3u);
  v4->lpD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, 4u);
  v4->Gepard->SetLightingType(LT_PRELIT);
  v4->Gepard->SetDrawType(DT_NORMAL);
  v4->lpD3DDev->SetFVF(324u);
  if ( v4->Gepard->Flags[1] )
    return;
  dword_58F3B8 = 0;
  dword_58F3D4 = 0;
  dword_58F3F0 = 0;
  dword_58F40C = 0;
  dword_58F3BC = 1065353216;
  dword_58F3D8 = 1065353216;
  dword_58F3F4 = 1065353216;
  dword_58F410 = 1065353216;
  dword_58F3C0 = 0xFFFFFF;
  dword_58F3DC = 0xFFFFFF;
  dword_58F3F8 = 0xFFFFFF;
  dword_58F414 = 0xFFFFFF;
  if ( init )
  {
    v5 = &dword_58F3BC;
    do
    {
      *(v5 - 1) = 0;
      *v5 = 1065353216;
      v5[1] = 0xFFFFFF;
      v5 += 7;
    }
    while ( (int)v5 < (int)dword_5976FC );
    init = 0;
  }
  array = v4->Frames.array;
  Sibling = 0;
  v8 = array;
  X = array->data.X;
  Y = array->data.Y;
  while ( 2 )
  {
    while ( 2 )
    {
      v180 = Sibling;
      v11 = (int)sizeof(SHeap<SFrame>::Element) * Sibling;
      v191 = X;
      v189 = Y;
      v203 = v11;
      if ( (*((_BYTE *)&array->data.Flags + v11) & 0x10) != 0 )
        v12 = *(SFrameType *)((char *)&array->data.Type + v11);
      else
        v12 = 0;
      switch ( v12 )
      {
        case 1:
          v13 = *(int *)((char *)&array->data.Sprite.Font + v11);
          if ( v13 >= 0 )
          {
            v4->Gepard->SetTexture(0, v4->Fonts.array[v13].data.TextureIndex, 1);
            v14 = v4->Frames.array;
            vert[0].x = (float)(*(float *)((char *)&v14->data.Sprite.Glyph.Dest.Left + v203) * scaleFactor) + X;
            dword_58F3CC = LODWORD(vert[0].x);
            *(float *)&dword_58F3B4 = (float)(*(float *)((char *)&v14->data.Sprite.Glyph.Dest.Top + v203) * scaleFactor)
                                    + Y;
            dword_58F3EC = dword_58F3B4;
            *(float *)&dword_58F3E8 = (float)(*(float *)((char *)&v14->data.Sprite.Glyph.Dest.Right + v203) * scaleFactor)
                                    + X;
            dword_58F404 = dword_58F3E8;
            *(float *)&dword_58F3D0 = (float)(*(float *)((char *)&v14->data.Sprite.Glyph.Dest.Bottom + v203)
                                            * scaleFactor)
                                    + Y;
            dword_58F408 = dword_58F3D0;
            dword_58F3C4 = *(int *)((char *)&v14->data.Text.Align + v203);
            dword_58F3E0 = dword_58F3C4;
            dword_58F3C8 = *(int *)((char *)&v14->data.Text.Text + v203);
            dword_58F400 = dword_58F3C8;
            dword_58F3FC = *(int *)((char *)&v14->data.Text.ReducedText + v203);
            dword_58F418 = dword_58F3FC;
            dword_58F3E4 = *(int *)((char *)&v14->data.Text.Width + v203);
            dword_58F41C = dword_58F3E4;
            v15 = *(int *)((char *)&v4->Frames.array->data.Text.TrueTypeTextureIndex + v203);
            if ( v15 )
            {
              dword_58F3C0 = *(int *)((char *)&v4->Frames.array->data.Text.TrueTypeTextureIndex + v203);
              dword_58F3DC = v15;
              dword_58F3F8 = v15;
              dword_58F414 = v15;
              v4->lpD3DDev->SetPixelShader(v4->Gepard->sepiaPixelShader.Ptr);
            }
            else
            {
              dword_58F3C0 = 0xFFFFFF;
              dword_58F3DC = 0xFFFFFF;
              dword_58F3F8 = 0xFFFFFF;
              dword_58F414 = 0xFFFFFF;
            }
            v4->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 28u);
            v11 = v203;
            if ( *(unsigned int *)((char *)&v4->Frames.array->data.Sprite.SepiaColor + v203) )
            {
              v4->lpD3DDev->SetPixelShader(0);
              goto LABEL_118;
            }
          }
          goto LABEL_119;
        case 2:
        case 3:
          if ( *(int *)((char *)&array->data.Text.TrueTypeTextureIndex + v11) >= 0 )
            goto LABEL_98;
          v175 = *(int *)((char *)&array->data.Sprite.Font + v11);
          if ( v175 < 0 )
            goto LABEL_119;
          v82 = (int)X;
          v164 = (int)Y;
          v83 = *(int *)((char *)&array->data.Text.Align + v11);
          v170 = (int)X;
          if ( v83 == 1 )
          {
            v84 = v82 - *(int *)((char *)&array->data.Text.Width + v11);
          }
          else
          {
            if ( v83 != 2 )
              goto LABEL_67;
            v84 = *(int *)((char *)&array->data.Text.Width + v11) / -2 + v82;
          }
          v170 = v84;
LABEL_67:
          v4->Gepard->SetTexture(0, v4->Fonts.array[v175].data.TextureIndex, 1);
          v11 = v203;
          dword_58F3C0 = *(int *)((char *)&v4->Frames.array->data.Sprite.Glyph.Width + v203);
          dword_58F3DC = dword_58F3C0;
          dword_58F3F8 = dword_58F3C0;
          dword_58F414 = dword_58F3C0;
          array = v4->Frames.array;
          v85 = *(char **)((char *)&array->data.Text.ReducedText + v203);
          v195 = v85;
          if ( !v85 )
          {
            v85 = *(char **)((char *)&array->data.Text.Text + v203);
            v195 = v85;
          }
          v176 = (unsigned int)&v85[strlen(v85) + 1];
          v4 = v168;
          v177 = v176 - 1;
          if ( (unsigned int)v85 >= v177 )
            goto LABEL_98;
          v86 = (char *)v195;
          break;
        case 4:
          v4->Gepard->SetTexture(0, -1, 1);
          v4->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
          v4->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 5u);
          v4->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 6u);
          v35 = X - 0.5;
          vert[0].x = v35;
          dword_58F3CC = LODWORD(v35);
          *(float *)&v36 = Y - 0.5;
          dword_58F3B4 = v36;
          dword_58F3EC = v36;
          *(float *)&v37 = (float)((float)((float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor)
                                 + X)
                         - 0.5;
          dword_58F3E8 = v37;
          dword_58F404 = v37;
          v38 = (float)*(int *)((char *)&v4->Frames.array->data.Height + v203);
          dword_58F3C4 = 0;
          dword_58F3E0 = 0;
          dword_58F3FC = 0;
          dword_58F418 = 0;
          dword_58F3C8 = 0;
          dword_58F3E4 = 0;
          dword_58F400 = 0;
          dword_58F41C = 0;
          *(float *)&dword_58F3D0 = (float)((float)(v38 * scaleFactor) + Y) - 0.5;
          dword_58F408 = dword_58F3D0;
          dword_58F3C0 = *(int *)((char *)&v4->Frames.array->data.Sprite.Font + v203);
          dword_58F3DC = dword_58F3C0;
          dword_58F3F8 = dword_58F3C0;
          dword_58F414 = dword_58F3C0;
          goto LABEL_117;
        case 5:
          if ( !*(int *)((char *)&array->data.Sprite.Font + v11) )
            goto LABEL_119;
          if ( *(unsigned int *)((char *)&array->data.Anim.EndTime + v11) )
          {
            v107 = v4->Gepard->AnimTime <= *(unsigned int *)((char *)&array->data.Anim.EndTime + v11);
            v4 = v168;
            if ( !v107 )
              goto LABEL_111;
          }
          Gepard = v4->Gepard;
          v4 = v168;
          if ( Gepard->AnimTime <= *(int *)((char *)&array->data.Text.Align + v11)
                                 + *(unsigned int *)((char *)&array->data.Anim.FrameTime + v11) )
            goto LABEL_116;
          while ( 1 )
          {
            *(int *)((char *)&array->data.Text.Align + v11) += *(int *)((char *)&array->data.Text.ReducedText + v11);
 if ( !SAnimation::NextFrame( *(SAnimation **)((char *)&v168->Frames.array->data.Anim.Anim + v11),
                    *((_BYTE *)&v168->Frames.array->data.Scaler + v11 + 20) & 1) )
              break;
            array = v168->Frames.array;
            v11 = v203;
            v108 = v168->Gepard;
            if ( v108->AnimTime <= *(int *)((char *)&array->data.Text.Align + v11)
                                 + *(unsigned int *)((char *)&array->data.Anim.FrameTime + v11) )
            {
 SGepard::UpdateTextureFromBitmap( v108, *(int *)((char *)&array->data.Sprite.Glyph.Width + v203),
                *(SBitmap **)((char *)&array->data.Anim.Anim + v203));
              v8 = v168->Frames.array;
              v11 = v203;
LABEL_116:
              v168->Gepard->SetTexture(0, *(int *)((char *)&v8->data.Sprite.Glyph.Width + v11), 1);
              v109 = X - 0.5;
              vert[0].x = v109;
              dword_58F3CC = LODWORD(v109);
              *(float *)&v110 = Y - 0.5;
              dword_58F3B4 = v110;
              dword_58F3EC = v110;
              *(float *)&v111 = (float)((float)((float)*(int *)((char *)&v168->Frames.array->data.Width + v203)
                                              * scaleFactor)
                                      + X)
                              - 0.5;
              dword_58F3E8 = v111;
              dword_58F404 = v111;
              v112 = (float)*(int *)((char *)&v168->Frames.array->data.Height + v203);
              dword_58F3C4 = 0;
              dword_58F3E0 = 0;
              dword_58F3C8 = 0;
              dword_58F400 = 0;
              *(float *)&v113 = (float)((float)(v112 * scaleFactor) + Y) - 0.5;
              dword_58F3D0 = v113;
              dword_58F408 = v113;
              dword_58F3FC = *(int *)((char *)&v168->Frames.array->data.Text.TrueTypeX + v203);
              dword_58F418 = dword_58F3FC;
              dword_58F3E4 = *(int *)((char *)&v168->Frames.array->data.Text.TrueTypeY + v203);
              dword_58F41C = dword_58F3E4;
              dword_58F3C0 = 0xFFFFFF;
              dword_58F3DC = 0xFFFFFF;
              dword_58F3F8 = 0xFFFFFF;
              dword_58F414 = 0xFFFFFF;
LABEL_117:
              v4->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 28u);
              goto LABEL_118;
            }
          }
LABEL_111:
          v4->StopAnim(v4, v180);
LABEL_118:
          v11 = v203;
          goto LABEL_119;
        case 6:
 SGepard::SetTexture( v4->Gepard, 0, v4->Fonts.array[*(int *)((char *)&array->data.Sprite.Font + v11)].data.TextureIndex,
            1);
          v4->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, 4u);
          v4->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, 4u);
          v4->lpD3DDev->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);
          v39 = v4->Frames.array;
          vert[0].x = (float)(*(float *)((char *)&v39->data.Sprite.Glyph.Dest.Left + v203) * scaleFactor) + X;
          dword_58F3CC = LODWORD(vert[0].x);
          *(float *)&dword_58F3B4 = (float)(*(float *)((char *)&v39->data.Sprite.Glyph.Dest.Top + v203) * scaleFactor)
                                  + Y;
          dword_58F3EC = dword_58F3B4;
          *(float *)&dword_58F3E8 = (float)(*(float *)((char *)&v39->data.Sprite.Glyph.Dest.Right + v203) * scaleFactor)
                                  + X;
          dword_58F404 = dword_58F3E8;
          *(float *)&dword_58F3D0 = (float)(*(float *)((char *)&v39->data.Sprite.Glyph.Dest.Bottom + v203) * scaleFactor)
                                  + Y;
          dword_58F408 = dword_58F3D0;
          v40 = _libm_sse2_sin_precise(LODWORD(bound[2].y), LODWORD(bound[3].x), LODWORD(bound[3].y));
          v40.m128_f32[0] = *(double *)v40.m128_u64;
          v174 = v40.m128_f32[0];
          v158 = _mm_shuffle_ps(v40, v40, 0);
          v41 = _libm_sse2_cos_precise(LODWORD(bound[2].y), LODWORD(bound[3].x), LODWORD(bound[3].y));
          v41.m128_f32[0] = *(double *)v41.m128_u64;
          v42 = v174 * 0.5;
          v43 = v41.m128_f32[0] * 0.5;
          v140 = _mm_shuffle_ps(v41, v41, 0);
          *(float *)&dword_58F3C4 = 0.5 - v43 + v42;
          v41.m128_f32[0] = 0.5 - v43 - v42;
          LODWORD(bound[2].x) = 28;
          dword_58F3C0 = 0xFFFFFF;
          dword_58F3DC = 0xFFFFFF;
          dword_58F3F8 = 0xFFFFFF;
          dword_58F414 = 0xFFFFFF;
          v44 = v43 + 0.5;
          dword_58F400 = dword_58F3C4;
          dword_58F3C8 = v41.m128_i32[0];
          dword_58F3E0 = v41.m128_i32[0];
          LODWORD(bound[1].y) = vert;
          LODWORD(bound[1].x) = 2;
          LODWORD(bound[0].y) = 5;
          *(float *)&dword_58F3E4 = v44 - v42;
          *(float *)&dword_58F3FC = v44 + v42;
          dword_58F418 = dword_58F3E4;
          dword_58F41C = dword_58F3FC;
          v4->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 28u);
          v4->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, 3u);
          v4->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, 3u);
          v4->Gepard->SetTexture(0, -1, 1);
          v4->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
          v4->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 5u);
          v4->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 6u);
          SGepard::GetViewBoundaries(v4->Gepard, &v204);
          v45 = v4->Frames.array;
          v46 = *(int *)((char *)&v45->data.Width + v203);
          v47 = *(int *)((char *)&v45->data.Height + v203);
          v185 = v46;
          v194 = v47;
          v48 = _mm_sub_ps(
                  _mm_unpacklo_ps(
                    _mm_unpacklo_ps((__m128)LODWORD(v204.x), (__m128)LODWORD(bound[0].y)),
                    _mm_unpacklo_ps((__m128)(unsigned int)v205, (__m128)LODWORD(bound[1].y))),
                  (__m128)_xmm);
          v49 = 0;
          dword_58F3B8 = 0;
          v50 = _mm_sub_ps(
                  _mm_unpacklo_ps(
                    _mm_unpacklo_ps((__m128)LODWORD(v204.y), (__m128)LODWORD(bound[1].x)),
                    _mm_unpacklo_ps((__m128)LODWORD(bound[0].x), (__m128)LODWORD(bound[2].x))),
                  (__m128)_xmm);
          dword_58F3D4 = 0;
          dword_58F3BC = 1065353216;
          dword_58F3D8 = 1065353216;
          dword_58F3C0 = -1056964672;
          dword_58F3DC = -1056964672;
          dword_58F3C4 = 0;
          dword_58F3E0 = 0;
          dword_58F3C8 = 0;
          dword_58F3E4 = 0;
          v51 = _mm_add_ps(_mm_mul_ps(v140, v50), _mm_mul_ps(v158, v48));
          v52 = _mm_add_ps(_mm_sub_ps(_mm_mul_ps(v140, v48), _mm_mul_ps(v158, v50)), (__m128)_xmm);
          v53 = _mm_shuffle_ps(v52, v52, 229);
          v54 = _mm_unpackhi_ps(v53, v53);
          v55 = _mm_mul_ps(
                  _mm_unpacklo_ps(_mm_unpacklo_ps(v52, v54), _mm_unpacklo_ps(v53, _mm_unpackhi_ps(v54, v54))),
                  _mm_cvtepi32_ps(_mm_shuffle_epi32(_mm_cvtsi32_si128(v46), 0)));
          v159 = _mm_sub_ps((__m128)_xmm, v51);
          v56 = _mm_shuffle_ps(v159, v159, 229);
          LODWORD(v204.x) = v55.m128_i32[0];
          v57 = _mm_shuffle_ps(v55, v55, 229);
          v205 = v57.m128_i32[0];
          v58 = _mm_unpackhi_ps(v57, v57);
          LODWORD(bound[0].y) = v58.m128_i32[0];
          v59 = _mm_unpackhi_ps(v56, v56);
          LODWORD(bound[1].y) = _mm_unpackhi_ps(v58, v58).m128_u32[0];
          v60 = _mm_mul_ps(
                  _mm_unpacklo_ps(
                    _mm_unpacklo_ps((__m128)v159.m128_u32[0], v59),
                    _mm_unpacklo_ps(v56, _mm_unpackhi_ps(v59, v59))),
                  _mm_cvtepi32_ps(_mm_shuffle_epi32(_mm_cvtsi32_si128(v47), 0)));
          LODWORD(v204.y) = v60.m128_i32[0];
          v61 = _mm_shuffle_ps(v60, v60, 229);
          LODWORD(bound[0].x) = v61.m128_i32[0];
          v62 = _mm_unpackhi_ps(v61, v61);
          LODWORD(bound[1].x) = v62.m128_i32[0];
          LODWORD(bound[2].x) = _mm_unpackhi_ps(v62, v62).m128_u32[0];
          do
          {
            v63 = *(&v204.x + 2 * v49);
            v64 = bound[v49++ - 1].x;
            v65 = v49 & 3;
            v66 = *(&v204.x + 2 * v65);
            v67 = bound[v65 - 1].x;
            if ( v63 >= 0.0 || v66 >= 0.0 )
            {
              v68 = (float)(v46 - 1);
              if ( (v63 <= v68 || v66 <= v68) && (v64 >= 0.0 || v67 >= 0.0) )
              {
                v69 = v47 - 1;
                v70 = (float)(v47 - 1);
                if ( v64 <= v70 || v67 <= v70 )
                {
                  if ( v63 < 0.0 )
                  {
                    v71 = v66 - v63;
                    v63 = 0.0;
                    v64 = (float)((float)((float)(v64 - v67) * v66) / v71) + v67;
                  }
                  if ( v66 < 0.0 )
                  {
                    v72 = v63 - v66;
                    v66 = 0.0;
                    v67 = (float)((float)((float)(v67 - v64) * v63) / v72) + v64;
                  }
                  v73 = (float)v46;
                  if ( v63 > v68 )
                  {
                    v74 = v66 - v73;
                    v75 = v66 - v63;
                    v63 = (float)(v46 - 1);
                    v76 = (float)((float)(v64 - v67) * (float)(v74 + 1.0)) / v75;
                    v73 = (float)v46;
                    v64 = v76 + v67;
                  }
                  if ( v66 > v68 )
                  {
                    v77 = v63 - v73;
                    v78 = v63 - v66;
                    v66 = (float)(v46 - 1);
                    v67 = (float)((float)((float)(v67 - v64) * (float)(v77 + 1.0)) / v78) + v64;
                  }
                  if ( v64 < 0.0 )
                  {
                    v79 = v67 - v64;
                    v64 = 0.0;
                    v63 = (float)((float)((float)(v63 - v66) * v67) / v79) + v66;
                  }
                  if ( v67 < 0.0 )
                  {
                    v80 = v64 - v67;
                    v67 = 0.0;
                    v66 = (float)((float)((float)(v66 - v63) * v64) / v80) + v63;
                  }
                  v81 = (float)v69;
                  if ( v64 > (float)v69 )
                  {
                    v81 = (float)v69;
                    v63 = (float)((float)((float)(v63 - v66) * (float)((float)(v67 - (float)v47) + 1.0))
                                / (float)(v67 - v64))
                        + v66;
                    v64 = (float)v69;
                  }
                  if ( v67 > v81 )
                  {
                    v81 = (float)v69;
                    v66 = (float)((float)((float)(v66 - v63) * (float)((float)(v64 - (float)v47) + 1.0))
                                / (float)(v64 - v67))
                        + v63;
                    v67 = (float)v69;
                  }
                  if ( (v63 >= 0.0 || v66 >= 0.0)
                    && (v63 <= v68 || v66 <= v68)
                    && (v64 >= 0.0 || v67 >= 0.0)
                    && (v64 <= v81 || v67 <= v81) )
                  {
                    LODWORD(bound[2].x) = 28;
                    LODWORD(bound[1].y) = vert;
                    LODWORD(bound[1].x) = 1;
                    LODWORD(bound[0].y) = 2;
                    vert[0].x = (float)(v63 * scaleFactor) + v191;
                    *(float *)&dword_58F3B4 = (float)(v64 * scaleFactor) + v189;
                    *(float *)&dword_58F3CC = (float)(v66 * scaleFactor) + v191;
                    *(float *)&dword_58F3D0 = (float)(v67 * scaleFactor) + v189;
                    v4->lpD3DDev->DrawPrimitiveUP(D3DPT_LINELIST, 1u, vert, 28u);
                    v46 = v185;
                    v47 = v194;
                  }
                }
              }
            }
          }
          while ( v49 < 4 );
          goto LABEL_118;
        case 8:
          v16 = *(int *)((char *)&array->data.Sprite.Font + v11);
          if ( v16 < 0 )
            goto LABEL_119;
          v4->Gepard->SetTexture(0, v4->Fonts.array[v16].data.TextureIndex, 1);
          v17 = (float *)((char *)&v4->Frames.array->use + v203);
          v18 = X - 0.5;
          v19 = (float)(v17[19] - v17[17]) * scaleFactor;
          v20 = (float)((float)(v17[18] - v17[16]) * scaleFactor) * 0.5;
          vert[0].x = v18;
          v162 = v191 - 0.5;
          v21 = v20;
          v22 = v18 - v21;
          v23 = (float)(v19 * 0.5);
          *(float *)&v24 = v21 + v18;
          dword_58F3CC = v24;
          *(float *)&v21 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor)
                         + v18
                         - v21;
          dword_58F3E8 = LODWORD(v21);
          *(float *)&v21 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor) + v18;
          v25 = v189 - 0.5;
          dword_58F404 = LODWORD(v21);
          v26 = v23 + v25;
          v172 = v25;
          *(float *)&v27 = v25;
          v28 = v25 - v23;
          dword_58F3B4 = v27;
          *(float *)&dword_58F424 = v26;
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v28;
          dword_58F494 = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v172;
          dword_58F504 = LODWORD(v26);
          dword_58F3C4 = *((_DWORD *)v17 + 12);
          *(float *)&dword_58F3FC = (float)(v17[14] + v17[12]) * 0.5;
          dword_58F3E0 = dword_58F3FC;
          dword_58F418 = *((_DWORD *)v17 + 14);
          dword_58F3C8 = *((_DWORD *)v17 + 13);
          *(float *)&dword_58F4A8 = (float)(v17[15] + v17[13]) * 0.5;
          dword_58F438 = dword_58F4A8;
          dword_58F518 = *((_DWORD *)v17 + 15);
          dword_58F420 = LODWORD(vert[0].x);
          dword_58F43C = v24;
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor) + v22;
          dword_58F458 = LODWORD(v26);
          *(float *)&v26 = (float)*(int *)((char *)&v4->Frames.array->data.Width + v203);
          dword_58F3D0 = v27;
          dword_58F440 = dword_58F424;
          *(float *)&v26 = (float)(*(float *)&v26 * scaleFactor) + v162;
          dword_58F474 = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v28;
          dword_58F4B0 = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v172;
          dword_58F520 = LODWORD(v26);
          dword_58F434 = *((_DWORD *)v17 + 12);
          *(float *)&dword_58F46C = (float)(v17[14] + v17[12]) * 0.5;
          dword_58F450 = dword_58F46C;
          dword_58F488 = *((_DWORD *)v17 + 14);
          dword_58F3E4 = *((_DWORD *)v17 + 13);
          *(float *)&dword_58F4C4 = (float)(v17[15] + v17[13]) * 0.5;
          dword_58F454 = dword_58F4C4;
          dword_58F534 = *((_DWORD *)v17 + 15);
          dword_58F490 = LODWORD(vert[0].x);
          dword_58F4AC = v24;
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor) + v22;
          dword_58F4C8 = LODWORD(v26);
          *(float *)&v26 = (float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor;
          dword_58F3EC = v27;
          dword_58F45C = dword_58F424;
          *(float *)&v26 = *(float *)&v26 + v162;
          dword_58F4E4 = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v28;
          dword_58F4CC = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v172;
          dword_58F53C = LODWORD(v26);
          dword_58F4A4 = *((_DWORD *)v17 + 12);
          *(float *)&dword_58F4DC = (float)(v17[14] + v17[12]) * 0.5;
          dword_58F4C0 = dword_58F4DC;
          dword_58F4F8 = *((_DWORD *)v17 + 14);
          dword_58F400 = *((_DWORD *)v17 + 13);
          *(float *)&dword_58F4E0 = (float)(v17[15] + v17[13]) * 0.5;
          dword_58F470 = dword_58F4E0;
          dword_58F550 = *((_DWORD *)v17 + 15);
          dword_58F51C = v24;
          dword_58F500 = LODWORD(vert[0].x);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Width + v203) * scaleFactor) + v22;
          dword_58F538 = LODWORD(v26);
          *(float *)&v26 = (float)*(int *)((char *)&v4->Frames.array->data.Width + v203);
          dword_58F408 = v27;
          dword_58F478 = dword_58F424;
          *(float *)&v26 = (float)(*(float *)&v26 * scaleFactor) + v162;
          dword_58F554 = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v28;
          dword_58F4E8 = LODWORD(v26);
          *(float *)&v26 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Height + v203) * scaleFactor) + v172;
          dword_58F558 = LODWORD(v26);
          dword_58F514 = *((_DWORD *)v17 + 12);
          v29 = &dword_58F3C0;
          *(float *)&dword_58F54C = (float)(v17[14] + v17[12]) * 0.5;
          dword_58F530 = dword_58F54C;
          dword_58F568 = *((_DWORD *)v17 + 14);
          dword_58F41C = *((_DWORD *)v17 + 13);
          *(float *)&dword_58F4FC = (float)(v17[15] + v17[13]) * 0.5;
          dword_58F48C = dword_58F4FC;
          dword_58F56C = *((_DWORD *)v17 + 15);
          do
          {
            *v29 = 0xFFFFFF;
            v29 += 7;
          }
          while ( (int)v29 < (int)dword_58F580 );
          v30 = 0;
          v31 = 5;
          v32 = 0;
          v193 = 0;
          do
          {
            indices[v30] = 4 * v32;
            word_5976F2[v30] = v31 - 1;
            v30 += 18;
            word_5976D0[v30] = v31;
            word_5976D2[v30] = 4 * v32;
            word_5976D4[v30] = v31;
            word_5976D6[v30] = 4 * v32 + 1;
            word_5976D8[v30] = 4 * v32 + 1;
            word_5976DA[v30] = v31;
            word_5976DC[v30] = v31 + 1;
            word_5976DE[v30] = 4 * v32 + 1;
            word_5976E0[v30] = v31 + 1;
            word_5976E2[v30] = 4 * v32 + 2;
            v33 = 4 * v32 + 2;
            word_5976E4[v30] = v33;
            v34 = v31 + 1;
            word_5976E6[v30] = v31 + 1;
            v31 += 4;
            word_5976E8[v30] = ++v34;
            word_5976EA[v30] = v33;
            word_5976EC[v30] = v34;
            word_5976EE[v30] = v33 + 1;
            v32 = ++v193;
          }
          while ( v31 < 17 );
          v4 = v168;
          v168->lpD3DDev->DrawIndexedPrimitiveUP(
            v168->lpD3DDev,
            D3DPT_TRIANGLELIST,
            0,
            16u,
            18u,
            indices,
            D3DFMT_INDEX16,
            vert,
            28u);
          goto LABEL_118;
        default:
          goto LABEL_119;
      }
      do
      {
        v196 = v86;
        v186 = (int)&v168->Fonts.array[*(int *)((char *)&v168->Frames.array->data.Sprite.Font + v11)];
        if ( (unsigned int)v86 >= v177 )
        {
          v87 = UnicodeDecodeTable[0];
          goto LABEL_94;
        }
        v88 = (unsigned char)*v86++;
        if ( v88 - 193 > 0x1E )
        {
          if ( v88 - 225 > 0xE )
          {
            if ( v88 - 241 <= 6 )
            {
              v198 = v196 + 4;
              if ( (unsigned int)v198 <= v177 )
              {
                v92 = *v86;
                if ( (unsigned char)*v86 >= 0x80u && (unsigned char)v92 <= 0xBFu )
                {
                  v93 = v86[1];
                  if ( (unsigned char)(v93 + 0x80) <= 0x3Fu )
                  {
                    v183 = v86[2];
                    if ( (unsigned char)(v183 + 0x80) <= 0x3Fu )
                    {
                      v86 = v198;
                      v89 = (v183 & 0x3F) + (((v93 & 0x3F) + ((((v88 & 7) << 6) + (v92 & 0x3F)) << 6)) << 6);
                      goto LABEL_91;
                    }
                  }
                }
              }
            }
          }
          else
          {
            v197 = v196 + 3;
            if ( (unsigned int)v197 <= v177 )
            {
              v90 = *v86;
              if ( (unsigned char)*v86 >= 0x80u && (unsigned char)v90 <= 0xBFu )
              {
                v91 = v86[1];
                if ( (unsigned char)(v91 + 0x80) <= 0x3Fu )
                {
                  v86 = v197;
                  v89 = (v91 & 0x3F) + ((((v88 & 0xF) << 6) + (v90 & 0x3F)) << 6);
                  goto LABEL_91;
                }
              }
            }
          }
        }
        else if ( (unsigned int)(v86 + 1) <= v177 && *v86 < -64 )
        {
          v89 = ((v88 & 0x1F) << 6) + (*v86 & 0x3F);
          v86 = v196 + 2;
          goto LABEL_91;
        }
        v89 = v88;
LABEL_91:
        if ( v89 >= 0x460 )
          v87 = 63;
        else
          v87 = UnicodeDecodeTable[v89];
LABEL_94:
        v94 = 9 * v87;
        v95 = *(_DWORD *)(v186 + 4 * v94 + 52);
        v199 = v94;
        if ( v95 )
        {
          v96 = *(float *)(v186 + 4 * v94 + 72);
          LODWORD(bound[2].x) = 28;
          LODWORD(bound[1].y) = vert;
          LODWORD(bound[1].x) = 2;
          LODWORD(bound[0].y) = 5;
          vert[0].x = v96 + (float)v170;
          *(float *)&dword_58F3CC = v96 + (float)v170;
          *(float *)&dword_58F3B4 = *(float *)(v186 + 4 * v94 + 76) + (float)v164;
          dword_58F3EC = dword_58F3B4;
          *(float *)&dword_58F3E8 = *(float *)(v186 + 4 * v94 + 80) + (float)v170;
          dword_58F404 = dword_58F3E8;
          *(float *)&dword_58F3D0 = *(float *)(v186 + 4 * v94 + 84) + (float)v164;
          dword_58F408 = dword_58F3D0;
          dword_58F3C4 = *(_DWORD *)(v186 + 4 * v94 + 56);
          dword_58F3E0 = dword_58F3C4;
          dword_58F3C8 = *(_DWORD *)(v186 + 4 * v94 + 60);
          dword_58F400 = dword_58F3C8;
          dword_58F3FC = *(_DWORD *)(v186 + 4 * v94 + 64);
          dword_58F418 = dword_58F3FC;
          dword_58F3E4 = *(_DWORD *)(v186 + 4 * v94 + 68);
          dword_58F41C = dword_58F3E4;
          v168->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 28u);
          v95 = *(_DWORD *)(v186 + 4 * v199 + 52);
        }
        v170 += v95;
        v11 = v203;
      }
      while ( (unsigned int)v86 < v177 );
      array = v168->Frames.array;
LABEL_98:
      v97 = *(float *)((char *)&array->data.Minimap.Rotation + v11);
      if ( v97 >= 0.0 )
      {
        v98 = *(int *)((char *)&array->data.Text.Align + v11);
        v99 = X;
        if ( v98 == 1 )
        {
          v100 = _mm_cvtsi32_si128(*(int *)((char *)&array->data.Text.TrueTypeWidth + v11));
          goto LABEL_103;
        }
        if ( v98 == 2 )
        {
          v101 = *(int *)((char *)&array->data.Text.TrueTypeWidth + v11);
          v11 = v203;
          v100 = _mm_cvtsi32_si128(v101 / 2);
LABEL_103:
          v99 = X - _mm_cvtepi32_ps(v100).m128_f32[0];
        }
        v102 = (float)*(int *)((char *)&array->data.Text.TrueTypeX + v11);
        LODWORD(bound[2].x) = 1;
        bound[1].y = v97;
        v103 = v4->Gepard;
        bound[1].x = 0.0;
        v187 = (float)*(int *)((char *)&array->data.Text.TrueTypeY + v11) + Y;
        v103->SetTexture(0, SLODWORD(bound[1].y), 1);
        if ( v4->roundPixels )
        {
          v104 = (float)(int)(float)(v102 + v99);
          v105 = (float)(int)v187;
        }
        else
        {
          v104 = v102 + v99;
          v105 = v187;
        }
        vert[0].x = v104 - 0.5;
        *(float *)&dword_58F3CC = v104 - 0.5;
        *(float *)&dword_58F3B4 = v105 - 0.5;
        *(float *)&dword_58F3EC = v105 - 0.5;
        *(float *)&dword_58F3E8 = (float)((float)*(int *)((char *)&v4->Frames.array->data.Text.TrueTypeWidth + v203)
                                        + v104)
                                - 0.5;
        dword_58F404 = dword_58F3E8;
        v106 = (float)*(int *)((char *)&v4->Frames.array->data.Text.TrueTypeHeight + v203);
        dword_58F3C4 = 0;
        dword_58F3E0 = 0;
        dword_58F3C8 = 0;
        dword_58F400 = 0;
        dword_58F3FC = 1065353216;
        dword_58F418 = 1065353216;
        dword_58F3E4 = 1065353216;
        dword_58F41C = 1065353216;
        *(float *)&dword_58F3D0 = (float)(v106 + v105) - 0.5;
        *(float *)&dword_58F408 = (float)(v106 + v105) - 0.5;
        dword_58F3C0 = *(int *)((char *)&v4->Frames.array->data.Sprite.Glyph.Width + v203);
        dword_58F3DC = dword_58F3C0;
        dword_58F3F8 = dword_58F3C0;
        dword_58F414 = dword_58F3C0;
        goto LABEL_117;
      }
LABEL_119:
      array = v4->Frames.array;
      if ( (*((_BYTE *)&array->data.Flags + v11) & 0x10) != 0 && *(int *)((char *)&array->data.Child + v11) >= 0 )
      {
        v114 = v191;
        v115 = v189;
        v116 = scaleFactor;
        maxsize = v4->frameStack.maxsize;
        size = v4->frameStack.size;
        if ( size == maxsize )
        {
          if ( maxsize >= 16 )
            v119 = 6 * maxsize / 5;
          else
            v119 = 16;
          v200 = v119;
          v120 = (SBoard::FrameData *)realloc(v4->frameStack.array, 12 * v119);
          v121 = v4->frameStack.maxsize;
          v4->frameStack.array = v120;
          memset(&v120[v121], 0, 12 * (v200 - v121));
          size = v4->frameStack.size;
          v116 = scaleFactor;
          v114 = v191;
          v115 = v189;
          v4->frameStack.maxsize = v200;
        }
        v122 = size;
        v4->frameStack.size = size + 1;
        v123 = v4->frameStack.array;
        *(_QWORD *)&v123[v122].xabs = __PAIR64__(LODWORD(v189), LODWORD(v191));
        v123[v122].scaleFactor = scaleFactor;
        array = v4->Frames.array;
        v8 = array;
        if ( *(SFrameType *)((char *)&array->data.Type + v203) == FT_SCALER )
        {
          v116 = v116 * *(float *)((char *)&array->data.Scaler.ScaleFactor + v203);
          scaleFactor = v116;
        }
        Sibling = *(int *)((char *)&array->data.Child + v203);
        X = v114 + (float)(array[Sibling].data.X * v116);
        Y = (float)(v116 * array[Sibling].data.Y) + v115;
        continue;
      }
      break;
    }
    if ( *(int *)((char *)&array->data.Sibling + v11) >= 0 )
    {
      v128 = v180;
LABEL_136:
      if ( v128 >= 0 )
      {
        Sibling = array[v128].data.Sibling;
        v8 = v4->Frames.array;
        v151 = *(_QWORD *)&v4->frameStack.array[v4->frameStack.size - 1].xabs;
        X = (float)(array[Sibling].data.X * scaleFactor) + *(float *)&v151;
        Y = (float)(array[Sibling].data.Y * scaleFactor) + *((float *)&v151 + 1);
        continue;
      }
    }
    else
    {
      while ( 1 )
      {
        v181 = *(int *)((char *)&array->data.Parent + v11);
        if ( v181 < 0 )
          break;
        v124 = v4->frameStack.size;
        v125 = v124 - 1;
        scaleFactor = v4->frameStack.array[v124 - 1].scaleFactor;
        if ( v124 - 1 < 0 || v125 >= v124 )
          Logger.g->Panic("SDArray::operator[]: invalid index (%d)", v124 - 1);
        v4->frameStack.size = v125;
        v126 = v125;
        v127 = v4->frameStack.array;
        *(_QWORD *)&v127[v126].xabs = 0LL;
        v127[v126].scaleFactor = 0.0;
        v128 = v181;
        array = v4->Frames.array;
        v11 = (int)sizeof(SHeap<SFrame>::Element) * v181;
        if ( array[v181].data.Sibling >= 0 )
          goto LABEL_136;
      }
    }
    break;
  }
  if ( v4->frameStack.size )
    Logger.g->Panic("SBoard::Render: Internal error");
  if ( !v4->HardwareCursor || v4->ForceSoftwareCursor )
  {
    CursorFont = v4->CursorFont;
    if ( CursorFont >= 0 )
    {
      CursorGlyph = v4->CursorGlyph;
      if ( CursorGlyph >= 0 && CursorGlyph < v4->NumCursors )
      {
        v4->Gepard->SetTexture(0, v4->Fonts.array[CursorFont].data.TextureIndex, 1);
        v131 = 9268 * v4->CursorFont;
        v132 = v4->CursorGlyph;
        CursorX = (float)v4->CursorX;
        LODWORD(bound[2].x) = 28;
        LODWORD(bound[1].y) = vert;
        LODWORD(bound[1].x) = 2;
        v134 = v131 + 36 * v132;
        v135 = v4->Fonts.array;
        LODWORD(bound[0].y) = 5;
        vert[0].x = *(float *)((char *)&v135->data.Glyphs[0].Dest.Left + v134) + CursorX;
        dword_58F3CC = LODWORD(vert[0].x);
        *(float *)&dword_58F3B4 = *(float *)((char *)&v135->data.Glyphs[0].Dest.Top + v134) + (float)v4->CursorY;
        dword_58F3EC = dword_58F3B4;
        *(float *)&dword_58F3E8 = *(float *)((char *)&v135->data.Glyphs[0].Dest.Right + v134) + (float)v4->CursorX;
        dword_58F404 = dword_58F3E8;
        *(float *)&dword_58F3D0 = *(float *)((char *)&v135->data.Glyphs[0].Dest.Bottom + v134) + (float)v4->CursorY;
        dword_58F408 = dword_58F3D0;
        dword_58F3C4 = *(_DWORD *)((char *)&v135->data.Glyphs[0].Src.Left + v134);
        dword_58F3E0 = dword_58F3C4;
        dword_58F3C8 = *(_DWORD *)((char *)&v135->data.Glyphs[0].Src.Top + v134);
        dword_58F400 = dword_58F3C8;
        dword_58F3FC = *(_DWORD *)((char *)&v135->data.Glyphs[0].Src.Right + v134);
        dword_58F418 = dword_58F3FC;
        dword_58F3E4 = *(_DWORD *)((char *)&v135->data.Glyphs[0].Src.Bottom + v134);
        dword_58F41C = dword_58F3E4;
        dword_58F3C0 = 0xFFFFFF;
        dword_58F3DC = 0xFFFFFF;
        dword_58F3F8 = 0xFFFFFF;
        dword_58F414 = 0xFFFFFF;
        v4->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 28u);
      }
    }
  }
}

//----- (00420530) --------------------------------------------------------

void SBoard::RerenderText(int idx)

{
  SHeap<SFrame>::__Tstruct *array; // edx
  int Font;
  int v5;
  SHeap<SFontProp>::__Tstruct *v6; // eax
  HDC CompatibleDC;
  SHeap<SFrame>::__Tstruct *v8; // ecx
  wchar_t *ReducedText; // edi
  unsigned char *v10; // esi
  unsigned int v11;
  int v12;
  unsigned int v13;
  unsigned int v14;
  unsigned char v15; // al
  unsigned char v16; // cl
  unsigned char v17; // ah
  unsigned char v18; // cl
  unsigned char v19; // ah
  bool v20;
  int v21;
  const wchar_t *v22; // eax
  unsigned int v23;
  wchar_t *v24; // ecx
  unsigned int v25;
  unsigned int v26;
  unsigned char v27; // al
  unsigned char v28; // cl
  char v29; // ah
  unsigned char v30; // cl
  char v31; // ah
  int v32;
  int v33;
  int v34;
  int v35;
  int right;
  int v37;
  SHeap<SFontProp>::__Tstruct *v38; // eax
  int v39;
  int v40;
  int bottom;
  unsigned char *v42; // edi
  int v43;
  int v44;
  unsigned char *v45; // esi
  int v46;
  unsigned char *v47; // ecx
  unsigned char v48; // dl
  unsigned char *v49; // ecx
  bool v50; // cc
  unsigned char v51; // al
  int v52;
  int v53;
  int v54;
  int v55;
  int v56;
  int v57;
  int v58;
  int v59;
  int v60;
  int v61;
  int v62;
  int v63;
  unsigned int v64;
  _BYTE *v65; // ecx
  float v66; // xmm0_4
  SHeap<SFontProp>::__Tstruct *v67; // ecx
  int v68;
  float sigma; // xmm0_4
  int v70;
  int v71;
  unsigned char *i; // edx
  int v73;
  int X;
  SBoard *v75; // edi
  unsigned int v76;
  int v77;
  SBoard *v78; // esi
  unsigned int v79;
  unsigned int v80;
  int v81;
  SBoard *v82; // ecx
  int v83;
  void (__stdcall *v84)(HGDIOBJ); // edi
  int v85;
  int v86;
  int v87;
  int v88;
  int v89;
  HGDIOBJ dilute;
  SBitmap v91;
  SBitmap bmap32;
  SBitmap src;
  HGDIOBJ HFont;
  HGDIOBJ ho;
  _BYTE *v96;
  int v97;
  int v98;
  void *pixels;
  float scaleFactor;
  int v101;
  FontBitmap result;
  _BYTE *v103;
  unsigned int v104;
  LPCWSTR lpchText;
  int v106;
  unsigned int v107;
  SBoard *v108;
  HDC hdc;
  bool doubleMode;
  void *block;
  int height;
  int v113;
  int cchText;
  unsigned char v115;
  tagBITMAPINFO bi;
  tagRECT size;
  tagRECT r;
  unsigned char *v119;
  int v120;
  v108 = this;
  array = this->Frames.array;
  v107 = (int)sizeof(SHeap<SFrame>::Element) * idx;
  Font = array[idx].data.Sprite.Font;
  if ( Font >= 0 )
  {
    v5 = Font;
    v6 = this->Fonts.array;
    v101 = v5 * 9268;
    if ( v6[v5].data.typeFace == None )
    {
      this->Gepard->ReleaseTexture(array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Sprite.SepiaColor, 0);
      this->Frames.array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Sprite.SepiaColor = -1;
      return;
    }
    scaleFactor = this->GetTextEffectiveScaleFactor(idx);
    CompatibleDC = CreateCompatibleDC(0);
    hdc = CompatibleDC;
    if ( !CompatibleDC )
      Logger.g->Panic("SBoard::SetText: CreateCompatibleDC failed");
    HFont = this->GetHFont((SFontProp *)((char *)&this->Fonts.array->data + v101), &doubleMode, scaleFactor);
    SelectObject(CompatibleDC, HFont);
    v8 = this->Frames.array;
    cchText = 0;
    ReducedText = (wchar_t *)v8[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Text.ReducedText;
    lpchText = ReducedText;
    if ( !ReducedText )
    {
      ReducedText = (wchar_t *)v8[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Text.Text;
      lpchText = ReducedText;
    }
    v10 = (unsigned char *)ReducedText;
    v11 = (unsigned int)ReducedText + strlen((const char *)ReducedText);
    v113 = v11;
    if ( (unsigned int)ReducedText < v11 )
    {
      v12 = 0;
      while ( 1 )
      {
        cchText = (int)v10;
        if ( (unsigned int)v10 < v11 )
        {
          v14 = *v10++;
          if ( v14 - 193 > 0x1E )
          {
            if ( v14 - 225 > 0xE )
            {
              if ( v14 - 241 > 6
                || (block = (void *)(cchText + 4), cchText + 4 > v11)
                || (v18 = *v10, *v10 < 0x80u)
                || v18 > 0xBFu
                || (v19 = v10[1], (unsigned char)(v19 + 0x80) > 0x3Fu)
                || (v115 = v10[2], (unsigned char)(v115 + 0x80) > 0x3Fu) )
              {
LABEL_29:
                v13 = v14;
                goto LABEL_30;
              }
              v10 = (unsigned char *)block;
              v13 = (v115 & 0x3F) + (((v19 & 0x3F) + ((((v14 & 7) << 6) + (v18 & 0x3F)) << 6)) << 6);
            }
            else
            {
              block = (void *)(cchText + 3);
              if ( cchText + 3 > v11 )
                goto LABEL_29;
              v16 = *v10;
              if ( *v10 < 0x80u )
                goto LABEL_29;
              if ( v16 > 0xBFu )
                goto LABEL_29;
              v17 = v10[1];
              if ( (unsigned char)(v17 + 0x80) > 0x3Fu )
                goto LABEL_29;
              v10 = (unsigned char *)block;
              v13 = (v17 & 0x3F) + ((((v14 & 0xF) << 6) + (v16 & 0x3F)) << 6);
            }
          }
          else
          {
            if ( (unsigned int)(v10 + 1) > v11 )
              goto LABEL_29;
            v15 = *v10;
            if ( *v10 < 0x80u || v15 > 0xBFu )
              goto LABEL_29;
            v13 = ((v14 & 0x1F) << 6) + (v15 & 0x3F);
            v10 = (unsigned char *)(cchText + 2);
          }
        }
        else
        {
          v13 = 0;
        }
LABEL_30:
        v20 = v13 < 0x10000;
        v11 = v113;
        v12 += 2 - v20;
        if ( (unsigned int)v10 >= v113 )
        {
          cchText = v12;
          ReducedText = (wchar_t *)lpchText;
          break;
        }
      }
    }
    v21 = cchText;
    lpchText = (LPCWSTR)operator new[](2 * cchText);
    height = (int)lpchText;
    v22 = &lpchText[v21];
    v23 = v113;
    v104 = (unsigned int)v22;
    while ( (unsigned int)ReducedText < v23 )
    {
      v24 = ReducedText;
      block = ReducedText;
      if ( (unsigned int)ReducedText < v23 )
      {
        v26 = *(unsigned char *)ReducedText;
        ReducedText = (wchar_t *)((char *)ReducedText + 1);
        if ( v26 - 193 > 0x1E )
        {
          if ( v26 - 225 > 0xE )
          {
            if ( v26 - 241 > 6
              || (block = v24 + 2, (unsigned int)(v24 + 2) > v23)
              || (v30 = *(_BYTE *)ReducedText, *(_BYTE *)ReducedText < 0x80u)
              || v30 > 0xBFu
              || (v31 = *((_BYTE *)ReducedText + 1), (unsigned char)(v31 + 0x80) > 0x3Fu)
              || (v115 = *((_BYTE *)ReducedText + 2), (unsigned char)(v115 + 0x80) > 0x3Fu) )
            {
LABEL_53:
              v25 = v26;
              goto LABEL_54;
            }
            ReducedText = (wchar_t *)block;
            v25 = (v115 & 0x3F) + (((v31 & 0x3F) + ((((v26 & 7) << 6) + (v30 & 0x3F)) << 6)) << 6);
          }
          else
          {
            block = (char *)v24 + 3;
            if ( (unsigned int)v24 + 3 > v23 )
              goto LABEL_53;
            v28 = *(_BYTE *)ReducedText;
            if ( *(_BYTE *)ReducedText < 0x80u )
              goto LABEL_53;
            if ( v28 > 0xBFu )
              goto LABEL_53;
            v29 = *((_BYTE *)ReducedText + 1);
            if ( (unsigned char)(v29 + 0x80) > 0x3Fu )
              goto LABEL_53;
            ReducedText = (wchar_t *)block;
            v25 = (v29 & 0x3F) + ((((v26 & 0xF) << 6) + (v28 & 0x3F)) << 6);
          }
        }
        else
        {
          if ( (unsigned int)ReducedText + 1 > v23 )
            goto LABEL_53;
          v27 = *(_BYTE *)ReducedText;
          if ( *(_BYTE *)ReducedText < 0x80u || v27 > 0xBFu )
            goto LABEL_53;
          v25 = ((v26 & 0x1F) << 6) + (v27 & 0x3F);
          ReducedText = (wchar_t *)((char *)block + 2);
        }
      }
      else
      {
        v25 = 0;
      }
LABEL_54:
      v32 = 2 - (v25 < 0x10000);
      if ( height + 2 * v32 <= v104 )
      {
        v33 = v32 - 1;
        if ( v33 )
        {
          if ( v33 == 1 )
          {
            v34 = height;
            *(_WORD *)height = (((v25 - 0x10000) >> 10) & 0x3FF) - 10240;
            *(_WORD *)(v34 + 2) = (v25 & 0x3FF) - 9216;
            height = v34 + 4;
          }
        }
        else
        {
          v35 = height;
          *(_WORD *)height = v25;
          height = v35 + 2;
        }
      }
    }
    size = 0LL;
    DrawTextW(hdc, lpchText, cchText, &size, 0xC20u);
    right = size.right;
    if ( size.right <= 0 || (v37 = size.bottom, size.bottom <= 0) )
    {
      v108->Gepard->ReleaseTexture(v108->Gepard, v108->Frames.array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Sprite.SepiaColor, 0);
      v108->Frames.array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Sprite.SepiaColor = -1;
      v84 = (void (__stdcall *)(HGDIOBJ))DeleteObject;
    }
    else
    {
      if ( doubleMode )
      {
        right = size.right + (size.right & 1);
        v37 = size.bottom + (size.bottom & 1);
      }
      bi.bmiHeader.biSize = 40;
      bi.bmiHeader.biWidth = right;
      bi.bmiHeader.biHeight = v37;
      *(_DWORD *)&bi.bmiHeader.biPlanes = 2097153;
      memset(&bi.bmiHeader.biCompression, 0, 24);
      ho = CreateDIBSection(hdc, &bi, 0, &pixels, 0, 0);
      SelectObject(hdc, ho);
      SetBkColor(hdc, 0);
      v38 = v108->Fonts.array;
      v39 = *(TypeFace *)((char *)&v38->data.typeFace + v101);
      if ( v39 == 3
        || v39 == 4
        || v39 == 5
        || (v40 = *(FontEffect *)((char *)&v38->data.fontEffect + v101)) == 0
        || v40 == 4 )
      {
        SetTextColor(hdc, 0xFFFFFFu);
      }
      else
      {
        SetTextColor(hdc, 0xC0C0C0u);
      }
      r.top = 0;
      r.bottom = right;
      r.right = 0;
      v119 = (unsigned char *)v37;
      DrawTextW(hdc, lpchText, cchText, (LPRECT)&r.top, 0x820u);
      GdiFlush();
      FontBitmap::FontBitmap((FontBitmap *)&r, right, v37);
      bottom = r.bottom;
      v42 = v119;
      v43 = r.right;
      v120 = 0;
      block = v119;
      cchText = (int)v119;
      height = r.bottom;
      v106 = r.right;
      if ( r.bottom > 0 )
      {
        v103 = (_BYTE *)r.bottom;
        v104 = -4 * r.right;
        v44 = (int)pixels + r.right * (4 * r.bottom - 4);
        v113 = v44;
        do
        {
          v45 = (unsigned char *)v44;
          if ( v43 > 0 )
          {
            v46 = v43;
            do
            {
              v47 = v45 + 1;
              if ( *v45 > v45[1] )
                v47 = v45;
              v48 = *v47;
              v49 = &v115;
              v50 = v48 <= v45[2];
              v115 = v48;
              if ( v50 )
                v49 = v45 + 2;
              v45 += 4;
              v51 = *v49;
              v52 = cchText;
              *(_BYTE *)cchText = v51;
              cchText = v52 + 1;
              --v46;
            }
            while ( v46 );
            v43 = v106;
            v44 = v113;
            bottom = (int)v103;
          }
          v44 += v104;
          --bottom;
          v113 = v44;
          v103 = (_BYTE *)bottom;
        }
        while ( bottom );
        v42 = (unsigned char *)block;
        bottom = height;
      }
      if ( doubleMode )
      {
        if ( (v43 & 1) != 0 || (bottom & 1) != 0 )
          Logger.g->Panic("FontBitmap::Reduce2: Not a multiple of 2");
        v53 = v43 >> 1;
        height = bottom >> 1;
        cchText = v43 >> 1;
        result.X = 0;
        result.Y = 0;
        result.Width = v43 >> 1;
        result.Height = bottom >> 1;
        v96 = operator new[]((bottom >> 1) * (v43 >> 1));
        v103 = v96;
        if ( height > 0 )
        {
          v54 = v106;
          v55 = height;
          v56 = cchText;
          v57 = (int)v42;
          v113 = (int)v42;
          v104 = height;
          do
          {
            v98 = v56;
            v58 = v57;
            if ( v56 )
            {
              v59 = v57 + v54;
              v60 = 1 - v54;
              v61 = v56;
              v97 = v60;
              do
              {
                v62 = *(unsigned char *)(v60 + v59);
                v58 += 2;
                v63 = *(unsigned char *)(v59 + 1);
                v59 += 2;
                v64 = *(unsigned char *)(v59 - 2) + v63 + v62 + 2 + *(unsigned char *)(v58 - 2);
                v65 = v103;
                *v103 = v64 >> 2;
                v60 = v97;
                v103 = v65 + 1;
                --v61;
              }
              while ( v61 );
              v54 = v106;
              v57 = v113;
              v56 = cchText;
              v55 = v104;
            }
            v57 += 2 * v54;
            --v55;
            v113 = v57;
            v104 = v55;
          }
          while ( v55 );
          v53 = cchText;
        }
        if ( block )
          delete[] block;
        v42 = v96;
        r.left = 0;
        r.top = 0;
        v106 = v53;
        r.right = v53;
        r.bottom = height;
        v119 = v96;
        result.Data = 0;
        FontBitmap::~FontBitmap(&result);
      }
      else
      {
        v53 = v106;
      }
      cchText = 0;
      bmap32 = SBitmap();
      v66 = 1.0f;
      v67 = v108->Fonts.array;
      v68 = *(TypeFace *)((char *)&v67->data.typeFace + v101);
      if ( v68 == 3 || v68 == 5 )
        v66 = 0.5f;
      sigma = v66 * scaleFactor;
      v70 = *(FontEffect *)((char *)&v67->data.fontEffect + v101);
      *(float *)&v113 = sigma;
      switch ( v70 )
      {
        case 0:
          new (&src) SBitmap(v53, height, D3DFMT_A8R8G8B8, 0);
          v71 = height * v106;
          for ( i = src.Data; v71; --v71 )
          {
            i += 4;
            v73 = *v42++ << 24;
            *((_DWORD *)i - 1) = v73 | 0xFFFFFF;
          }
          goto LABEL_105;
        case 1:
          FontBitmap::Blur((FontBitmap *)&r, &result, sigma, 1.0);
          v85 = (int)(float)(*(float *)&v113 * -4.0);
          if ( result.X - v85 < 0 )
            Logger.g->Panic("FontBitmap::NormalWithShadow: Unsupported parameters");
          cchText = 0;
          FontBitmap::CombineLightAndShadow(&src, (const FontBitmap *)&r, &result, result.X - v85);
          if ( result.Data )
            operator delete(result.Data);
LABEL_105:
          bmap32 = &src;
          src.~SBitmap();
          goto LABEL_106;
        case 2:
          FontBitmap::Blur((FontBitmap *)&r, &result, sigma * 1.5, 0.80000001);
          FontBitmap::Add(&result, (const FontBitmap *)&r, -result.X);
          FontBitmap::Blur((FontBitmap *)&r, (FontBitmap *)&src.Start, *(float *)&v113, 1.0);
          X = result.X;
          v86 = (int)(float)(*(float *)&v113 * -4.0);
          if ( src.Start - v86 - result.X < 0 )
            Logger.g->Panic("FontBitmap::NormalWithShadow: Unsupported parameters");
          cchText = result.Y;
          FontBitmap::CombineLightAndShadow(&v91, &result, (const FontBitmap *)&src.Start, src.Start - v86 - result.X);
          if ( src.Data )
            operator delete(src.Data);
          if ( result.Data )
            operator delete(result.Data);
          bmap32 = &v91;
          v91.~SBitmap();
          break;
        case 3:
          FontBitmap::Blur((FontBitmap *)&r, &result, sigma * 1.5, 0.80000001);
          v87 = result.X;
          FontBitmap::Add(&result, (const FontBitmap *)&r, -result.X);
          FontBitmap::Blur((FontBitmap *)&r, (FontBitmap *)&src.Start, *(float *)&v113, 1.0);
          v88 = (int)(float)(*(float *)&v113 + *(float *)&v113);
          v89 = src.Start - (int)(float)(*(float *)&v113 * -4.0) - v88 - v87;
          if ( v89 < 0 )
            Logger.g->Panic("FontBitmap::NormalWithShadow: Unsupported parameters");
          v113 = v88 + v87;
          cchText = v88 + result.Y;
          FontBitmap::CombineLightAndShadow(&v91, &result, (const FontBitmap *)&src.Start, v89);
          if ( src.Data )
            operator delete(src.Data);
          if ( result.Data )
            operator delete(result.Data);
          goto LABEL_126;
        case 4:
          FontBitmap::BoxBlur((FontBitmap *)&r, &result, (int)(float)(sigma * 1.5));
          v113 = result.X;
          cchText = result.Y;
 FontBitmap::CombineEmboss( &v91, (const FontBitmap *)&r,
            &result,
            (float)(int)(float)(sigma * 1.5) * 0.69999999);
          if ( result.Data )
            operator delete(result.Data);
LABEL_126:
          bmap32 = &v91;
          v91.~SBitmap();
          X = v113;
          break;
        default:
LABEL_106:
          X = 0;
          break;
      }
      v75 = v108;
      v76 = v107;
      dilute = ho;
      v108->Frames.array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Text.TrueTypeWidth = bmap32.Width;
      *(int *)((char *)&v75->Frames.array->data.Text.TrueTypeHeight + v76) = bmap32.Height;
      v77 = cchText;
      *(int *)((char *)&v75->Frames.array->data.Text.TrueTypeX + v76) = X;
      v78 = v75;
      *(int *)((char *)&v75->Frames.array->data.Text.TrueTypeY + v76) = v77;
      DeleteObject(dilute);
      v79 = v107;
      v78->Gepard->ReleaseTexture(v78->Gepard, v78->Frames.array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Sprite.SepiaColor, 0);
      v80 = (unsigned int)v78->Frames.array + v79;
      v81 = SGepard::CreateTextureFromBitmap(v108->Gepard, "<dynamic>_hq.tga", &bmap32, 0);
      v82 = v108;
      v83 = v101;
      v84 = (void (__stdcall *)(HGDIOBJ))DeleteObject;
      *(_DWORD *)(v80 + 80) = v81;
      if ( *(FontEffect *)((char *)&v82->Fonts.array->data.fontEffect + v83) == Arial )
        SGepard::ChangeTextureAlphaType(v82->Gepard, v82->Frames.array[v107 / (int)sizeof(SHeap<SFrame>::Element)].data.Text.TrueTypeTextureIndex, 4);
      bmap32.~SBitmap();
      FontBitmap::~FontBitmap((FontBitmap *)&r);
    }
    v84(HFont);
    DeleteDC(hdc);
    operator delete[]((void *)lpchText);
  }
}

#endif // disabled Render + RerenderText

//----- (00421220) --------------------------------------------------------

void SBoard::ResizeFrame(int idx, int width, int height)
{
  SHeap<SFrame>::__Tstruct *array = this->Frames.array;
  int Sibling = array[idx].data.Child;
  while (Sibling >= 0) {
    int Flags = array[Sibling].data.Flags;
    if (Flags & 1) {
      int dx = width / 2 - array[idx].data.Width / 2;
      array[Sibling].data.X = (float)dx + array[Sibling].data.X;
    } else if (Flags & 2) {
      int dx = width - array[idx].data.Width;
      array[Sibling].data.X = (float)dx + array[Sibling].data.X;
    }
    if (Flags & 4) {
      int dy = height / 2 - array[idx].data.Height / 2;
      array[Sibling].data.Y = (float)dy + array[Sibling].data.Y;
    } else if (Flags & 8) {
      int dy = height - array[idx].data.Height;
      array[Sibling].data.Y = (float)dy + array[Sibling].data.Y;
    }
    Sibling = array[Sibling].data.Sibling;
  }
  array[idx].data.Width  = width;
  array[idx].data.Height = height;
  if (array[idx].data.Type == FT_SPRITE) {
    array[idx].data.Sprite.Glyph.Dest.Left   = -0.5f;
    array[idx].data.Sprite.Glyph.Dest.Top    = -0.5f;
    array[idx].data.Sprite.Glyph.Dest.Right  = (float)width  - 0.5f;
    array[idx].data.Sprite.Glyph.Dest.Bottom = (float)height - 0.5f;
  }
}

//----- (00421340) --------------------------------------------------------

void SBoard::SetBoxColor(int idx, unsigned int color)

{
  SHeap<SFrame>::__Tstruct *array; // ecx
  array = this->Frames.array;
  if ( array[idx].data.Type != FT_BOX )
    Logger.g->Panic("SBoard::SetBoxColor: Not a box frame");
  array[idx].data.Sprite.Font = color;
}

//----- (00421370) --------------------------------------------------------

void SBoard::SetCursor(int idx, int x, int y)

{
  tagPOINT *CursorHotspots; // esi
  int v5;
  int CursorGlyph;
  CursorHotspots = this->CursorHotspots;
  if ( CursorHotspots )
  {
    v5 = idx;
    CursorGlyph = this->CursorGlyph;
    if ( idx < 0 || idx >= this->NumCursors )
    {
      this->CursorGlyph = -1;
      v5 = -1;
    }
    else
    {
      this->CursorGlyph = idx;
      this->CursorX = x - CursorHotspots[idx].x;
      this->CursorY = y - CursorHotspots[idx].y;
    }
    this->ForceSoftwareCursor = v5 == 9;
    if ( CursorGlyph != v5 )
      this->ApplyHardwareCursor();
  }
}

//----- (004213D0) --------------------------------------------------------

void SBoard::SetMinimapFont(int frameIdx, int font)

{
  unsigned int v3;
  SHeap<SFrame>::__Tstruct *array; // ecx
  SHeap<SFontProp>::__Tstruct *v6; // edx
  SHeap<SFontProp>::__Tstruct *v7; // eax
  SHeap<SFrame>::__Tstruct *v8; // ecx
  v3 = frameIdx;
  array = this->Frames.array;
  if ( array[frameIdx].data.Type != FT_MINIMAP )
    Logger.g->Panic("SBoard::SetMinimapFont: Not a minimap frame");
  this->ReleaseFont(array[v3].data.Sprite.Font);
  if ( font < 0 )
  {
    this->Frames.array[v3].data.Sprite.Font = -1;
  }
  else
  {
    if ( font < this->Fonts.size )
    {
      v6 = this->Fonts.array;
      if ( v6[font].use == 0x7FFFFFFF )
        ++v6[font].data.RefCount;
    }
    this->Frames.array[v3].data.Sprite.Font = font;
    // IDA decomp wrote the 36-byte SGlyph payload via three union-aliased
    // addresses (Anim.TextureIndex, Scaler+20, Text.TrueTypeHeight) that
    // all happened to overlap Minimap.Glyph on x86. On x64 SFrameAnim's
    // leading SAnimation* is 8 bytes (not 4), so Anim.TextureIndex shifts
    // from union+4 to union+8, and SFrameText's char* fields shift the
    // tail down 12 bytes — only the middle write hits the right offset
    // and Minimap.Glyph.Dest.Bottom stays 0 from heap memset, giving a
    // zero-height quad and a black thumbnail. Same pattern fixed for
    // Sprite.Glyph in c1dac52; this is the minimap sister.
    this->Frames.array[v3].data.Minimap.Glyph = this->Fonts.array[font].data.Glyphs[0];
  }
}

//----- (00421470) --------------------------------------------------------

void SBoard::SetMinimapRotation(int frameIdx, float rotation)

{
  SHeap<SFrame>::__Tstruct *array; // ecx
  array = this->Frames.array;
  if ( array[frameIdx].data.Type != FT_MINIMAP )
    Logger.g->Panic("SBoard::SetMinimapRotation: Not a minimap frame");
  array[frameIdx].data.Minimap.Rotation = rotation;
}

//----- (004214B0) --------------------------------------------------------

void SBoard::SetPixelRounding(bool round)

{
  this->roundPixels = round;
}

//----- (004214C0) --------------------------------------------------------

void SBoard::SetScaleFactor(int idx, float scaleFactor)

{
  int Child;
  SHeap<SFrame>::__Tstruct *i; // eax
  int v6;
  SFrameType Type;
  SHeap<SFrame>::__Tstruct *array; // ecx
  Child = idx;
  this->Frames.array[idx].data.Scaler.ScaleFactor = scaleFactor;
  for ( i = this->Frames.array; ; i = array )
  {
    while ( 1 )
    {
      v6 = Child;
      Type = i[Child].data.Type;
      if ( Type == FT_TEXT || Type == FT_FIXTEXT )
      {
        this->RerenderText(Child);
        i = this->Frames.array;
      }
      if ( i[v6].data.Child < 0 )
        break;
      Child = i[v6].data.Child;
    }
    array = i;
    if ( i[v6].data.Sibling < 0 )
      break;
LABEL_10:
    if ( Child < 0 )
      return;
    Child = array[Child].data.Sibling;
  }
  while ( 1 )
  {
    Child = i[v6].data.Parent;
    if ( Child < 0 )
      break;
    array = this->Frames.array;
    v6 = Child;
    if ( array[Child].data.Sibling >= 0 )
      goto LABEL_10;
  }
}

// PANZERS 0x6cb0c0
// HD SBoard +0x58: stores the virtual size in the scaler frame (+0x28/+0x2c
// of the 0x5c-byte HD frame record). The render (0x6c7150) multiplies the
// pushed x scale by frame width / virtual width and the y scale by frame
// height / virtual height.
void SBoard::SetVirtualSize(int idx, int width, int height)
{
  if ( idx < 0 || idx >= this->Frames.size || this->Frames.array[idx].use != 0x7FFFFFFF )
    Logger.g->Panic("SBoard::SetVirtualSize: invalid frame (%d)", idx);
  this->Frames.array[idx].data.Scaler.VirtualWidth = width;
  this->Frames.array[idx].data.Scaler.VirtualHeight = height;
}

//----- (00421540) --------------------------------------------------------

void SBoard::SetSpriteGlyph(int idx, int font, int glyph)

{
  unsigned int v4;
  SHeap<SFrame>::__Tstruct *array; // ecx
  SFrameType Type;
  SHeap<SFontProp>::__Tstruct *v8; // edx
  SHeap<SFrame>::__Tstruct *v12; // ecx
  v4 = idx;
  array = this->Frames.array;
  Type = array[idx].data.Type;
  if ( Type != FT_SPRITE && Type != FT_SPRITE_9SLICE )
    Logger.g->Panic("SBoard::SetSpriteGlyph: Not a sprite frame");
  this->ReleaseFont(array[v4].data.Sprite.Font);
  if ( font < 0 )
  {
    this->Frames.array[v4].data.Sprite.Font = -1;
    this->Frames.array[v4].data.Width = 0;
    this->Frames.array[v4].data.Height = 0;
  }
  else
  {
    if ( font < this->Fonts.size )
    {
      v8 = this->Fonts.array;
      if ( v8[font].use == 0x7FFFFFFF )
      {
        ++v8[font].data.RefCount;
      }
    }
    this->Frames.array[v4].data.Sprite.Font = font;
    // IDA decomp wrote the 36-byte SGlyph payload through three
    // union-aliased addresses (Anim.TextureIndex, Scaler+20,
    // Text.TrueTypeHeight) that all happened to overlap Sprite.Glyph on
    // x86. On x64 SFrameAnim's leading SAnimation* is 8 bytes (not 4) and
    // SFrameText's char* fields shift the tail down 12 bytes, so two of
    // the three writes land at wrong offsets and Sprite.Glyph.Dest.Bottom
    // stays 0 — every sprite computes Width/Height as 0 and renders
    // invisibly. Typed assignment hits Sprite.Glyph on both archs.
    this->Frames.array[v4].data.Sprite.Glyph = this->Fonts.array[font].data.Glyphs[glyph];
    v12 = this->Frames.array;
    if ( v12[v4].data.Type == FT_SPRITE_9SLICE )
    {
      v12[v4].data.Width = 0;
      this->Frames.array[v4].data.Height = 0;
    }
    else
    {
      v12[v4].data.Width = (int)((float)(v12[v4].data.Sprite.Glyph.Dest.Right - v12[v4].data.Sprite.Glyph.Dest.Left)
                               + 0.5);
      this->Frames.array[v4].data.Height = (int)((float)(this->Frames.array[v4].data.Sprite.Glyph.Dest.Bottom
                                                       - this->Frames.array[v4].data.Sprite.Glyph.Dest.Top)
                                               + 0.5);
    }
  }
}

//----- (00421670) --------------------------------------------------------

void SBoard::SetSpriteSepiaFilter(int idx, bool enable, unsigned int color)

{
  SHeap<SFrame>::__Tstruct *array; // ecx
  unsigned int v5;
  array = this->Frames.array;
  if ( array[idx].data.Type != FT_SPRITE )
    Logger.g->Panic("SBoard::SetSpriteGlyph: Not a sprite frame");
  v5 = 0;
  if ( enable )
    v5 = color;
  array[idx].data.Sprite.SepiaColor = v5;
}

//----- (004216B0) --------------------------------------------------------

void SBoard::SetText(int idx, int font, int align, const char *text)

{
  int v5;
  SHeap<SFrame>::__Tstruct *array; // ecx
  SFrameType Type;
  char v9; // dl
  SHeap<SFontProp>::__Tstruct *v10; // ecx
  char *v11; // eax
  int v12;
  SHeap<SFrame>::__Tstruct *v13; // esi
  double v14; // st7
  SHeap<SFrame>::__Tstruct *v15; // eax
  char *ReducedText; // eax
  size_t v17;
  SHeap<SFrame>::__Tstruct *v18; // eax
  SHeap<SFrame>::__Tstruct *v19;
  int ellipse;
  int height;
  int width;
  float scaleFactor;
  v5 = idx;
  array = this->Frames.array;
  Type = array[idx].data.Type;
  if ( Type != FT_TEXT && Type != FT_FIXTEXT )
    Logger.g->Panic("SBoard::SetText: Not a text frame");
  v9 = 0;
  ellipse = array[v5].data.Sprite.Font;
  if ( ellipse != font )
  {
    this->ReleaseFont(ellipse);
    if ( font >= 0 && font < this->Fonts.size )
    {
      v10 = this->Fonts.array;
      if ( v10[font].use == 0x7FFFFFFF )
      {
        ++v10[font].data.RefCount;
      }
    }
    v9 = 1;
    this->Frames.array[v5].data.Sprite.Font = font;
    array = this->Frames.array;
  }
  array[v5].data.Text.Align = align;
  if ( v9 )
    goto LABEL_15;
  v11 = this->Frames.array[v5].data.Text.Text;
  if ( !v11 || !text )
    goto LABEL_15;
  v12 = strcmp(v11, text);
  if ( v12 )
    v12 = v12 < 0 ? -1 : 1;
  if ( v12 )
  {
LABEL_15:
    // The IDA decomp stored char* pointers through the SFrameAnim union
    // alias (`Anim.EndTime` for `Text.Text`, `Anim.FrameTime` for
    // `Text.ReducedText`) and truncated them with `(unsigned int)`. On
    // x86 the alias is exact (Text.Text starts at union offset 12, same
    // as EndTime; both fields are 4 bytes wide because pointers and
    // unsigned int are both 4 bytes). On x64 Text.Text starts at union
    // offset 16 (after 4 bytes of alignment padding) and is 8 bytes
    // wide, but EndTime/FrameTime are still only 4 bytes — so each
    // SetText only updated half of the pointer, leaving the other half
    // either zero or stale from a previous strdup. The visible bug:
    // every text frame appeared to render whichever string was strdup'd
    // most recently because the high 32 bits decided what region of
    // memory a stale low-32 actually pointed into. Use the typed fields
    // directly so the full pointer is written.
    free(this->Frames.array[v5].data.Text.Text);
    v13 = this->Frames.array;
    v13[v5].data.Text.Text = _strdup(text);
    v14 = this->GetTextEffectiveScaleFactor(idx);
    v15 = this->Frames.array;
    scaleFactor = (float)(v14);

    if ( v15[v5].data.Type == FT_TEXT )
    {
      this->GetTextExtent(
        font,
        text,
        strlen(text),
        &this->Frames.array[v5].data.Width,
        &this->Frames.array[v5].data.Height,
        scaleFactor);
      this->Frames.array[v5].data.Text.Width = this->Frames.array[v5].data.Width;
      this->RerenderText(idx);
    }
    else
    {
      ReducedText = v15[v5].data.Text.ReducedText;
      if ( ReducedText )
      {
        ::operator delete(ReducedText);
        this->Frames.array[v5].data.Text.ReducedText = nullptr;
      }
      v17 = strlen(text);
      this->GetTextExtent(font, text, v17, &width, &height, scaleFactor);
      v18 = this->Frames.array;
      if ( width > v18[v5].data.Width )
      {
        this->GetTextExtent(font, "...", 3, &ellipse, &height, scaleFactor);
        v19 = this->Frames.array;
        if ( ellipse > v19[v5].data.Width )
        {
          v19[v5].data.Text.ReducedText = _strdup("");
        }
        else
        {
          if ( v17 )
          {
            while ( 1 )
            {
              this->GetTextExtent(font, text, --v17, &width, &height, scaleFactor);
              if ( width + ellipse <= this->Frames.array[v5].data.Width )
                break;
              if ( !v17 )
                goto LABEL_25;
            }
          }
          else
          {
LABEL_25:
            --v17;
          }
          this->Frames.array[v5].data.Text.ReducedText = (char *)operator new[](v17 + 4);
          if ( v17 )
            memcpy(this->Frames.array[v5].data.Text.ReducedText, text, v17);
          // 3026478 = 0x002E2E2E = "...\0"  (three ASCII '.' + null term)
          *(_DWORD *)(this->Frames.array[v5].data.Text.ReducedText + v17) = 3026478;
        }
        this->GetTextExtent(
          font,
          this->Frames.array[v5].data.Text.ReducedText,
          strlen(this->Frames.array[v5].data.Text.ReducedText),
          &this->Frames.array[v5].data.Text.Width,
          &height,
          scaleFactor);
        this->RerenderText(idx);
      }
      else
      {
        v18[v5].data.Text.Width = width;
        this->RerenderText(idx);
      }
    }
  }
}

//----- (00421990) --------------------------------------------------------

void SBoard::SetTextColor(int idx, unsigned int color)

{
  SHeap<SFrame>::__Tstruct *array; // ecx
  SFrameType Type;
  array = this->Frames.array;
  Type = array[idx].data.Type;
  if ( Type != FT_TEXT && Type != FT_FIXTEXT )
    Logger.g->Panic("SBoard::SetTextColor: Not a text frame");
  array[idx].data.Sprite.Glyph.Width = color | 0xFF000000;
}

//----- (004219D0) --------------------------------------------------------

void SBoard::SetTextF(int idx, int font, int align, const char *format, ...)

{
  va_list va;
  va_start(va, format);
  this->SetTextV(idx, font, align, format, va);
}

//----- (004219F0) --------------------------------------------------------

void SBoard::SetTextV(int idx, int font, int align, const char *format, char *args)

{
  char buf[512];
  vsprintf(buf, format, (va_list)args);
  this->SetText(idx, font, align, buf);
}

//----- (00421A60) --------------------------------------------------------

void SBoard::ShowFrame(int idx, bool visible)

{
  SHeap<SFrame>::__Tstruct *v3; // edx
  unsigned int v4;
  v3 = &this->Frames.array[idx];
  v4 = v3->data.Flags & 0xFFFFFFEF;
  if ( visible )
    v4 = v3->data.Flags | 0x10;
  v3->data.Flags = v4;
}

//----- (00421A90) --------------------------------------------------------

void SBoard::StartAnim(int idx, const char *filename, int flags, int duration, int frameRate)

{
  unsigned int v8;
  SAnimation *v9; // eax
  SHeap<SFrame>::__Tstruct *array; // ecx
  int v13;
  SHeap<SFrame>::__Tstruct *v14; // ecx
  int v15;
  unsigned int v16;
  SHeap<SFrame>::__Tstruct *v17; // esi
  unsigned int v18;
  int v19;
  unsigned int v20;
  SHeap<SFrame>::__Tstruct *v21; // ecx
  int v22;
  int v23;
  const char *filenamea;
  v8 = idx;
  v23 = idx;
  if ( this->Frames.array[idx].data.Type != FT_ANIM )
    Logger.g->Panic("SBoard::SetAnim: Not an anim frame");
  this->StopAnim(idx);
  v9 = new SAnimation();
  this->Frames.array[v8].data.Anim.Anim = v9;
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "StartAnim: loading '%s' frameRate=%d", filename, frameRate);
#endif
  v9->LoadANI(0, filename, "SBoard::SetAnim");
#ifdef HD_DEBUG_BOARD
  Logger.g->Log(0, "StartAnim: loaded W=%d H=%d Frames=%d", v9->Width, v9->Height, v9->Frames);
#endif
  array = this->Frames.array;
  v13 = array[v8].data.Anim.Anim->Width;
  array[v8].data.Width = v13;
  v14 = this->Frames.array;
  v15 = v14[v8].data.Anim.Anim->Height;
  v14[v8].data.Height = v15;
  v16 = (v13 + 3) & 0xFFFFFFFC;
  filenamea = (const char *)v16;
  v18 = (v15 + 3) & 0xFFFFFFFC;
  v19 = 4;
  v22 = v18;
  if ( (flags & 4) != 0 )
    v19 = 32;
  v17 = &this->Frames.array[v8];
  // Use Anim.TextureIndex directly. IDA decomp wrote via Sprite.Glyph.Width
  // because on x86 the two fields alias at union offset 4. On x64 Anim.Anim
  // is 8 bytes, so Sprite.Glyph.Width (union offset 4) aliases with the
  // UPPER 4 bytes of Anim.Anim — writing stomped the pointer we just stored.
  v17->data.Anim.TextureIndex = this->Gepard->CreateEmptyTexture(v16, v18, v19, 0, 0);
  // IDA decomp used data.Text.Align and data.Text.Width as aliases for
  // Anim.LastFrame and Anim.Flags (matching x86 union offsets). On x64 those
  // Text fields shift (SFrameText has pointers) so Text.Align lands on top of
  // Anim.TextureIndex and wipes the texture index we just stored. Use the
  // real SFrameAnim fields.
  this->Frames.array[v23].data.Anim.LastFrame = this->Gepard->AnimTime;
  this->Frames.array[v23].data.Anim.Flags = flags;
  if ( duration )
    v20 = duration + this->Gepard->AnimTime;
  else
    v20 = 0;
  this->Frames.array[v23].data.Anim.EndTime = v20;
  // UV max values stored in Anim.MaxU/MaxV (was Sprite.Glyph.Dest.Left/Top on x86
  // via union alias — Dest.Left is at union offset 24 which collides with Anim.Flags
  // on x64).
  this->Frames.array[v23].data.Anim.MaxU = (float)this->Frames.array[v23].data.Width
                                         / (float)(int)filenamea;
  this->Frames.array[v23].data.Anim.MaxV = (float)this->Frames.array[v23].data.Height / (float)v22;
  this->Frames.array[v23].data.Anim.FrameTime = 1000 / frameRate;
  v21 = this->Frames.array;
  // Test the "show parent while playing" bit on Anim.Flags directly. The IDA
  // decomp wrote `*(_BYTE *)(&data.Scaler + 5)` — pointer arithmetic on
  // SFrameScaler* advances by 5 * sizeof(SFrameScaler) = 20 bytes, which on
  // x86 lands on byte 0 of Anim.Flags (union offset 20). On x64 the 8-byte
  // Anim.Anim shifts the layout and offset 20 is now Anim.FrameTime byte 0,
  // so the test reads garbage and ShowFrame is never called — the parent
  // container stays hidden and the reportanim never appears on screen.
  if ( (v21[v23].data.Anim.Flags & 2) != 0 )
    this->ShowFrame(v21[v23].data.Parent, 1);
}

//----- (00421C40) --------------------------------------------------------

void SBoard::StopAnim(int idx)

{
  unsigned int v2;
  SHeap<SFrame>::__Tstruct *array; // edx
  SHeap<SFrame>::__Tstruct *v5; // ecx
  SAnimation *Anim; // ebx
  v2 = idx;
  array = this->Frames.array;
  if ( array[idx].data.Type != FT_ANIM )
    Logger.g->Panic("SBoard::SetAnim: Not an anim frame");
  // Use Anim.Anim / Anim.TextureIndex directly. On x86 the decomp relied on
  // Sprite.Font aliasing with the 4-byte Anim.Anim pointer (offset 0) and
  // Sprite.Glyph.Width aliasing with Anim.TextureIndex (offset 4). On x64
  // Anim.Anim is 8 bytes wide, so Sprite.Font=0 only clears the low half
  // of the pointer and Sprite.Glyph.Width is the upper half — the decomp
  // paths read/write the wrong memory there.
  if ( array[v2].data.Anim.Anim )
  {
    this->Gepard->ReleaseTexture(array[v2].data.Anim.TextureIndex, 0);
    v5 = this->Frames.array;
    Anim = v5[v2].data.Anim.Anim;
    if ( Anim )
    {
      delete Anim;
      this->Frames.array[v2].data.Anim.Anim = nullptr;
      v5 = this->Frames.array;
    }
    // Mirror of the StartAnim flag test above — read Anim.Flags directly so
    // x64 doesn't pick up Anim.FrameTime byte 0 instead.
    if ( (v5[v2].data.Anim.Flags & 2) != 0 )
      this->ShowFrame(v5[v2].data.Parent, 0);
  }
}

//----- (00421CF0) --------------------------------------------------------

void SBoard::UnloadCursorSet()

{
  tagPOINT *CursorHotspots; // eax
  HICON__ **CursorIcons; // eax
  this->ReleaseFont(this->CursorFont);
  CursorHotspots = this->CursorHotspots;
  this->CursorFont = -1;
  if ( CursorHotspots )
  {
    delete[] (POINT *)CursorHotspots;
    this->CursorHotspots = 0;
  }
  CursorIcons = this->CursorIcons;
  if ( CursorIcons )
  {
    delete[] CursorIcons;
    this->CursorIcons = 0;
  }
}

// PANZERS 0x6c61a0
// SBoard::LoadFontFileFont: loads a Panzers ".font" file (HD SBoard vtable
// slot +0x6C). SWINE has no equivalent; the Panzers menu fonts
// (menu/fonts/sans_serif_new/*_hq.font, strings at 0x80a558..) all go
// through it.
//
// File layout (Stormregion chunk file):
//   signature  "Sr\x1a\x1b\r\n\x87\n"
//   chunk      'FONT' (0x544E4F46), size 0xE08
//     int      version 'v100' (0x30303176)
//     int      line height (14 for sans_serif_14)
//     short    glyph[256][7] = { srcX, srcY, w, h, dstX, dstY, advance }
// The bitmap has the same name with the extension replaced by "tga"
// (SetExtension with the string at 0x8179d0).
//
// Differences from the HD body, on purpose:
// - HD throws a `const char*` ("Not a font file", "Unsupported font file
//   version") that its catch turns into Panic("SBoard::LoadFontFileFont:
//   Error loading %s: %s"); here the Panic is called directly.
// - HD keeps raw destination rects (0..w); SWINE's SBoard::Render expects the
//   -0.5 texel offset that its own loaders (LoadFixedFont/LoadSingleFont)
//   bake in, so it is baked in here too.
// - The line height goes into SFontProp::fontSize (HD keeps it in its own
//   field at +0x20 of the 0x4860-byte HD SFontProp; SWINE's is 0x2430).
int SBoard::LoadFontFileFont(const char *filename)
{
  short raw[256][7];
  const char *err = nullptr;
  int lineHeight = 0;

  SStream *s = FileSystem.OpenRead(filename, "SBoard::LoadFontFileFont");
  s->ReadSignature();
  if ( s->ReadChunkHeader() != 0x544E4F46 )      // 'FONT'
    err = "Not a font file";
  else if ( s->ReadInt() != 0x30303176 )         // 'v100'
    err = "Unsupported font file version";
  else
  {
    lineHeight = s->ReadInt();
    s->Read(raw, sizeof(raw));                   // 0xE00 bytes
  }
  s->Release();
  if ( err )
    Logger.g->Panic("SBoard::LoadFontFileFont: Error loading %s: %s", filename, err);

  char texname[MAX_PATH];
  strncpy(texname, filename, sizeof(texname) - 5);
  texname[sizeof(texname) - 5] = 0;
  char *dot = strrchr(texname, '.');
  char *slash = strrchr(texname, '/');
  char *bslash = strrchr(texname, '\\');
  if ( dot && dot > slash && dot > bslash )
    strcpy(dot + 1, "tga");
  else
    strcat(texname, ".tga");

  SBitmap bmap;
  bmap.LoadTGA(texname, (char *)"SBoard::LoadFontFileFont");
  int tex_width, tex_height;
  this->Gepard->RoundToTextureSize(bmap.Width, bmap.Height, &tex_width, &tex_height);

  SFontProp *fp = new SFontProp();               // value-init: all zero
  fp->RefCount = 1;
  fp->FirstGlyph = 0;
  fp->LastGlyph = 255;
  fp->fontSize = lineHeight;
  const float su = 1.0f / (float)tex_width;
  const float sv = 1.0f / (float)tex_height;
  for ( int i = 0; i < 256; ++i )
  {
    const short *d = raw[i];
    SGlyph *g = &fp->Glyphs[i];
    g->Width = d[6];
    g->Src.Left = (float)d[0] * su;
    g->Src.Top = (float)d[1] * sv;
    g->Src.Right = (float)(d[0] + d[2]) * su;
    g->Src.Bottom = (float)(d[1] + d[3]) * sv;
    g->Dest.Left = (float)d[4] - 0.5f;
    g->Dest.Top = (float)d[5] - 0.5f;
    g->Dest.Right = (float)(d[4] + d[2]) - 0.5f;
    g->Dest.Bottom = (float)(d[5] + d[3]) - 0.5f;
  }

  if ( bmap.Width == tex_width && bmap.Height == tex_height )
  {
    fp->TextureIndex = this->Gepard->CreateTextureFromBitmap(texname, &bmap, 0);
  }
  else
  {
    SBitmap padded(tex_width, tex_height, bmap.Format, 0);
    memset(padded.Data, 0, padded.Size);
    padded.BitBlt(0, 0, bmap.Width, bmap.Height, &bmap, 0, 0);
    fp->TextureIndex = this->Gepard->CreateTextureFromBitmap(texname, &padded, 0);
  }

  int idx = this->Fonts.Add();
  memcpy(&this->Fonts.array[idx].data, fp, sizeof(SFontProp));
  delete fp;
  return idx;
}

// Layout tripwire against the HD exe (see panzers_hd_sizes.h).
#include "panzers_hd_sizes.h"
PANZERS_LAYOUT_CHECK(SBoard, SBOARD);
PANZERS_LAYOUT_CHECK(SFontProp, SFONTPROP);
PANZERS_LAYOUT_CHECK(SFrame, SFRAME);

