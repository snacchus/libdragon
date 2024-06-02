#ifndef CUBE_MESH_H
#define CUBE_MESH_H

#include "vertex.h"

static const mgfx_vertex_t cube_vertices[] = {
    // +X
    VERTEX( 3.0f, -3.0f, -3.0f, 0.f, 0.f, 0x0F, 0x00, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f, -3.0f, 1.f, 0.f, 0x0F, 0x00, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f,  3.0f, 1.f, 1.f, 0x0F, 0x00, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f, -3.0f,  3.0f, 0.f, 1.f, 0x0F, 0x00, 0x00, 0xFFFFFFFF),

    // -X
    VERTEX(-3.0f, -3.0f, -3.0f, 0.f, 0.f, 0x1F, 0x00, 0x00, 0xFFFFFFFF),
    VERTEX(-3.0f, -3.0f,  3.0f, 0.f, 1.f, 0x1F, 0x00, 0x00, 0xFFFFFFFF),
    VERTEX(-3.0f,  3.0f,  3.0f, 1.f, 1.f, 0x1F, 0x00, 0x00, 0xFFFFFFFF),
    VERTEX(-3.0f,  3.0f, -3.0f, 1.f, 0.f, 0x1F, 0x00, 0x00, 0xFFFFFFFF),
    
    // +Y
    VERTEX(-3.0f,  3.0f, -3.0f, 0.f, 0.f, 0x00, 0x1F, 0x00, 0xFFFFFFFF),
    VERTEX(-3.0f,  3.0f,  3.0f, 0.f, 1.f, 0x00, 0x1F, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f,  3.0f, 1.f, 1.f, 0x00, 0x1F, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f, -3.0f, 1.f, 0.f, 0x00, 0x1F, 0x00, 0xFFFFFFFF),
    
    // -Y
    VERTEX(-3.0f, -3.0f, -3.0f, 0.f, 0.f, 0x00, 0x3F, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f, -3.0f, -3.0f, 1.f, 0.f, 0x00, 0x3F, 0x00, 0xFFFFFFFF),
    VERTEX( 3.0f, -3.0f,  3.0f, 1.f, 1.f, 0x00, 0x3F, 0x00, 0xFFFFFFFF),
    VERTEX(-3.0f, -3.0f,  3.0f, 0.f, 1.f, 0x00, 0x3F, 0x00, 0xFFFFFFFF),
    
    // +Z
    VERTEX(-3.0f, -3.0f,  3.0f, 0.f, 0.f, 0x00, 0x00, 0x0F, 0xFFFFFFFF),
    VERTEX( 3.0f, -3.0f,  3.0f, 1.f, 0.f, 0x00, 0x00, 0x0F, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f,  3.0f, 1.f, 1.f, 0x00, 0x00, 0x0F, 0xFFFFFFFF),
    VERTEX(-3.0f,  3.0f,  3.0f, 0.f, 1.f, 0x00, 0x00, 0x0F, 0xFFFFFFFF),
    
    // -Z
    VERTEX(-3.0f, -3.0f, -3.0f, 0.f, 0.f, 0x00, 0x00, 0x1F, 0xFFFFFFFF),
    VERTEX(-3.0f,  3.0f, -3.0f, 0.f, 1.f, 0x00, 0x00, 0x1F, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f, -3.0f, 1.f, 1.f, 0x00, 0x00, 0x1F, 0xFFFFFFFF),
    VERTEX( 3.0f, -3.0f, -3.0f, 1.f, 0.f, 0x00, 0x00, 0x1F, 0xFFFFFFFF),
};

static const uint16_t cube_indices[] = {
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23,
};

#endif
