#ifndef SCENE_DATA_H
#define SCENE_DATA_H

#include "utility.h"

/*
    A minimalistic "asset database" for this demo.
    The only purpose is to remove some clutter from the main file.
    Using this system for a full game is probably not recommended.
*/

#define TEXTURE_COUNT       3
#define MATERIAL_COUNT      5
#define MESH_COUNT          2
#define OBJECT_COUNT        10
#define LIGHT_COUNT         2
#define MAX_SUBMESH_COUNT   8


/* Textures */
static const char *texture_files[] = {
    "rom:/texture0.ci4.sprite",
    "rom:/texture1.ci4.sprite",
    "rom:/texture2.ci4.sprite",
};


/* Materials */
static const uint32_t material_texture_indices[] = {
    0, 0, 0, 1, 2
};
static const uint32_t material_diffuse_colors[] = { // Currently unused
    0xffffffff,
    0x5a81e6ff,
    0x3b5c34ff,
    0xffffffff,
    0xffffffff,
};


/* Meshes */
static const char *mesh_files[] = {
    "rom:/pipe.model64",
    "rom:/crate.model64",
};


/* Objects */
static const uint32_t object_mesh_ids[] = {
    0, 1, 1, 0, 0, 1, 1, 0, 1, 1
};
static const uint32_t object_material_ids[][MAX_SUBMESH_COUNT] = {
    { 0, 1 }, 
    { 1 }, 
    { 2 }, 
    { 3, 0 }, 
    { 4, 2 }, 
    { 3 }, 
    { 2 }, 
    { 1, 2 }, 
    { 4 }, 
    { 4 }
};
static const float object_positions[][3] = {
    { 0.0f, 0.0f, 0.0f },
    { -10.0f, 0.0f, 0.0f },
    { 10.0f, 0.0f, 0.0f },
    { 0.0f, 10.0f, -10.0f },
    { 10.0f, 10.0f, 0.0f },
    { 5.0f, -5.0f, 9.0f },
    { -15.0f, -8.0f, 0.0f },
    { 4.0f, 3.0f, -12.0f },
    { -14.0f, 6.0f, 8.0f },
    { -4.0f, 16.0f, 0.0f },
};


/* Lights */
static const uint32_t light_colors[] = {
    0x868686ff,
    0xdbbc72ff
};
static const float light_positions[][4] = {
    { 0.196116f, -0.784465f, -0.588348f, 0.0f },
    { 0.0f, 10.0f, 0.0f, 1.0f }
};
static const float light_radii[] = {
    0.0f,
    50.0f
};
static const uint32_t ambient_light_color = 0x101010ff;

/* Environment */
static const uint32_t fog_color = 0x000000ff;
static const float fog_start = 25.0f;
static const float fog_end = 100.0f;

/* Camera */
static const float camera_fov = 65.0f;
static const float camera_near_plane = 1.0f;
static const float camera_far_plane = 100.0f;
static const float camera_start_distance = 30.0f;

#endif
