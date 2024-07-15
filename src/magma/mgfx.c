#include "mgfx.h"
#include <math.h>
#include <float.h>
#include "utils.h"

_Static_assert(sizeof(mgfx_matrix_t) == MGFX_MATRIX_SIZE);

_Static_assert(offsetof(mgfx_vertex_t, position) == MGFX_VTX_POS);
_Static_assert(offsetof(mgfx_vertex_t, packed_normal) == MGFX_VTX_NORM);
_Static_assert(offsetof(mgfx_vertex_t, color) == MGFX_VTX_RGBA);
_Static_assert(offsetof(mgfx_vertex_t, texcoord) == MGFX_VTX_TEX);
_Static_assert(sizeof(mgfx_vertex_t) == MGFX_VTX_SIZE);

_Static_assert(offsetof(mgfx_light_t, position) == MGFX_LIGHT_POSITION);
_Static_assert(offsetof(mgfx_light_t, color) == MGFX_LIGHT_COLOR);
_Static_assert(offsetof(mgfx_light_t, attenuation_int) == MGFX_LIGHT_ATT_INT);
_Static_assert(offsetof(mgfx_light_t, attenuation_frac) == MGFX_LIGHT_ATT_FRAC);
_Static_assert(sizeof(mgfx_light_t) == MGFX_LIGHT_SIZE);

#define U8_TO_I16(x) ((x) << 7)
#define FLOAT_TO_S10_5(x) ((x) * (1<<5))
#define FLOAT_TO_I16(x) (CLAMP(x, -1.f, 1.f) * 0x7FFF)

DEFINE_RSP_UCODE(rsp_mgfx);

typedef struct 
{
    mgfx_fog_t fog;
    mgfx_lighting_t lighting;
    mgfx_texturing_t texturing;
    mgfx_modes_t modes;
    mgfx_matrices_t matrices;
} mgfx_state_t;

static const mg_uniform_t mgfx_uniforms[] = {
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
    }
};

mg_pipeline_t *mgfx_create_pipeline(void)
{
    return mg_pipeline_create(&(mg_pipeline_parms_t) {
        .vertex_shader_ucode = &rsp_mgfx,
        .uniform_count = sizeof(mgfx_uniforms)/sizeof(mgfx_uniforms[0]),
        .uniforms = mgfx_uniforms
    });
}

void mgfx_get_fog(mgfx_fog_t *dst, const mgfx_fog_parms_t *parms)
{
    float diff = parms->end - parms->start;
    // start == end is undefined, so disable fog by setting the factor to 0
    float factor = fabsf(diff) < FLT_MIN ? 0.0f : 1.0f / diff;
    float offset = parms->start;

    // Convert to s15.16 and premultiply with 1.15 conversion factor
    int32_t factor_fx = factor * (1<<(16 + 7 + (8 - MGFX_VTX_POS_SHIFT)));
    int16_t offset_fx = offset * (1<<MGFX_VTX_POS_SHIFT);

    int16_t factor_i = factor_fx >> 16;
    uint16_t factor_f = factor_fx & 0xFFFF;

    dst->factor_int = factor_i;
    dst->offset_int = offset_fx;
    dst->factor_frac = factor_f;
    dst->offset_frac = 0;
}

inline void color_to_i16(int16_t *dst, color_t color)
{
    dst[0] = U8_TO_I16(color.r);
    dst[1] = U8_TO_I16(color.g);
    dst[2] = U8_TO_I16(color.b);
    dst[3] = U8_TO_I16(color.a);
}

void mgfx_get_light(mgfx_light_t *dst, const mgfx_light_parms_t *parms)
{
    color_to_i16(dst->color, parms->color);

    // The user should pre-transform positional lights into eye-space

    const float *p = parms->position;
    // If W is zero then the light is directional
    if (p[3] == 0.0f) {
        // Pre-normalize the light direction
        float magnitude = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
        dst->position[0] = -FLOAT_TO_I16(p[0] / magnitude);
        dst->position[1] = -FLOAT_TO_I16(p[1] / magnitude);
        dst->position[2] = -FLOAT_TO_I16(p[2] / magnitude);
        dst->position[3] = FLOAT_TO_I16(0.0f);
    } else {
        assertf(parms->radius > 0.0f, "Light radius must be greater than zero!");
        
        dst->position[0] = FLOAT_TO_S10_5(p[0]);
        dst->position[1] = FLOAT_TO_S10_5(p[1]);
        dst->position[2] = FLOAT_TO_S10_5(p[2]);
        dst->position[3] = FLOAT_TO_S10_5(1.0f);

        float const_att = 1.0f;
        float linear_att = 2.0f / parms->radius;
        float quad_att = 1.0f / (parms->radius * parms->radius);

        uint32_t const_att_fx = const_att * (1 << (16 - 1));
        uint32_t linear_att_fx = linear_att * (1 << (16 - 5));
        uint32_t quad_att_fx = quad_att * (1 << (16 + 5));

        dst->attenuation_int[0] = const_att_fx >> 16;
        dst->attenuation_int[1] = linear_att_fx >> 16;
        dst->attenuation_int[2] = quad_att_fx >> 16;
        dst->attenuation_int[3] = 0;

        dst->attenuation_frac[0] = const_att_fx & 0xFFFF;
        dst->attenuation_frac[1] = linear_att_fx & 0xFFFF;
        dst->attenuation_frac[2] = quad_att_fx & 0xFFFF;
        dst->attenuation_frac[3] = 0;
    }
}

void mgfx_get_lighting(mgfx_lighting_t *dst, const mgfx_lighting_parms_t *parms)
{
    assertf(parms->light_count <= MGFX_LIGHT_COUNT_MAX, "Light count must be %d or less!", MGFX_LIGHT_COUNT_MAX);

    dst->count = parms->light_count;
    color_to_i16(dst->ambient, parms->ambient_color);
    for (size_t i = 0; i < parms->light_count; i++)
    {
        mgfx_get_light(&dst->lights[i], &parms->lights[i]);
    }
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
    mg_inline_uniform(&mgfx_uniforms[MGFX_BINDING_FOG], &fog);
}

void mgfx_set_lighting_inline(const mgfx_lighting_parms_t *parms)
{
    mgfx_lighting_t lighting = {};
    mgfx_get_lighting(&lighting, parms);
    mg_inline_uniform(&mgfx_uniforms[MGFX_BINDING_LIGHTING], &lighting);
}

void mgfx_set_texturing_inline(const mgfx_texturing_parms_t *parms)
{
    mgfx_texturing_t texturing = {};
    mgfx_get_texturing(&texturing, parms);
    mg_inline_uniform(&mgfx_uniforms[MGFX_BINDING_TEXTURING], &texturing);
}

void mgfx_set_modes_inline(const mgfx_modes_parms_t *parms)
{
    mgfx_modes_t modes = {};
    mgfx_get_modes(&modes, parms);
    mg_inline_uniform(&mgfx_uniforms[MGFX_BINDING_MODES], &modes);
}

void mgfx_set_matrices_inline(const mgfx_matrices_parms_t *parms)
{
    mgfx_matrices_t matrices = {};
    mgfx_get_matrices(&matrices, parms);
    mg_inline_uniform(&mgfx_uniforms[MGFX_BINDING_MATRICES], &matrices);
}

// TODO: RSP side matrix manipulation.
//       This could be done using a separate overlay which acts as a batch matrix processor.
