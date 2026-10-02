// 3dengine/gepard.h
// SGepard — main renderer class
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_GEPARD_H
#define DENGINE3_GEPARD_H

#include "igepard.h"
#include "smartptr.h"
#include "chain.h"
#include "darray.h"
#include "string2.h"
#include "vector3.h"
#include "hdbeefup.h"

// Enum typedefs
typedef int GepardFlags;
typedef int SLightingType;

// Forward declarations
struct SBitmap;
struct SBoard;
struct STerrain;
struct SEffect;
struct SGroup;
struct SMesh;
struct SParcel;
struct SParcel2;
struct SShadowMask2;
struct SWorld;
struct SProperties;
struct SMeshProp;
struct SLakeBlock {
    int BlockIdx;
    SMesh *Mesh;
    SMesh *Mesh2;
    int RecalcHeightTransparency;
};
struct SVert {
    float x, y, z;
    float u, v;
};

struct SVertDiff {
    float x, y, z;
    unsigned int diff;
    float u, v;
};

struct SSmokeTrailPoint {
    D3DXVECTOR3 Vector;
    float Scale;
    float TextureV;
    float Alpha;
    unsigned int StartTime;
};
struct SGroundTrailSegment {
    D3DXVECTOR3 Vertices[4];
    unsigned int StartTime;
    bool Noticed;
};
struct SStream;
struct SParticles;
struct SNodeInfo2 {
    SNodeInfo2 *prev;
    SNodeInfo2 *next;
    float x;
    float y;
    float z;
    int nodetype;
};
struct SAblak;
struct SShaderClipboard {
    char *filename;
    int length;
    int X;
    int Z;
    int Rotation;
};
struct S3Vertex {
    D3DXVECTOR3 v1;
    D3DXVECTOR3 v2;
    D3DXVECTOR3 v3;
};

struct SBaseVertex {
    float x, y, z;
    float nx, ny, nz;
    float tx, ty, tz;
};

struct SXZ {
    float x;
    float z;
};

struct SMaterial;

struct SRiverStripRing {
    SRiverStripRing *next;
    SRiverStripRing *prev;
    unsigned int numvertices;
    float textureV1pos;
    float textureV2pos;
    int numverify;
    SXZ verify[14];
    SVertDiff *strip;
};

// === Supporting structs used by value in SGepard ===

struct SShaderInfo {
    SShaderInfo* next;
    SShaderInfo* prev;
    char filename[120];
    float xpos;
    float zpos;
    SMesh* mesh;
    int ShaderTHandle;
    int XVertices;
    int ZVertices;
    int Rotation;
};

struct SVertexLight {
    float r;
    float g;
    float b;
};

struct SShader2Info {
    SShader2Info* next;
    SShader2Info* prev;
    float xpos;
    float zpos;
    SMesh* mesh;
    int ShaderTHandle;
    int XVertices;
    int ZVertices;
    SDrawType DrawType;
    float Erosseg;
    float R;
    float G;
    float B;
    bool ForceBright;
    SVertexLight* vertexlight;
};

struct SShaderClipboard;

struct SFolders {
    SFolders* next;
    SFolders* prev;
    char filename[120];
};

struct SHoleInfo {
    SHoleInfo* next;
    SHoleInfo* prev;
    int x;
    int z;
    SChain<struct SNodeInfo2>* nodechain;
};

struct SNodeInfo {
    SNodeInfo* prev;
    SNodeInfo* next;
    SHillRing* hillring;
    SSplineRing* splinering;
    SNode* node;
    float magassag;
    int nodeindex;
};

struct SHillRing {
    SHillRing* prev;
    SHillRing* next;
    SChain<SSplineRing>* SplineChain;
    int Type;
    bool WaterEnabled;
    float RiverSize;
    SMesh* HillMesh;
    SMesh* HillMesh2;
};

struct SSplineRing {
    SSplineRing* prev;
    SSplineRing* next;
    struct SSpline* spline;
};

struct SNode {
    SNode* next;
    SNode* prev;
    bool ControlPoint;
    bool Selected;
    bool Highlighted;
    int HeightFrom;
    float x;
    float y;
    float z;
    float waterleftsize;
    float waterrightsize;
};

struct SObjectCacheProp {
    char* FileName;
    float Scale;
    unsigned short RefCount;
    SGroup* Group;
};

struct SObjectProp {
    int CacheIdx;
    SGroup* Group;
    unsigned int LastRendered;
};

struct SObjectHashChain {
    int ObjectIdx;
    int Next;
};

struct STextureProp {
    char* FileName;
    IDirect3DTexture9* lpTexture;
    int RefCount;
    unsigned int Width;
    unsigned int Height;
    bool MipMaps;
    int Alpha;
};

struct SDynamicShaderProp {
    int TextureIndex;
    float X;
    float Z;
    float Size;
    unsigned int Color;
};

struct SRectProp {
    SMesh* Mesh;
    unsigned int Color;
    int X0;
    int Z0;
    int X1;
    int Z1;
};

struct SSmokeTrail {
    SDArray<SSmokeTrailPoint>* Points;
    int TextureIndex;
    bool AutoDestruct;
    int Color;
    float Strength;
    float FadeSpeed;
    float UScale;
    float VScale;
    SDrawType DrawType;
};

struct SGroundTrail {
    SHeap<SGroundTrailSegment>* Segments;
    int TextureIndex;
    bool AutoDestruct;
    float Strength;
    float FadeTime;
    float UScale;
    float VScale;
    float LastX;
    float LastZ;
    float LastDir;
    bool HaveFirst;
    SDrawType DrawType;
};

struct SGLake {
    int TextureIndex;
    int TextureIndex2;
    float Y;
    unsigned int Color;
    SDArray<SLakeBlock> Blocks;
    int Flow;
    int Sparkle;
};

struct SDynamicVB {
    IDirect3DVertexBuffer9* lpVertexBuffer;
    unsigned int NumVertices;
    unsigned int VertexFormat;
    int VertexSize;
    bool Locked;
    int LockPosition;
};

struct SLightProp {
    int type;
    D3DLIGHT9 d3dlight;
};

struct SShadowTransformCB {
    float ViewProjMatrix[4][4];
    int Vector4fCount;
};

struct SLightCB {
    float Ambient[4];
    float BottomAmbient[4];
    float SunlightDir[4];
    float SunlightColor[4];
    float FogParams[4];
    int Vector4fCount;
};

// SGepard::DebugLine
struct SGepardDebugLine {
    SVector start;
    SVector end;
    unsigned int color;
    int owner;
};

// === SGepard — main renderer ===
struct SGepard : SIGepard {
    bool BoardVisible;
    int HegyOldal;
    int HeightFrom;
    SChain<SShaderInfo> ShaderInfos;
    SChain<SShader2Info> Shader2Infos;
    SShaderInfo* SelectedShader;
    SChain<SFolders> ShaderFolders;
    SProperties* DecalsIni;
    SChain<SHoleInfo> GlobalHoleList;
    SChain<SNodeInfo> NodeInfos;
    SHillRing* gpSelHillRing;
    SSplineRing* gpSelSplineRing;
    SChain<SHillRing> HillChain;
    bool SplineDisplay;
    unsigned int Editor_LastTime;
    int XSize;
    int ZSize;
    unsigned int RefCount;
    SmartPtr<IDirect3DTexture9> ShadowMapTexture;
    SmartPtr<IDirect3DTexture9> ShadowMapDepthTexture;
    int ShadowMapWidth;
    float ShadowDistance;
    unsigned int Adapter;
    D3DADAPTER_IDENTIFIER9 AdapterID;
    D3DDEVTYPE DeviceType;
    D3DFORMAT AdapterFormat;
    IDirect3D9* lpD3D;
    D3DDISPLAYMODE d3ddm;
    D3DXMATRIX CameraMatrix;
    D3DXMATRIX ProjectionMatrix;
    D3DXMATRIX ShadowMatrix;
    D3DXMATRIX InverseShadowCameraMatrix;
    IDirect3DDevice9* lpD3DDev;
    unsigned int DefAmbient;
    float CameraHRot;
    float CameraVRot;
    float CameraXPos;
    float CameraYPos;
    float CameraZPos;
    void* Glow;
    const char* ScreenshotFile;
    HWND hWnd;
    bool FullScreen;
    bool VSync;
    D3DMULTISAMPLE_TYPE MSAALevel;
    D3DPRESENT_PARAMETERS PresentationParameters;
    int ModeWidth;
    int ModeHeight;
    float OrthoScale;
    float FOV;
    float NearPlane;
    float FarPlane;
    int ViewWidth;
    int ViewHeight;
    int ViewWidthScaled;
    int ViewHeightScaled;
    SBoard* Board;
    int PolyCount;
    D3DCOLORVALUE AmbientColorVal;
    D3DCOLORVALUE BottomAmbientColorVal;
    D3DCOLORVALUE SunColorVal;
    D3DCOLORVALUE EnvironmentColorVal;
    D3DXVECTOR3 SunDir;
    D3DCOLORVALUE FogColorVal;
    D3DXMATRIX SunProjection;
    float LastSunDirection;
    float LastSunElevation;
    unsigned int FogColor;
    float FogStart;
    float FogEnd;
    float FogStart2;
    float FogEnd2;
    float FogMultiplier;
    float FogMultiplier256;
    SGroup* node1;
    SGroup* node2;
    int FPSTextFrame;
    int DebugTextFrame;
    int FPSFont;
    int FrameCount;
    unsigned int RenderCounter;
    unsigned int WorldTime;
    unsigned int LastWorldTime;
    unsigned int ElapsedTime;
    unsigned int AnimTime;
    unsigned int AnimElapsedTime;
    double Interpolation;
    SLightProp LightProps[4];
    SShadowTransformCB shadowCB;
    SLightCB lightCB;
    SHeap<SObjectCacheProp> ObjectCache;
    SHeap<SObjectProp> StaticObjects;
    SHeap<SObjectProp> DynamicObjects;
    bool ObjectsHashed;
    int* ObjectHash;
    SDArray<SObjectHashChain> ObjectHashChain;
    int ShadowQuality;
    int MaxShadowQuality;
    IDirect3DSurface9* ShadowRenderTarget;
    IDirect3DSurface9* ShadowRenderTargetZ;
    int SunLight;
    STerrain* Terrain;
    void* ShaderChangeNotification;
    bool Flags[2];
    int DebugMode;
    D3DXMATRIX IdentityMatrix;
    Array<SGepardDebugLine> debugLines;
    SmartPtr<IDirect3DVertexShader9> standardVertexShaders[7];
    SmartPtr<IDirect3DPixelShader9> standardPixelShaders[7];
    SmartPtr<IDirect3DVertexShader9> depthWriteVertexShaders[8];
    SmartPtr<IDirect3DPixelShader9> depthWritePixelShaders[8];
    SmartPtr<IDirect3DVertexShader9> terrainVertexShader[2];
    SmartPtr<IDirect3DPixelShader9> terrainPixelShader[2];
    // IDirect3DPixelShader9 *wireframePS; // CONFORMANCE: removed — not in original binary
    SmartPtr<IDirect3DVertexShader9> terrainLightingVertexShader;
    SmartPtr<IDirect3DVertexShader9> ambientLitVertexShader;
    SmartPtr<IDirect3DVertexShader9> decalVertexShader;
    SmartPtr<IDirect3DVertexShader9> unlitDecalVertexShader;
    SmartPtr<IDirect3DPixelShader9> sepiaPixelShader;
    SmartPtr<IDirect3DPixelShader9> terrainLightingPixelShader;
    SmartPtr<IDirect3DPixelShader9> ambientLitPixelShader;
    SmartPtr<IDirect3DPixelShader9> decalPixelShader;
    SmartPtr<IDirect3DPixelShader9> unlitDecalPixelShader;
    SHeap<STextureProp> Textures;
    int TextureFilter;
    int TextureDetail;
    D3DFORMAT TFOpaque;
    D3DFORMAT TF1Bit;
    D3DFORMAT TFAlpha;
    D3DFORMAT TFHiOpaque;
    D3DFORMAT TFHiAlpha;
    D3DFORMAT TFShadow;
    D3DFORMAT TFShadowNull;
    D3DFORMAT TFShadowDepth;
    bool TFSupportNonPow2;
    SDrawType DrawType;
    SHeap<SDynamicShaderProp> DynamicShaders;
    SHeap<SRectProp> Rects;
    SHeap<SSmokeTrail> SmokeTrails;
    SHeap<SGroundTrail> GroundTrails;
    SHeap<SGLake> Lakes;
    SHeap<SDynamicVB> DynamicVBs;
    SEffect* Effect;
    bool FullBright;
    int Tileset;
    bool FirstDrawWater;
    int Temp;
    float ResolutionScale;
    SmartPtr<IDirect3DSurface9> ResolutionScaleSurface;
    SmartPtr<IDirect3DSurface9> ResolutionScaleDepthSurface;

    // Optional pre/post-Reset hook for non-engine consumers that hold their own
    // D3DPOOL_DEFAULT resources (currently: SMarket's HQ 3D preview RT). One
    // consumer at a time; SMarket sets these in InitPreview3D, clears in
    // DestroyPreview3D.
    void (*PreResetCallback)(void *userData);
    void (*PostResetCallback)(void *userData);
    void *ResetCallbackUserData;
    void RegisterResetCallbacks(void (*pre)(void *), void (*post)(void *), void *userData);

    // Extended debug overlay (DebugInfoFrame: textures Mb / tiles / camera
    // direction + compass / mouse XYZ). Appended after the original SGepard
    // layout so binary-offset asserts above stay valid. Gated by ShowDebugInfo
    // runtime flag — defaulted true in _DEBUG ctors so the "show fps" cheat
    // creates DebugInfoFrame in debug game builds; false in release game.
    int DebugInfoFrame;
    float DebugMouseX, DebugMouseY, DebugMouseZ;
    bool ShowDebugInfo;
    SShaderInfo *HoverShader;  // editor decal hover (white tint, distinct from yellow selection)

    // HDB_PREVIEW_GHOST: editor placement-preview hand-off. The editor sets
    // EditorPreviewGhost to an SIObject loaded via CreateObject and updates
    // EditorPreviewGhost{X,Y,Z} every frame; RenderScene draws it once with
    // alpha-blend + flat-fog tint after RenderObjects. Game leaves null.
    SIObject *EditorPreviewGhost;
    float EditorPreviewGhostX, EditorPreviewGhostY, EditorPreviewGhostZ;

    // Constructor / destructor
    SGepard();
    ~SGepard();

    // === SIGepard virtual overrides ===
    // These override pure virtuals from SIGepard. Some methods below have
    // additional overloads with extra IDA __usercall artifact params — those
    // are non-virtual internal helpers that coexist with these overrides.
    void SetMode(bool isfullscreen, bool vsync, int msaa, int width, int height) override;
    void SetResolutionScale(float scale) override;
    void Resize(unsigned int width, unsigned int height) override;
    void RenderScene(bool minimapmode) override;
    void RenderBoardOnly() override;
    void SetSunLight(float r, float g, float b, float direction, float elevation) override;
    void TrackGroundTrail(int handle, float x, float z, float dir) override;
    void TrackSmokeTrail(int handle, float x, float y, float z, float width) override;
    SIObject* CreateObjectByIndex(int index, bool dynamic) override;
    void ReplaceObjectByIndex(SIObject* obj, int index) override;
    void DrawLine(float x1, float y1, float z1, float x2, float y2, float z2) override;
    void RedrawHill(SHillRing* hill) override;
    SShader2Info* CreateShader2Info(float x, float z, int texture, int type, SDrawType dt, bool a, bool b, bool c) override;
    SShaderInfo* CreateShader2(float x, float z, const char* filename, int type, bool a, bool b) override;
    void PlayEffectStr(int effect, SIObject* obj, char* str, int val) override;
    void* PlayEffectPos(int effect, float x, float y, float z, float scale) override;
    void PlayEffectFull(int effect, float x, float y, float z, float a, float b, float c, float d, float e, float f, float g) override;
    // Type-mismatch overrides (int vs unsigned int, bool vs BOOL, etc.)
    void MakeScreenshot(int w, int h, char* filename, unsigned int flags) override;
    void SetLightPosition(int light, float x, float y, float z) override;
    void DestroyLight(int light) override;
    int CreatePointLight(float x, float y, float z, float r, float g, float b, float range, float atten) override;
    void SaveSplines(SStream* stream, bool flag) override;
    void CreateSpline(SHillRing* hill, SSplineRing** spline, float size, int type) override;
    void SetInterpolation(double interp) override;
    SIObject* CreateObject(const char* filename, float scale, bool flag) override;

    // All methods (extracted from gepard.cpp)
    HBITMAP RenderToWinBitmap(int rotate, int width, int height);
    S3Vertex *GetNext3Vertex(S3Vertex *result, SChain<SNodeInfo2> *_nodechain, bool plus3);
    SChain<SNodeInfo2> *CollectTileNodes(int _x, int _z, SChain<SSplineRing> *_splinechain);
    int PrecacheObject(const char *filename, float scale, SDrawType drawtype);
    SGroup *CreateObject(int cacheidx, bool dynamic);
    SIObject *CreateObject(const char *filename, float scale, BOOL dynamic);
    SShader2Info *CreateShader2(float _x, float _z, int _thandle, int _scale, SDrawType drawtype, bool _snaptocenter, bool _appendtolist, bool _forcebright);
    SShaderInfo *CreateShader(float _x, float _z, int thandle, float texscale, int _rotation, bool _appendtolist, bool _snaptocenter);
    SShaderInfo *CreateShader(float _x, float _z, const char *_filename, int _rotation, bool _appendtolist, bool _snaptocenter);
    SShaderInfo *GetIndexFromCoordinates(float _x, float _z);
    SShaderInfo *GetSelectedShader();
    SIPlane *CreatePlane(int xsize, int zsize);
    SVector *GetCameraForward(SVector *result);
    bool CheckClickBox(D3DXVECTOR3 *vec, float click_x1, float click_y1, float click_x2, float click_y2);
    bool CheckDepthStencilFormat(D3DFORMAT format);
    bool CheckTextureFormat(D3DFORMAT format, bool rendertarget);
    bool GetFPSEnabled();
    bool GetSplineVisibility(SSplineRing *_splinering);
    bool IsShaderSelected();
    bool IsTextureOpaque(int idx);
    char ResetDevice(int a2);
    float GetCameraHRot();
    float GetClickDistanceSquare(D3DXVECTOR3 *vec, float click_x, float click_y);
    float GetFOV();
    float GetRiverSize(SHillRing *hillring);
    float GetShadowDistance();
    float GetSplineAltitude2(SSplineRing *_splinering);
    float GetSplineSize(SSplineRing *_splinering);
    float GetTopSplineAltitude(SHillRing *_hillring);
    float GetTopSplineAltitude2(SHillRing *_hillring);
    int AddRefTexture(int idx);
    int CleanupObjectCache();
    int CreateDirectionalLight(float colorr, float colorg, float colorb, float vectorx, float vectory, float vectorz);
    int CreateDynamicShader(int texture, float x, float z, float size, unsigned int color);
    int CreateDynamicVB(unsigned int fvf, unsigned int numvertices);
    int CreateEmptyTexture(unsigned int width, unsigned int height, int depth, bool mipmaps, bool forcePow2);
    int CreateGroundTrail(int texture_idx, float strength, float fade_time, float u_scale, float v_scale, SDrawType drawtype);
    int CreateLake(int texture_idx, float x, float y, float z, int flow, int sparkle);
    int CreatePointLight(float colorr, float colorg, float colorb, float posx, float posy, float posz, float range, unsigned int attenuation);
    int CreateRect(unsigned int color, int x0, int z0, int x1, int z1);
    int CreateShadowTexture(unsigned int width, unsigned int height);
    int CreateSmokeTrail(int texture_idx, float x, float y, float z, int color, float strength, float fade_speed, float u_scale, float v_scale, SDrawType drawtype);
    int CreateTextureFromBitmap(const char *filename, SBitmap *bmap, bool mipmaps);
    int CreateTextureFromDXT(const char *dxt_filename, const char *filename, bool mipmaps);
    int CreateTextureFromLevelBitmaps(SDArray<SBitmap *> *bitmaps, const char *name);
    int GetCheckBoxNr(SSplineRing *_splinering);
    int GetDebugTextFrame();
    int GetHillSideFaceCount(SHillRing *_hillring);
    int GetHillSideVertexCount(SHillRing *_hillring);
    int GetMaxShadowQuality();
    int GetShadowQuality();
    int GetTextureAlpha(int idx);
    int GetTextureDetail();
    int GetTextureFilter();
    int GetTileset();
    int Initialize(HWND _hwnd, bool isfullscreen, bool vsync, D3DMULTISAMPLE_TYPE msaa, int width, int height, bool enable_tl);
    int LoadEffect(char *effectname, const char *texturename);
    int LoadTexture(const char *filename, bool mipmaps, bool convert);

#ifdef HDB_MISSING_ASSET_FALLBACK
    // Like LoadTexture, but on a missing-file -1 logs the original filename
    // and retries with editor/missing.dxt so single-load callers (units,
    // projectile FX, particles, glow) get a visible magenta placeholder
    // instead of -1. Returns -1 only if the placeholder itself is missing —
    // then the caller's panic still fires (broken install). Do NOT use
    // this in enumeration loops that rely on -1 as a "no more frames"
    // sentinel (e.g. Init_Boom, Init_TT) — call LoadTexture directly there.
    int LoadTextureOrPlaceholder(const char *filename, bool mipmaps, bool convert);
#endif
    int MemChk(void *memptr);
    int getViewHeight();
    int getViewWidth();
    unsigned char *LockDynamicVB(int a2, int a3, int a4, int idx, unsigned int needed_vertices);
    unsigned char *RenderMinimap(int width, int height);
    unsigned int GetAnimElapsedTime();
    unsigned int GetLightHandle();
    void *PlayEffect(int handle, float _x, float _y, float _z, float _scalespeed);
    void AddDebugLine(SVector start, SVector end, unsigned int color, unsigned int owner);
    void AddGlow(float x, float y, float z, int r, int g, int b, int scale, float fadeing);
    void AddNewSplineNode(SHillRing *_hillring, float x, float y, float z);
    void AddNodesFromSplineNodes(SChain<SNode> *snodechain, SChain<SNodeInfo2> *nodechain, float x1, float x2, float z1, float z2);
    void AddRef();
    void AddToHoleList(int x, int z);
    void AddToShaderFolderList(const char *foldername);
    void AdvanceTime(unsigned int elapsed, unsigned int anim_elapsed);
    void BackbufferScreenshot(const char *filename);
    void BringToFront();
    void ChangeLakeHeight(int idx, float y);
    void ChangeTextureAlphaType(int idx, int alpha);
    void CleanupSplines();
    void CleanupTextureCache();
    void ClearDebugLines(unsigned int color, int owner);
    void ClearDynamicVBs();
    void ClearGlow();
    void ClearHoleList();
    void ClearObjectShadows(bool reset, bool keepmask);
    void ClearResolutionScale();
    void ClearShaderFolderList();
    void ClearShaders();
    void ClearShadowMap();
    void CloseGroundTrail(int idx);
    void CloseSmokeTrail(int idx);
    void CloseSpline(SHillRing *_hillring);
    void CollectNode(SHillRing *_hillring, SSplineRing *_splinering, SNode *_node, int _nodeindex, float _magassag);
    void CopyShaders(SDArray<SShaderClipboard> *shaders, int x0, int z0, int x1, int z1);
    void CreateEffectsHandler();
    void CreateHill(SHillRing **_hillring);
    void CreateScene();
    void CreateSpline(SHillRing *_hillring, SSplineRing **_splinering, int _altitude2, int _checkboxnr);
    void CreateTriangles(SHillRing *_hillring);
    void DeleteAllNodeInfos();
    void DeleteSelectedNodes(SHillRing *_hillring, SSplineRing *_splinering);
    void DeleteSelectedShader();
    void DeselectAllNode(SSplineRing *_splinering);
    void DestroyEffectsHandler();
    void DestroyHill(SHillRing *_hillring);
    void DestroyLight(unsigned int handle);
    void DestroyPlane();
    void DestroyScene();
    void DestroyShader2(SShader2Info *s2i);
    void DestroyShaders(BOOL dont_remove_textures);
    void DestroySpline(SHillRing *_hillring, SSplineRing *_splinering);
    void DestroySplines();
    void DoAltitude1(SHillRing *selhillring);
    void DoAltitude2(SHillRing *selhillring);
    void DrawDynamicVB(int idx, unsigned int triangles);
    void DrawLakesToWaterMap();
    void DrawNode(bool cp, float x, float y, float z, D3DCOLORVALUE color);
    void DrawWater(SHillRing *_hillring, int drawmode, float _scroll, SStream *is);
    // Compute-from-spline-nodes branch ported from original editor.exe
    // DrawWater. Walks SSpline::Nodes pairwise, generates strip vertices on
    // the fly, and writes them to `is` (vertex-count int + 24*N bytes per
    // strip, trailing 0-int terminator per spline). Joining stub: leaves
    // FirstLeft/RightJoined* at -1.0 (visual seams at multi-spline joins
    // accepted as MVP defect). Used by SaveSplines to serialize fresh
    // geometry; in Commit 2 also populates RiverStripChain when is==NULL.
    void DrawWater_ComputeAndStream(SHillRing *_hillring, float _scroll, SStream *is);
    void EgysegAlattiTalajKoszolas(float x, float z, int race);
    void EnableFPS(bool enable, int font, int parentFrame);
    void EnableFog();
    void GenerateAllShadowShaders();
    void GenerateObjectShadow(SGroup *object, bool dynamic);
    void GetNodePosition(SSplineRing *_splinering, int index, float *x, float *y, float *z);
    void GetResolution(int *w, int *h);
    void GetSelectedNode(float x, float y, float z, SHillRing **_hillring, SSplineRing **_level, SNode **_node, bool _lockhillselection);
    void GetSelectedNode2(float clickx, float clicky, SHillRing **_hillring, SSplineRing **_level, SNode **_node, bool _lockhillselection);
    void GetSelections(SHillRing **_hillring, SSplineRing **_splinering);
    void GetViewBoundaries(D3DXVECTOR2 *bound);
    void HashObjects();
    void InitEditor();
    void InitPixelShader(const char *path, SmartPtr<IDirect3DPixelShader9> *shader, D3DXMACRO *defines);
    void InitRenderStates();
    void InitResolutionScale(int a2, int a3);
    void InitShaders();
    void InitTextureFormats();
    void InitVertexShader(const char *path, SmartPtr<IDirect3DVertexShader9> *shader, D3DXMACRO *defines);
    void JezusAtmentAVizenMiMiertNe();
    void LerpFolyo(SVertDiff *folyo, int folyoidx, SVert *vert, int vertidx1, int vertidx2, float ratio);
    void LoadDXTToLevelBitmaps(const char *dxt_filename, SDArray<SBitmap *> *bitmaps);
    void LoadShaders(SStream *is);
    void LoadSplines(SStream *is);
    void LoadSplinesSel(SStream *is);
    void LogCardInfo();
    void MakeObjectShadowMask(SGroup *object);
    void MakeObjectShadowShader(SGroup *object);
    void MakeScreenshot(unsigned int width, unsigned int height, char *pathandname, unsigned int sernumber);
    void MoveShader2(SShader2Info *s2i, float _x, float _z, bool _snaptocenter);
    void MoveSplineNode(SHillRing *_hillring, SSplineRing *_splinering, SNode *_node, float x, float y, float z);
    void PasteShaders(SDArray<SShaderClipboard> *shaders, int x0, int z0);
    void PlayEffect(int handle, SIObject *object, SDArray<SAblak> *ablakok);
    void PlayEffect(int handle, SIObject *object, char *meshname, int race);
    void PlayEffect(int handle, float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float strength, float fade_speed, float u_scale, float v_scale);
    void ProjectPoint(float u, float v, float xobj, float yobj, float zobj, float *x, float *z);
    void RecalculateCentralNodes(SHillRing *_hillring);
    void RecalculateControlPoints(SHillRing *_hillring);
    void RecalculateNodesAltitude();
    void RefreshHill(SHillRing *_hillring);
    void RefreshIndices(void *handle);
    void RefreshLights();
    void RefreshRect(SMesh *mesh, int x0, int z0, int x1, int z1);
    void RefreshShader(SShaderInfo *si);
    void RefreshSubNodes(SHillRing *_hillring);
    void RegenerateObjectShadowDecals();
    void Release();
    void ReleaseMesh(SHillRing *_hillring);
    void ReleaseTexture(int idx, bool noremove);
    void RelitShader2(SShader2Info *s2i);
    void ReloadTexture(int idx);
    void RemoveDynamicObject(int idx);
    void RemoveDynamicShader(int idx);
    void RemoveDynamicVB(int idx);
    void RemoveGroundTrail(int idx);
    void RemoveHashedObject(int idx);
    void RemoveLake(int idx);
    void RemoveRect(int idx);
    void RemoveSmokeTrail(int idx);
    void RenderBoardOnly(int a2, int a3);
    void RenderDebugLines();
    void RenderDecals(bool lit);
    void RenderDynamicShaders();
    void RenderGroundTrails();
    void RenderLakes();
    void RenderObjectShadowDecals();
    void RenderObjectShadows(bool minimapmode);
    void RenderObjects(bool minimapmode, bool shadows);
    void RenderRects();
    void RenderScene(int a2, bool minimapmode);
    void RenderScene2(int a2, bool minimapmode);
    void RenderShadowBoxToStencil();
    void RenderShadowMap(bool minimap);
    void RenderSmokeTrails();
    void ReplaceObject(SIObject *original, const char *filename, float scale);
    void ReplaceObject(SIObject *original, int cacheidx);
    void ResetShadowMapTexture(unsigned int stage);
    void Resize(int a2, unsigned int width, unsigned int height);
    void RestoreDynamicVBs();
    void RotateShaderToLeft();
    void RotateShaderToRight();
    void RoundToTextureSize(unsigned int width, unsigned int height, int *tex_width, int *tex_height);
    void SaveShaders(SStream *is);
    void SaveSplines(SStream *is, unsigned int internal);
    void SaveSplinesSel(SStream *is);
    void SelectAllNode(SSplineRing *_splinering);
    void SelectHillRing(SHillRing *_hillring);
    void SelectNode(SSplineRing *_splinering, SNode *_node);
    void SelectSplineRing(SSplineRing *_splinering);
    void SelectionFog(unsigned int color);
    // Like SelectionFog but with a depth-independent (flat) shader-side
    // blend. Uses for the Selection==2 hover tint where the original
    // depth-graded fog made the white wash invisibly subtle (~12%) at
    // typical editor camera distances. intensity ∈ [0,1] = visible
    // fraction of `color` blended over the underlying vertex.
    void SelectionFogFlat(unsigned int color, float intensity);
    void SendToBack();
    void SetAmbientColor(unsigned int ambient);
    void SetAmbientGray(int ambient);
    void SetAmbientLight(float r, float g, float b);
    void SetBoardVisibility(bool b);
    void SetBottomAmbientLight(float r, float g, float b);
    void SetDebugMode(int mode);
    void SetDrawType(SDrawType drawtype);
    void SetDynamicShaderPosition(int idx, float x, float z);
    void SetEnvironmentLight();
    void SetFlag(GepardFlags flag, bool enable);
    void SetFog(float r, float g, float b, float start, float end);
    void SetFullBright(bool fullbright);
    void SetInterpolation(long double interpolation);
    void SetLightCB();
    void SetLightPosition(unsigned int handle, float posx, float posy, float posz);
    void SetLightingType(SLightingType lighttype);
    void SetMaterial(SMaterial *mat);
    void SetMode(int a2, bool isfullscreen, bool vsync, D3DMULTISAMPLE_TYPE msaa, int width, int height);
    void SetOrthogonalProjection(float scale, float near_plane, float far_plane);
    void SetPerspectiveProjection(float fov, float near_plane, float far_plane);
    void SetRectColor(int idx, unsigned int color);
    void SetRectPosition(int idx, int x0, int z0, int x1, int z1);
    void SetResolution(int w, int h);
    void SetResolutionScale(int a2, float scale);
    void SetRiverSize(SHillRing *hillring, float size);
    void SetSSAA(bool enabled);
    void SetSelectedShader(SShaderInfo *_si);
    void SetShadowCB();
    void SetShadowDistance(float dist);
    void SetShadowMapTexture(unsigned int stage);
    void SetShadowQuality(int q);
    void SetSplineAltitude2(float _altitude, SSplineRing *_splinering);
    void SetSplineDisplay(bool kibe);
    void SetSplineSize(float _size, SSplineRing *_splinering);
    void SetSplineVisibility(SSplineRing *_splinering, bool _visibilitystate);
    void SetSunLight(int a2, int a3, float r, float g, float b, float direction, float elevation);
    void SetTessFolyoAlpha(SVertDiff *folyo, int idx);
    void SetTexture(unsigned int stage, int idx, bool setblend);
    void SetTextureDetail(int detail);
    void SetTextureFilter(int filter);
    void SetTileset(int tileset);
    void SetTopSplineAltitude(SHillRing *_hillring, float _altitude);
    void SetTopSplineAltitude2(SHillRing *_hillring, float _altitude);
    void SetUpObjectEffects();
    void SetUpPresentation();
    void SetViewProperties(float xpos, float ypos, float zpos, float hrot, float vrot);
    void SetWorldViewProjVertexShaderConstantBuffer(D3DXMATRIX *worldMatrix);
    void SkipObjectShadowMask(SIObject *object, SStream *is);
    void SortSplines(SHillRing *_hillring);
    void StopEffect(void *handle);
    void TrackEffect(void *handle, float _x, float _y, float _z, float _borningspeed);
    void TransformGroundPoint(int a2, float x, float z, float *s_x, float *s_y, float *s_zbuf, float *s_rhw, unsigned int *s_fog, float h_bias, float z_bias);
    void TransformPoint(float x, float y, float z, float *s_x, float *s_y);
    void TransformPoint(float x, float y, float z, float *s_x, float *s_y, float *s_z);
    void TransformScaledPointAdd(float x, float y, float z, float size, float *s_x, float *s_y, float *s_size, float *s_zbuf, unsigned int *s_fog);
    void TransformScaledPointBlend(float x, float y, float z, float size, float *s_x, float *s_y, float *s_size, float *s_zbuf, float *s_rhw, unsigned int *s_fog);
    void TransformScaledPointUi(float x, float y, float z, float size, float *s_x, float *s_y, float *s_size);
    void TransformScreenToCamera(float *x, float *y);
    void TriggerRehash();
    void TurnOffGlow();
    void TurnOnGlow();
    void UnlockDynamicVB(int idx);
    void UpdateMinimap(int width, int height, int texture, unsigned char *bits);
    void UpdateRects();
    void UpdateShaders();
    void UpdateTextureFromBitmap(int idx, SBitmap *bmap);
    void WaterLine(float x1, float z1, float x2, float z2, float y);
};

#endif // DENGINE3_GEPARD_H
