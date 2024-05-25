#include "magma_fixed_function.h"

_Static_assert(sizeof(mgfx_matrix_t) == MGFX_MATRIX_SIZE);

_Static_assert(offsetof(mgfx_vertex_t, position) == MGFX_VTX_POS);
_Static_assert(offsetof(mgfx_vertex_t, packed_normal) == MGFX_VTX_NORM);
_Static_assert(offsetof(mgfx_vertex_t, color) == MGFX_VTX_RGBA);
_Static_assert(offsetof(mgfx_vertex_t, texcoord) == MGFX_VTX_TEX);
_Static_assert(sizeof(mgfx_vertex_t) == MGFX_VTX_SIZE);

DEFINE_RSP_UCODE(rsp_magma_fixed_function);

typedef struct 
{
    mgfx_fog_t fog;
    mgfx_lighting_t lighting;
    mgfx_texturing_t texturing;
    mgfx_modes_t modes;
    mgfx_matrices_t matrices;
    uint32_t matrix_palette;
} mgfx_state_t;

mg_pipeline_t *mgfx_create_pipeline(void)
{
    mg_uniform_t uniforms[] = {
        (mg_uniform_t) {
            .binding = MGFX_BINDING_FOG,
            .offset = offsetof(mgfx_state_t, fog),
            .size = sizeof(mgfx_fog_t)
        },
        (mg_uniform_t) {
            .binding = MGFX_BINDING_LIGHTING,
            .offset = offsetof(mgfx_state_t, lighting),
            .size = sizeof(mgfx_lighting_t)
        },
        (mg_uniform_t) {
            .binding = MGFX_BINDING_TEXTURING,
            .offset = offsetof(mgfx_state_t, texturing),
            .size = sizeof(mgfx_texturing_t)
        },
        (mg_uniform_t) {
            .binding = MGFX_BINDING_MODES,
            .offset = offsetof(mgfx_state_t, modes),
            .size = sizeof(mgfx_modes_t)
        },
        (mg_uniform_t) {
            .binding = MGFX_BINDING_MATRICES,
            .offset = offsetof(mgfx_state_t, matrices),
            .size = sizeof(mgfx_matrices_t)
        },
        (mg_uniform_t) {
            .binding = MGFX_BINDING_MATRIX_PALETTE,
            .offset = offsetof(mgfx_state_t, matrix_palette),
            .size = sizeof(uint32_t)
        },
    };

    return mg_pipeline_create(&(mg_pipeline_parms_t) {
        .vertex_shader_ucode = &rsp_magma_fixed_function,
        .uniform_count = sizeof(uniforms)/sizeof(uniforms[0]),
        .uniforms = uniforms
    });
}

void mgfx_get_fog(mgfx_fog_t *dst, const mgfx_fog_parms_t *parms)
{
    // TODO
}

void mgfx_get_lighting(mgfx_lighting_t *dst, const mgfx_lighting_parms_t *parms)
{
    // TODO
}

void mgfx_get_texturing(mgfx_texturing_t *dst, const mgfx_texturing_parms_t *parms)
{
    dst->tex_scale[0] = parms->scale[0];
    dst->tex_scale[1] = parms->scale[1];
    dst->tex_offset[0] = parms->offset[0];
    dst->tex_offset[1] = parms->offset[1];
}

void mgfx_get_modes(mgfx_modes_t *dst, const mgfx_modes_parms_t *parms)
{
    dst->flags = parms->flags;
}

void mgfx_convert_matrix(mgfx_matrix_t *dst, const float *src)
{
    for (uint32_t i = 0; i < 16; i++)
    {
        int32_t fixed = src[i] * (1<<16);
        dst->i[i] = (int16_t)((fixed & 0xFFFF0000) >> 16);
        dst->f[i] = (uint16_t)(fixed & 0x0000FFFF);
    }
}

void mgfx_get_matrices(mgfx_matrices_t *dst, const mgfx_matrices_parms_t *parms)
{
    mgfx_convert_matrix(&dst->mvp, parms->model_view_projection);
    mgfx_convert_matrix(&dst->mv, parms->model_view);
    mgfx_convert_matrix(&dst->normal, parms->normal);
}

void mgfx_set_fog_inline(const mgfx_fog_parms_t *parms)
{
    mgfx_fog_t fog = {};
    mgfx_get_fog(&fog, parms);
    mg_push_constants(offsetof(mgfx_state_t, fog), sizeof(mgfx_fog_t), &fog);
}

void mgfx_set_lighting_inline(const mgfx_lighting_parms_t *parms)
{
    mgfx_lighting_t lighting = {};
    mgfx_get_lighting(&lighting, parms);
    mg_push_constants(offsetof(mgfx_state_t, lighting), sizeof(mgfx_lighting_t), &lighting);
}

void mgfx_set_texturing_inline(const mgfx_texturing_parms_t *parms)
{
    mgfx_texturing_t texturing = {};
    mgfx_get_texturing(&texturing, parms);
    mg_push_constants(offsetof(mgfx_state_t, texturing), sizeof(mgfx_texturing_t), &texturing);
}

void mgfx_set_modes_inline(const mgfx_modes_parms_t *parms)
{
    mgfx_modes_t modes = {};
    mgfx_get_modes(&modes, parms);
    mg_push_constants(offsetof(mgfx_state_t, modes), sizeof(mgfx_modes_t), &modes);
}

void mgfx_set_matrices_inline(const mgfx_matrices_parms_t *parms)
{
    mgfx_matrices_t matrices = {};
    mgfx_get_matrices(&matrices, parms);
    mg_push_constants(offsetof(mgfx_state_t, matrices), sizeof(mgfx_matrices_t), &matrices);
}

void mgfx_set_matrix_palette(mg_buffer_t *palette_buffer)
{
    assertf(0, "Not implemented");
}
