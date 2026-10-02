// 3dengine/spline.h
// SSpline — spline curve handler
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_SPLINE_H
#define DENGINE3_SPLINE_H

#include <d3d9.h>

#include "chain.h"

struct SGepard;
struct SNode;
struct SHillRing;
struct SSplineRing;
struct SRiverStripRing;

struct SPoint {
    float x;
    float y;
    float z;
};

struct SSpline;
struct SSplineRing;

struct SSpline {
    SGepard* Gepard;
    SPoint lpLine[2];
    IDirect3DDevice9* lpD3DDev;
    SChain<SNode> Nodes;
    float lastx;
    float lastz;
    float lastxkul;
    float lastzkul;
    bool Closed;
    float Altitude;
    float Altitude2;
    bool MiddleSpline;
    int CheckBoxNr;
    bool VisibilityState;
    float Size;
    unsigned int Seed;
    float FirstLeftX;
    float FirstLeftY;
    float FirstLeftZ;
    float FirstCenterX;
    float FirstCenterY;
    float FirstCenterZ;
    float FirstRightX;
    float FirstRightY;
    float FirstRightZ;
    float FirstLeftJoinedX;
    float FirstLeftJoinedY;
    float FirstLeftJoinedZ;
    float FirstRightJoinedX;
    float FirstRightJoinedY;
    float FirstRightJoinedZ;
    float LastLeftX;
    float LastLeftY;
    float LastLeftZ;
    float LastCenterX;
    float LastCenterY;
    float LastCenterZ;
    float LastRightX;
    float LastRightY;
    float LastRightZ;
    float LastLeftJoinedX;
    float LastLeftJoinedY;
    float LastLeftJoinedZ;
    float LastRightJoinedX;
    float LastRightJoinedY;
    float LastRightJoinedZ;
    float URatio;
    float VEndPos;
    float VStartPos;
    int UAlign;
    float JoinedVPos;
    float RiverLength;
    SChain<SRiverStripRing>* RiverStripChain;

    SSpline(SGepard* _Gepard, bool _middlespline, int _checkboxnr);
    ~SSpline();

    void AddNode(float x, float y, float z, bool forced);
    void Close();
    void CopySelectionFrom(SSpline* from);
    void DeleteSelectedNodes();
    void DeselectAllNode();
    void DeselectNode(int index);
    void DrawLines();
    void DrawNodes(bool top, SHillRing* _hillring, SSplineRing* _level);
    void GetCenter(float* _cx, float* _cz);
    bool IsAxleSame(float _x1, float _z1, float _x2, float _z2);
    bool IsFloatNear(float f1, float f2);
    bool IsSectionSame(float _x1, float _z1, float _x2, float _z2);
    int IsSegmentTouch(float x, float z, int* xaxle, int* zaxle);
    void KillSubNodes();
    void MoveNode(SNode* _node, float x, float y, float z);
    void RecalculateControlPoints();
    void Reduce(float ratio);
    void RefreshSubNodes();
    void ReplaceNodesFrom(SSpline* from, SSpline* from2);
    void ReplaceNodesFrom(SSpline* from);
    void SelectAllNode();
    void SelectNode(SNode* _node);
    void SetAltitude2(float _altitude);
    void SetAltitude(float _altitude);
};

#endif // DENGINE3_SPLINE_H
