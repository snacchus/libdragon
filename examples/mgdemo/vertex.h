#ifndef VERTEX_H
#define VERTEX_H

#include <stdint.h>
#include <libdragon.h>

#define S10_5(v) ((v)*(1<<5))
#define S8_8(v) ((v)*(1<<8))
#define N8(v) ((v)*0x7F)

#define POS(x, y, z) { S10_5(x), S10_5(y), S10_5(z) }
#define TEX(s, t) { S8_8(s), S8_8(t) }
#define NRM(x, y, z) { N8(x), N8(y), N8(z) }

#define VERTEX(x, y, z, s, t, nx, ny, nz) { .position = POS((x), (y), (z)), .texcoord = TEX((s), (t)), .normal = NRM((nx), (ny), (nz)) }

typedef struct
{
    int16_t position[3];
    int16_t texcoord[2];
    int8_t normal[3];
} __attribute__((packed)) vertex;


// This describes the vertex layout and format to magma. It is used to create a "vertex loader" object.
static mg_vertex_attribute_descriptor_t vertex_attributes[] = {
    {
        .location = MGFX_VERTEX_ATTRIBUTE_POSITION,
        .format = MG_VERTEX_FORMAT_VEC3_16,
        .offset = 0,
    },
    {
        .location = MGFX_VERTEX_ATTRIBUTE_TEXCOORD,
        .format = MG_VERTEX_FORMAT_VEC2_16,
        .offset = 6,
    },
    {
        .location = MGFX_VERTEX_ATTRIBUTE_NORMAL,
        .format = MG_VERTEX_FORMAT_VEC3_8,
        .offset = 10
    },
};


#endif
