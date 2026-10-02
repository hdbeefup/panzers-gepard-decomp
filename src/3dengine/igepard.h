// 3dengine/igepard.h
// SIGepard — base renderer interface with virtual methods
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_IGEPARD_H
#define DENGINE3_IGEPARD_H

#include "core_common.h"
#include "darray.h"
#include "chain.h"

#include <d3d9.h>
#include <corecrt_math.h>
#include <d3dx9.h>

// Forward declarations
struct SIGepard;
struct SIObject;
struct SIPlane;
struct SHillRing;
struct SSplineRing;
struct SNode;
struct SShaderInfo;
struct SShader2Info;
struct SShaderClipboard;
struct SStream;
struct SAblak;
struct SVector;

// Enums
enum SDrawType : int {
    DT_NORMAL = 0,
    DT_ADD = 1,
    DT_SHADOW = 2,
};

enum SFrameType : int {
    FT_EMPTY = 0,
    FT_SPRITE = 1,
    FT_TEXT = 2,
    FT_FIXTEXT = 3,
    FT_BOX = 4,
    FT_ANIM = 5,
    FT_MINIMAP = 6,
    FT_SCALER = 7,
    FT_SPRITE_9SLICE = 8,
    FT_FORCE_INT = 0x7FFFFFFF,
};

enum HDMode : int {
    Default = 0,
    X2 = 1,
    FullHD = 2,
    FullHD_Shift = 3,
};

enum TypeFace : int {
    TF_None = 0,
    Pig = 1,
    Rabbit = 2,
    Small = 3,
    Arial = 4,
    ChineseSmall = 5,
    ChineseRabbit = 6,
    ChinesePig = 7,
};
typedef TypeFace FontEffect;

struct SEXPLOSIONPARAMS {
    int index;
    int hangindex;
    int fust;
};

struct SCustomGlyph {
    int X;
    int Y;
    int Width;
    int Height;
};

// SIGepard — renderer interface base class
// Virtual methods converted from SIGepard_vtbl in PDB
// Data fields from PDB struct layout
struct SIGepard {
    // === Virtual methods (vtable order from PDB) ===
    virtual void SetBoardVisibility(bool visible) = 0;
    virtual void SetResolution(int w, int h) = 0;
    virtual void GetResolution(int* w, int* h) = 0;
    virtual int getViewWidth() = 0;
    virtual int getViewHeight() = 0;
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual void CreateHill(SHillRing** ring) = 0;
    virtual void SetMode(bool a, bool b, int c, int d, int e) = 0;
    virtual void SetResolutionScale(float scale) = 0;
    virtual void Resize(unsigned int w, unsigned int h) = 0;
    virtual void InitEditor() = 0;
    virtual void RenderScene(bool flag) = 0;
    virtual void RenderBoardOnly() = 0;
    virtual HBITMAP RenderToWinBitmap(int a, int b, int c) = 0;
    virtual unsigned char* RenderMinimap(int w, int h) = 0;
    virtual void UpdateMinimap(int a, int b, int c, unsigned char* data) = 0;
    virtual void MakeScreenshot(int w, int h, char* filename, unsigned int flags) = 0;
    virtual void BackbufferScreenshot(const char* filename) = 0;
    virtual void CreateScene() = 0;
    virtual void DestroyScene() = 0;
    virtual void SetPerspectiveProjection(float fov, float nearPlane, float farPlane) = 0;
    virtual void SetOrthogonalProjection(float a, float b, float c) = 0;
    virtual float GetFOV() = 0;
    virtual void SetViewProperties(float a, float b, float c, float d, float e) = 0;
    virtual void CreateSpline(SHillRing* hill, SSplineRing** spline, float size, int type) = 0;
    virtual void GetSelectedNode(float x, float y, float z, SHillRing** hill, SSplineRing** spline, SNode** node, bool flag) = 0;
    virtual void GetSelectedNode2(float x, float y, SHillRing** hill, SSplineRing** spline, SNode** node, bool flag) = 0;
    virtual void SelectNode(SSplineRing* spline, SNode* node) = 0;
    virtual void SelectAllNode(SSplineRing* spline) = 0;
    virtual void DeselectAllNode(SSplineRing* spline) = 0;
    virtual void DeleteSelectedNodes(SHillRing* hill, SSplineRing* spline) = 0;
    virtual void CloseSpline(SHillRing* hill) = 0;
    virtual void GetNodePosition(SSplineRing* spline, int index, float* x, float* y, float* z) = 0;
    virtual void MoveSplineNode(SHillRing* hill, SSplineRing* spline, SNode* node, float x, float y, float z) = 0;
    virtual void SetTopSplineAltitude(SHillRing* hill, float alt) = 0;
    virtual void SetTopSplineAltitude2(SHillRing* hill, float alt) = 0;
    virtual float GetTopSplineAltitude(SHillRing* hill) = 0;
    virtual float GetTopSplineAltitude2(SHillRing* hill) = 0;
    virtual void DestroySpline(SHillRing* hill, SSplineRing* spline) = 0;
    virtual void DestroyHill(SHillRing* hill) = 0;
    virtual void RecalculateCentralNodes(SHillRing* hill) = 0;
    virtual void SetSplineAltitude2(float alt, SSplineRing* spline) = 0;
    virtual float GetSplineAltitude2(SSplineRing* spline) = 0;
    virtual void DrawLine(float x1, float y1, float z1, float x2, float y2, float z2) = 0;
    virtual void SelectSplineRing(SSplineRing* spline) = 0;
    virtual void SelectHillRing(SHillRing* hill) = 0;
    virtual int GetCheckBoxNr(SSplineRing* spline) = 0;
    virtual void SetSplineVisibility(SSplineRing* spline, bool visible) = 0;
    virtual bool GetSplineVisibility(SSplineRing* spline) = 0;
    virtual void JezusAtmentAVizenMiMiertNe() = 0;
    virtual void SetUpObjectEffects() = 0;
    virtual void SaveSplines(SStream* stream, bool flag) = 0;
    virtual void SaveSplinesSel(SStream* stream) = 0;
    virtual void LoadSplines(SStream* stream) = 0;
    virtual void LoadSplinesSel(SStream* stream) = 0;
    virtual void DestroySplines() = 0;
    virtual void DoAltitude1(SHillRing* hill) = 0;
    virtual void DoAltitude2(SHillRing* hill) = 0;
    virtual void SetRiverSize(SHillRing* hill, float size) = 0;
    virtual float GetRiverSize(SHillRing* hill) = 0;
    virtual void GetSelections(SHillRing** hill, SSplineRing** spline) = 0;
    virtual void RecalculateNodesAltitude() = 0;
    virtual void RecalculateControlPoints(SHillRing* hill) = 0;
    virtual void SetSplineSize(float size, SSplineRing* spline) = 0;
    virtual float GetSplineSize(SSplineRing* spline) = 0;
    virtual void BringToFront() = 0;
    virtual void SendToBack() = 0;
    virtual void RefreshShader(SShaderInfo* shader) = 0;
    virtual void RedrawHill(SHillRing* hill) = 0;
    virtual void ReleaseMesh(SHillRing* hill) = 0;
    virtual void SetSplineDisplay(bool display) = 0;
    virtual void UpdateShaders() = 0;
    virtual void SaveShaders(SStream* stream) = 0;
    virtual void LoadShaders(SStream* stream) = 0;
    virtual void CopyShaders(SDArray<SShaderClipboard>* clipboard, int x0, int z0, int x1, int z1) = 0;
    virtual void PasteShaders(SDArray<SShaderClipboard>* clipboard, int x, int z) = 0;
    virtual SShaderInfo* CreateShader(float x, float z, int texture, float size, int rotation, bool a, bool b) = 0;
    virtual SShaderInfo* CreateShader2(float x, float z, const char* filename, int type, bool a, bool b) = 0;
    virtual SShader2Info* CreateShader2Info(float x, float z, int texture, int type, SDrawType dt, bool a, bool b, bool c) = 0;
    virtual void MoveShader2(SShader2Info* shader, float x, float z, bool flag) = 0;
    virtual void DestroyShader2(SShader2Info* shader) = 0;
    virtual SShaderInfo* GetIndexFromCoordinates(float x, float z) = 0;
    virtual void SetSelectedShader(SShaderInfo* shader) = 0;
    virtual void DeleteSelectedShader() = 0;
    virtual bool IsShaderSelected() = 0;
    virtual void RotateShaderToLeft() = 0;
    virtual void RotateShaderToRight() = 0;
    virtual SShaderInfo* GetSelectedShader() = 0;
    virtual void RefreshHill(SHillRing* hill) = 0;
    virtual void AddNewSplineNode(SHillRing* hill, float x, float y, float z) = 0;
    virtual void AddToShaderFolderList(const char* folder) = 0;
    virtual void ClearShaderFolderList() = 0;
    virtual void EnableFPS(bool enable, int x, int y) = 0;
    virtual bool GetFPSEnabled() = 0;
    virtual int GetDebugTextFrame() = 0;
    virtual void AdvanceTime(unsigned int worldTime, unsigned int animTime) = 0;
    virtual void SetInterpolation(double interp) = 0;
    virtual unsigned int GetAnimElapsedTime() = 0;
    virtual void TransformScreenToCamera(float* x, float* y) = 0;
    virtual int CreateDirectionalLight(float dx, float dy, float dz, float r, float g, float b) = 0;
    virtual int CreatePointLight(float x, float y, float z, float r, float g, float b, float range, float atten) = 0;
    virtual void SetLightPosition(int light, float x, float y, float z) = 0;
    virtual void DestroyLight(int light) = 0;
    virtual void SetAmbientLight(float r, float g, float b) = 0;
    virtual void SetBottomAmbientLight(float r, float g, float b) = 0;
    virtual void SetSunLight(float r, float g, float b, float dir, float elev) = 0;
    virtual void SetEnvironmentLight() = 0;
    virtual void SetFog(float r, float g, float b, float start, float end) = 0;
    virtual void TransformScaledPointUi(float x, float y, float z, float scale, float* outX, float* outY, float* outScale) = 0;
    virtual int PrecacheObject(const char* filename, float scale, SDrawType dt) = 0;
    virtual int CleanupObjectCache() = 0;
    virtual SIObject* CreateObject(const char* filename, float scale, bool flag) = 0;
    virtual SIObject* CreateObjectByIndex(int index, bool flag) = 0;
    virtual void ReplaceObject(SIObject* obj, const char* filename, float scale) = 0;
    virtual void ReplaceObjectByIndex(SIObject* obj, int index) = 0;
    virtual int GetShadowQuality() = 0;
    virtual void SetShadowQuality(int quality) = 0;
    virtual int GetMaxShadowQuality() = 0;
    virtual float GetShadowDistance() = 0;
    virtual void SetShadowDistance(float dist) = 0;
    virtual void SkipObjectShadowMask(SIObject* obj, SStream* stream) = 0;
    virtual void GenerateAllShadowShaders() = 0;
    virtual SIPlane* CreatePlane(int xsize, int zsize) = 0;
    virtual void DestroyPlane() = 0;
    virtual int LoadTexture(const char* filename, bool mipmap, bool alpha) = 0;
    virtual void ReleaseTexture(int handle, bool flag) = 0;
    virtual void SetTextureFilter(int filter) = 0;
    virtual int GetTextureFilter() = 0;
    virtual void SetTextureDetail(int detail) = 0;
    virtual int GetTextureDetail() = 0;
    virtual int CreateDynamicShader(int texture, float x, float z, float size, unsigned int color) = 0;
    virtual void RemoveDynamicShader(int handle) = 0;
    virtual void SetDynamicShaderPosition(int handle, float x, float z) = 0;
    virtual int CreateRect(unsigned int color, int x0, int z0, int x1, int z1) = 0;
    virtual void RemoveRect(int handle) = 0;
    virtual void SetRectColor(int handle, unsigned int color) = 0;
    virtual void SetRectPosition(int handle, int x0, int z0, int x1, int z1) = 0;
    virtual void UpdateRects() = 0;
    virtual int CreateSmokeTrail(int texture, float x, float y, float z, int color, float strength, float fadeSpeed, float uScale, float vScale, SDrawType dt) = 0;
    virtual void TrackSmokeTrail(int handle, float x, float y, float z, float width) = 0;
    virtual void CloseSmokeTrail(int handle) = 0;
    virtual int CreateGroundTrail(int texture, float strength, float uScale, float vScale, float fadeTime, SDrawType dt) = 0;
    virtual void TrackGroundTrail(int handle, float x, float z, float dir) = 0;
    virtual void CloseGroundTrail(int handle) = 0;
    virtual void RemoveGroundTrail(int handle) = 0;
    virtual int CreateLake(int texture, float x, float y, float z, int flow, int sparkle) = 0;
    virtual void RemoveLake(int handle) = 0;
    virtual void ChangeLakeHeight(int handle, float y) = 0;
    virtual void DrawLakesToWaterMap() = 0;
    virtual void SetTileset(int tileset) = 0;
    virtual void ClearGlow() = 0;
    virtual void AddGlow(float x, float y, float z, int r, int g, int b, int scale, float fadeing) = 0;
    virtual void TurnOnGlow() = 0;
    virtual void EgysegAlattiTalajKoszolas(float x, float z, int type) = 0;
    virtual void CreateEffectsHandler() = 0;
    virtual void DestroyEffectsHandler() = 0;
    virtual int LoadEffect(char* classname, const char* section) = 0;
    virtual void PlayEffect(int effect, SIObject* obj, SDArray<SAblak>* ablak) = 0;
    virtual void PlayEffectStr(int effect, SIObject* obj, char* str, int val) = 0;
    virtual void* PlayEffectPos(int effect, float x, float y, float z, float scale) = 0;
    virtual void PlayEffectFull(int effect, float x, float y, float z, float a, float b, float c, float d, float e, float f, float g) = 0;
    virtual void TrackEffect(void* handle, float x, float y, float z, float scale) = 0;
    virtual void StopEffect(void* handle) = 0;
    virtual void RefreshIndices(void* handle) = 0;
    virtual void SetFullBright(bool bright) = 0;
    virtual float GetCameraHRot() = 0;
    virtual SVector* GetCameraForward(SVector* result) = 0;
    virtual int GetTileset() = 0;

    // === Data fields (from PDB) ===
    int _izzo_robbanas_darabok;
    int _izzo_robbanas_darabok2;
    int _izzo_robbanas_darabok3;
    int _shader_light;
    int _shader_light_long;
    int _becsapodas_anim_small;
    int _becsapodas_anim_medium;
    int _robbanas_anim;
    int _robbanas_anim2;
    int _robbanas_anim_fuel;
    int _lovegtorony_robbanas_anim;
    int _lovegtorony_robbanas_anim2;
    int _lovegtorony_darabok;
    int _nagy_robbanas_darabok;
    int _tuzijatek_robbanas_anim;
    int _tuzijatek_robbanas_anim2;
    int _becsapodo_darab_nyom1;
    int _becsapodo_darab_nyom2;
    int _becsapodo_darab_nyom3;
    int _becsapodo_darab_nyom4;
    int _egyseg_alatti_talaj_koszolas1;
    int _egyseg_alatti_talaj_koszolas2;
    int _egyseg_alatti_talaj_koszolas3;
    int _egyseg_alatti_talaj_koszolas_rabbit;
    int _egyseg_alatti_talaj_koszolas_pig;
    int _tankloves_alatti_talaj_koszolas1;
    int _tankloves_alatti_talaj_koszolas2;
    int _tankloves_alatti_talaj_koszolas3;
    int _raketa_alatti_talaj_koszolas1;
    int _raketa_alatti_talaj_koszolas2;
    int _kis_robbanas_darabok;
    int _rain;
    int _snowfall;
    int _tank_foldbeloves_particles;
    int _tank_foldbeloves_kis_darabok;
    int _raketas_foldbeloves_particles;
    int _raketas_foldbeloves_kis_darabok;
    int _raketas_foldbeloves_nagy_darabok;
    int _becsapodas_fust;
    int _rocket_boom;
    int _rocket_boom2;
    int _folyo;
    int _folyo_csillogas;
    int _gepfegyveres_kozepso_torkolattuz;
    int _gepfegyveres_oldalso_torkolattuz;
    int _loveg_torkolattuz;
    int _tank_torkolattuz;
    int _egysegfust_gyenge;
    int _egysegfust_eros;
    int _raketa_robbanas_talaj_feny;
    int _torkolattuz_talaj_feny;
    int _torkolattuz_talaj_feny2;
    int _torkolat_fust;
    int _heli_celfust;
    int _normal_porzas;
    int _desert_porzas;
    int _rock_porzas;
    int _a_foldbe_csapodo_darabok_fustje;
    int _unit_reflektor;
    int _lighting;
    int _kis_kor;
    int _bomba_lokeshullam;
    int _uzemanyag_lokeshullam;
    int _haz_fust;
    int _haz_fust2;
    int _haz_fust3;
    int _glow;
    int _szintlepes_csillagok;
    int _szintlepes_csillagok2;
    int _levelup_lens;
    int _ablak_mozgas;
    int _eryngo;
    int _eryngo_porzas;
    int _objektum_fust;
    int _gray_texture;
    SEXPLOSIONPARAMS _robbanastipusok[100];
    float GlobalisSzelirany;
};

#endif // DENGINE3_IGEPARD_H
