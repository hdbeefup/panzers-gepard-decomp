#ifndef _C4D_MAP_OBJECTS_H_
#define _C4D_MAP_OBJECTS_H_

#include "defines.h"

typedef struct c4d_layer
{
	char* name;
	uint32_t attributes;
	uint8_t* blend_strength;
	char* extrastr;
};


typedef struct c4d_doodad
{
	char* name;
	float x, y, z, r;
	float b, c;
};


typedef struct c4d_ambient_sound
{
	char* name;
	float x, y, z, range;
};


typedef struct c4d_decal
{
	char* name;
	uint32_t a, b, c;
};


typedef struct c4d_lake
{
	char* name;
	float a, b, c, d;
	uint32_t e, f, g;
};


typedef struct c4d_unit
{
	char* class_name;
};


typedef struct c4d_effect
{
	char* name;
	float a, b, c, d, e;
};


typedef struct c4d_location
{
    char* name;
    uint32_t x1, y1, x2, y2, a;
};


typedef struct c4d_path
{
    char* name;
    uint32_t a, node_count;
    cheetah_vec2_t* nodes;
    uint8_t b;
};


typedef struct c4d_road_node
{
    float x, y, r1, r2, a;
    uint32_t jcn;
};


typedef struct c4d_road
{
    char* name;
    float tesselation_distance, b;
    uint32_t mirror_flags;

    uint32_t node_count;
    c4d_road_node* nodes;
};


typedef struct c4d_road_junction
{
    char* name;
    float a, b, d, e, f, g, h;
    uint32_t c, i;
};

#endif