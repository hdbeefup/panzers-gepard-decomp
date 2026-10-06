// src/game/unitanim_model.h
// The model calls of the unit animations: thin wrappers over the named
// SIModel slots (+0x38, +0x48, +0x4c, +0x54, +0x64, +0x68, +0x78, +0x84,
// +0xf0; bodies in src/3dengine/pz/pzmodel.cpp) and the SGepard prototype
// sequence queries (+0x30 / +0x34, still Slot_XX in igepardhd.h).

#ifndef PZ_GAME_UNITANIM_MODEL_H
#define PZ_GAME_UNITANIM_MODEL_H

namespace pz {

struct SIModel;

// +0x38 HD 0x6db2a0: fade node (roof) in or out over 1 s, node2 swaps in.
void AnimModelSetNodeFade(SIModel* model, bool show, int node, int node2);
// +0x48 HD 0x6da4d0: node user transform = position, yaw about y and a tilt
// toward (tiltX, tiltZ) (atan approximation 0x661440 of the length).
void AnimModelSetNodeTilt(SIModel* model, int node, float x, float y, float z, float yaw, float tiltX, float tiltZ);
// +0x4c HD 0x6da330: node user transform = position and two rotations
// (a about z, then b about x); wheels.
void AnimModelSetNodeRotation(SIModel* model, int node, float x, float y, float z, float a, float b);
// +0x54 HD 0x6d7d30: world position of a node (translation of +0x58).
void AnimModelGetNodePosition(SIModel* model, int node, float* xyz);
// +0x64 HD 0x6da810: texture animation 2 (rotate the UVs by angle about u, v).
void AnimModelSetNodeTexRotation(SIModel* model, int node, float u, float v, float angle);
// +0x68 HD 0x6da7a0: texture animation 1 (scroll the UVs by u, v); track belts.
void AnimModelSetNodeTexScroll(SIModel* model, int node, float u, float v);
// +0x78 HD 0x6d78c0: Type of the playing sequence (1 stand, 2 move, 4/5 spin).
int AnimModelSequenceType(SIModel* model);
// +0x84 HD 0x6d7f30: Length of a named sequence, 0 if the model has none.
float AnimModelSequenceLength(SIModel* model, const char* name);
// +0xf0 HD 0x6da2c0: second colour override (fog of war tint of buildings).
void AnimModelSetColor2(SIModel* model, bool on, unsigned color);

// Gepard +0x30 HD 0x67c0a0: index of a sequence in a model prototype, -1.
int AnimProtoFindSequence(int proto, const char* name);
// Gepard +0x34 HD 0x67c170: the prototype's sequence names.
int AnimProtoSequenceCount(int proto);
const char* AnimProtoSequenceName(int proto, int index);

} // namespace pz

#endif // PZ_GAME_UNITANIM_MODEL_H
