#include "magma.h"
#include "magma_internal.h"
#include "rspq.h"

DEFINE_RSP_UCODE(rsp_magma);

static uint32_t magma_overlay_id;

void mg_init(void)
{
    magma_overlay_id = rspq_overlay_register(&rsp_magma);
}

void mg_close(void)
{
    rspq_overlay_unregister(magma_overlay_id);
}

mg_shader_t *mg_shader_create(rsp_ucode_t *ucode)
{
    return NULL;
}

void mg_shader_free(mg_shader_t *vertex_shader)
{

}

mg_vertex_loader_t *mg_vertex_loader_create(mg_vertex_loader_parms_t *parms)
{
    return NULL;
}

void mg_vertex_loader_free(mg_vertex_loader_t *vertex_loader)
{

}

mg_pipeline_t *mg_pipeline_create(mg_pipeline_parms_t *parms)
{
    return NULL;
}

void mg_pipeline_free(mg_pipeline_t *pipeline)
{

}

mg_buffer_t *mg_buffer_create(mg_buffer_parms_t *parms)
{
    return NULL;
}

void mg_buffer_free(mg_buffer_t *buffer)
{

}

void *mg_buffer_map(mg_buffer_t *buffer, uint32_t offset, uint32_t size, mg_buffer_map_flags_t flags)
{
    return NULL;
}

void mg_buffer_unmap(mg_buffer_t *buffer)
{

}

void mg_buffer_write(mg_buffer_t *buffer, uint32_t offset, uint32_t size, const void *data)
{

}

mg_resource_set_t *mg_resource_set_create(mg_resource_set_parms_t *parms)
{
    return NULL;
}

void mg_resource_set_free(mg_resource_set_t *resource_set)
{

}

void mg_bind_pipeline(mg_pipeline_t *pipeline)
{

}

void mg_set_culling(mg_culling_parms_t *culling)
{

}

void mg_set_viewport(mg_viewport_t *mg_viewport_t)
{

}

void mg_bind_resource_set(mg_resource_set_t *resource_set)
{

}

void mg_push_constants(uint32_t offset, uint32_t size, const void *data)
{

}

void mg_bind_vertex_buffer(mg_buffer_t *buffer, uint32_t offset)
{

}

void mg_bind_index_buffer(mg_buffer_t *buffer, uint32_t offset)
{

}

void mg_bind_vertex_loader(mg_vertex_loader_t *vertex_loader)
{

}

void mg_draw(mg_input_assembly_parms_t *input_assembly_parms, uint32_t vertex_count, uint32_t first_vertex)
{

}

void mg_draw_indexed(mg_input_assembly_parms_t *input_assembly_parms, uint32_t index_count, uint32_t index_offset, int32_t vertex_offset)
{

}
