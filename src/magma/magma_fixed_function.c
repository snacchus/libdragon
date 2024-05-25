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

void mgfx_get_fog(mgfx_fog_t *dst, const mgfx_fog_parms_t *parms)
{

}

void mgfx_get_lighting(mgfx_lighting_t *dst, const mgfx_lighting_parms_t *parms)
{

}

void mgfx_get_texturing(mgfx_texturing_t *dst, const mgfx_texturing_parms_t *parms)
{

}

void mgfx_get_modes(mgfx_modes_t *dst, const mgfx_modes_parms_t *parms)
{

}

void mgfx_get_matrices(mgfx_matrices_t *dst, const mgfx_matrices_parms_t *parms)
{

}

void mgfx_set_fog_inline(const mgfx_fog_parms_t *parms)
{

}

void mgfx_set_lighting_inline(const mgfx_lighting_parms_t *parms)
{

}

void mgfx_set_texturing_inline(const mgfx_texturing_parms_t *parms)
{

}

void mgfx_set_modes_inline(const mgfx_modes_parms_t *parms)
{

}

void mgfx_set_matrices_inline(const mgfx_matrices_parms_t *parms)
{

}

void mgfx_set_matrix_palette(mg_buffer_t *palette_buffer)
{
    
}
