#ifndef __LIBDRAGON_MAGMA_FIXED_FUNCTION_H
#define __LIBDRAGON_MAGMA_FIXED_FUNCTION_H

#include <magma.h>

/* Constants */

#define MGFX_LIGHT_COUNT_MAX    8

/* Enums */

typedef enum
{
    MGFX_VERTEX_ATTRIBUTE_POSITION              = 0,
    MGFX_VERTEX_ATTRIBUTE_COLOR                 = 1,
    MGFX_VERTEX_ATTRIBUTE_TEXCOORD              = 2,
    MGFX_VERTEX_ATTRIBUTE_NORMAL                = 3,
    MGFX_VERTEX_ATTRIBUTE_MTX_INDEX             = 4,
} mgfx_vertex_attribute_t;

typedef enum
{
    MGFX_MODES_FLAGS_MATRIX_PALETTE_ENABLED     = 0x01,
    MGFX_MODES_FLAGS_LIGHTING_ENABLED           = 0x02,
    MGFX_MODES_FLAGS_NORMALIZATION_ENABLED      = 0x04,
    MGFX_MODES_FLAGS_FOG_ENABLED                = 0x08,
    MGFX_MODES_FLAGS_ENV_MAP_ENABLED            = 0x10,
    MGFX_MODES_FLAGS_TEXTURING_ENABLED          = 0x20,
    MGFX_MODES_FLAGS_ZBUFFER_ENABLED            = 0x40,
} mgfx_modes_flags_t;

typedef enum
{
    MGFX_VERTEX_COLOR_TARGET_NONE               = 0,
    MGFX_VERTEX_COLOR_TARGET_DIFFUSE            = 1,
    MGFX_VERTEX_COLOR_TARGET_EMISSIVE           = 2,
} mgfx_vertex_color_target_t;

typedef enum
{
    MGFX_BINDING_MODES                          = 0,
    MGFX_BINDING_FOG                            = 1,
    MGFX_BINDING_LIGHTING                       = 2,
    MGFX_BINDING_TEXTURING                      = 3,
    MGFX_BINDING_MATERIAL                       = 4,
    MGFX_BINDING_MATRICES                       = 5,
    MGFX_BINDING_MATRIX_PALETTE                 = 6,
} mgfx_binding_t;

/* RSP side uniform structs */

typedef struct
{
    uint32_t flags;
} __attribute__((packed, aligned(8))) mgfx_modes_t;

typedef struct
{
    int16_t factor_int;
    int16_t offset_int;
    uint16_t factor_frac;
    uint16_t offset_frac;
} __attribute__((packed, aligned(8))) mgfx_fog_t;

typedef struct
{
    int16_t position[MGFX_LIGHT_COUNT_MAX][4];
    int16_t diffuse[MGFX_LIGHT_COUNT_MAX][4];
    int16_t attenuation_int[MGFX_LIGHT_COUNT_MAX][4];
    uint16_t attenuation_frac[MGFX_LIGHT_COUNT_MAX][4];
    int16_t ambient[4];
    uint32_t count;

} __attribute__((packed, aligned(16))) mgfx_lighting_t;

typedef struct 
{
    int16_t diffuse[4];
    int16_t emissive[4];
    uint32_t color_target;
} __attribute__((packed, aligned(8))) mgfx_material_t;

typedef struct
{
    int16_t tex_scale[2];
    int16_t tex_offset[2];
} __attribute__((packed, aligned(8))) mgfx_texturing_t;

typedef struct
{
    int16_t  i[4][4];
    uint16_t f[4][4];
} __attribute__((packed, aligned(16))) mgfx_matrix_t;

typedef struct
{
    mgfx_matrix_t mvp;
    mgfx_matrix_t mv;
    mgfx_matrix_t normal;
} __attribute__((packed, aligned(16))) mgfx_matrices_t;

/* Parameter structs */

typedef struct
{
    mgfx_modes_flags_t flags;
} mgfx_modes_parms_t;

typedef struct
{
    float start;
    float end;
} mgfx_fog_parms_t;

typedef struct
{
    float position[4];
    color_t diffuse_color;
    float radius;
} mgfx_light_t;

typedef struct
{
    color_t ambient_color;
    mgfx_light_t *lights;
    uint32_t light_count;
} mgfx_lighting_parms_t;

typedef struct
{
    color_t diffuse_color;
    color_t emissive_color;
    mgfx_vertex_color_target_t vertex_color_target;
} mgfx_material_parms_t;

typedef struct
{
    float scale[2];
    float offset[2];
} mgfx_texturing_parms_t;

typedef struct
{
    float *model_view_projection;
    float *model_view;
    float *normal;
} mgfx_matrices_parms_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Functions */

mg_shader_t *mgfx_create_vertex_shader();

/* Convert parameter structs to RSP side uniform structs */

void mgfx_get_modes(mgfx_modes_t *dst, const mgfx_modes_parms_t *parms);
void mgfx_get_fog(mgfx_fog_t *dst, const mgfx_fog_parms_t *parms);
void mgfx_get_lighting(mgfx_lighting_t *dst, const mgfx_lighting_parms_t *parms);
void mgfx_get_material(mgfx_material_t *dst, const mgfx_material_parms_t *parms);
void mgfx_get_texturing(mgfx_texturing_t *dst, const mgfx_texturing_parms_t *parms);
void mgfx_get_matrices(mgfx_matrices_t *dst, const mgfx_matrices_parms_t *parms);

/* Set uniforms directly via push constants */

void mgfx_set_modes_inline(const mgfx_modes_parms_t *parms);
void mgfx_set_fog_inline(const mgfx_fog_parms_t *parms);
void mgfx_set_lighting_inline(const mgfx_lighting_parms_t *parms);
void mgfx_set_material_inline(const mgfx_material_parms_t *parms);
void mgfx_set_texturing_inline(const mgfx_texturing_parms_t *parms);
void mgfx_set_matrices_inline(const mgfx_matrices_parms_t *parms);

void mgfx_set_matrix_palette(mg_buffer_t *palette_buffer);

// TODO: RSP side matrix manipulation

#ifdef __cplusplus
}
#endif

#endif
