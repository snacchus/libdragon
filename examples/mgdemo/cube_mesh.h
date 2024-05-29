#ifndef CUBE_MESH_H
#define CUBE_MESH_H

#include "vertex.h"

static const mgfx_vertex_t cube_vertices[] = {
    // +X
    VERTEX( 3.0f, -3.0f, -3.0f, 0.f, 0.f, 1.f,  0.f,  0.f, 0xFF0000FF),
    VERTEX( 3.0f,  3.0f, -3.0f, 1.f, 0.f, 1.f,  0.f,  0.f, 0xFFFF00FF),
    VERTEX( 3.0f,  3.0f,  3.0f, 1.f, 1.f, 1.f,  0.f,  0.f, 0xFFFFFFFF),
    VERTEX( 3.0f, -3.0f,  3.0f, 0.f, 1.f, 1.f,  0.f,  0.f, 0xFF00FFFF),

    // -X
    VERTEX(-3.0f, -3.0f, -3.0f, 0.f, 0.f, 1.f,  0.f,  0.f, 0x000000FF),
    VERTEX(-3.0f, -3.0f,  3.0f, 0.f, 1.f, 1.f,  0.f,  0.f, 0x0000FFFF),
    VERTEX(-3.0f,  3.0f,  3.0f, 1.f, 1.f, 1.f,  0.f,  0.f, 0x00FFFFFF),
    VERTEX(-3.0f,  3.0f, -3.0f, 1.f, 0.f, 1.f,  0.f,  0.f, 0x00FF00FF),
    
    // +Y
    VERTEX(-3.0f,  3.0f, -3.0f, 0.f, 0.f, 0.f,  1.f,  0.f, 0x00FF00FF),
    VERTEX(-3.0f,  3.0f,  3.0f, 0.f, 1.f, 0.f,  1.f,  0.f, 0x00FFFFFF),
    VERTEX( 3.0f,  3.0f,  3.0f, 1.f, 1.f, 0.f,  1.f,  0.f, 0xFFFFFFFF),
    VERTEX( 3.0f,  3.0f, -3.0f, 1.f, 0.f, 0.f,  1.f,  0.f, 0xFFFF00FF),
    
    // -Y
    VERTEX(-3.0f, -3.0f, -3.0f, 0.f, 0.f, 0.f, -1.f,  0.f, 0x000000FF),
    VERTEX( 3.0f, -3.0f, -3.0f, 1.f, 0.f, 0.f, -1.f,  0.f, 0xFF0000FF),
    VERTEX( 3.0f, -3.0f,  3.0f, 1.f, 1.f, 0.f, -1.f,  0.f, 0xFF00FFFF),
    VERTEX(-3.0f, -3.0f,  3.0f, 0.f, 1.f, 0.f, -1.f,  0.f, 0x0000FFFF),
    
    // +Z
    VERTEX(-3.0f, -3.0f,  3.0f, 0.f, 0.f, 0.f,  0.f,  1.f, 0x0000FFFF),
    VERTEX( 3.0f, -3.0f,  3.0f, 1.f, 0.f, 0.f,  0.f,  1.f, 0xFF00FFFF),
    VERTEX( 3.0f,  3.0f,  3.0f, 1.f, 1.f, 0.f,  0.f,  1.f, 0xFFFFFFFF),
    VERTEX(-3.0f,  3.0f,  3.0f, 0.f, 1.f, 0.f,  0.f,  1.f, 0x00FFFFFF),
    
    // -Z
    VERTEX(-3.0f, -3.0f, -3.0f, 0.f, 0.f, 0.f,  0.f, -1.f, 0x000000FF),
    VERTEX(-3.0f,  3.0f, -3.0f, 0.f, 1.f, 0.f,  0.f, -1.f, 0x00FF00FF),
    VERTEX( 3.0f,  3.0f, -3.0f, 1.f, 1.f, 0.f,  0.f, -1.f, 0xFFFF00FF),
    VERTEX( 3.0f, -3.0f, -3.0f, 1.f, 0.f, 0.f,  0.f, -1.f, 0xFF0000FF),
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
