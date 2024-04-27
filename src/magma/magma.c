#include "magma.h"
#include "magma_constants.h"
#include "rspq.h"
#include "utils.h"

DEFINE_RSP_UCODE(rsp_magma);

uint32_t mg_overlay_id;

inline void mg_cmd_load_vertices(uint32_t count, uint8_t cache_offset, uint32_t buffer_offset)
{
    assertf(count <= MAGMA_VERTEX_CACHE_COUNT, "too many vertices");
    assertf(cache_offset + count <= MAGMA_VERTEX_CACHE_COUNT, "offset out of range");
    mg_cmd_write(MG_CMD_LOAD_VERTICES, count, (cache_offset<<24) | buffer_offset);
}

inline void mg_cmd_draw_indices(uint8_t index0, uint8_t index1, uint8_t index2)
{
    mg_cmd_write(MG_CMD_DRAW_INDICES, (index0 << 16) | (index1 << 8) | index2);
}

void mg_init(void)
{
    mg_overlay_id = rspq_overlay_register(&rsp_magma);
}

void mg_close(void)
{
    rspq_overlay_unregister(mg_overlay_id);
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

#define TRI_LIST_ADVANCE_COUNT ROUND_DOWN(MAGMA_VERTEX_CACHE_COUNT, 3)

void mg_draw(mg_input_assembly_parms_t *input_assembly_parms, uint32_t vertex_count, uint32_t first_vertex)
{
    uint32_t advance_count = 0;
    switch (input_assembly_parms->primitive_topology) {
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        advance_count = TRI_LIST_ADVANCE_COUNT;
        break;
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        advance_count = MAGMA_VERTEX_CACHE_COUNT;
        break;
    }

    for (uint32_t current_vertex = 0; current_vertex < vertex_count; current_vertex += advance_count)
    {
        uint32_t load_count = MIN(advance_count, vertex_count - current_vertex);
        mg_cmd_load_vertices(load_count, 0, current_vertex + first_vertex);

        switch (input_assembly_parms->primitive_topology) {
        case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        {
            size_t prim_count = load_count / 3;
            for (size_t i = 0; i < prim_count; i++) mg_cmd_draw_indices(3*i, 3*i+1, 3*i+2);
            break;
        }
        case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        {
            size_t prim_count = MAX(0, load_count - 2);
            for (size_t i = 0; i < prim_count; i++) mg_cmd_draw_indices(i, i + 1 + i%2, i + 2 - i%2);
            break;
        }
        case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        {
            size_t prim_count = MAX(0, load_count - 2);
            for (size_t i = 0; i < prim_count; i++) mg_cmd_draw_indices(i+1, i+2, 0);
            break;
        }
        }
    }
}

#define SPECIAL_INDEX UINT16_MAX

typedef struct
{
    uint16_t start;
    uint16_t count;
} vertex_cache_block;

typedef struct 
{
    vertex_cache_block blocks[MAGMA_VERTEX_CACHE_COUNT];
    uint32_t block_count;
    uint32_t total_count;
} vertex_cache;

static void vertex_cache_clear(vertex_cache *cache)
{
    cache->block_count = 0;
    cache->total_count = 0;
}

static bool vertex_cache_try_insert(vertex_cache *cache, uint16_t index)
{
    if (cache->total_count >= MAGMA_VERTEX_CACHE_COUNT) {
        return false;
    }

    size_t new_block_index = cache->block_count;

    // Find existing block to insert into
    for (size_t i = 0; i < cache->block_count; i++)
    {
        vertex_cache_block *block = &cache->blocks[i];
        if (block->count == 0) {
            continue;
        }

        uint16_t diff = index - block->start;
        if (diff < block->count) {
            // Index is already contained in this block
            return true;
        }
        if (diff == block->count) {
            // Index is the next one after this block. Grow it by one.
            block->count++;
            cache->total_count++;
            // TODO: coalesce blocks
            return true;
        }

        if (block->start > index) {
            // Blocks are always sorted. That means if the current block's start
            // is after the index, we already know that none of the existing blocks
            // will contain the index. The new block we need to create has to be inserted
            // in the current block's place.
            new_block_index = i;
            break;
        }
    }

    // Create new block
    assertf(cache->block_count < MAGMA_VERTEX_CACHE_COUNT, "Maximum number of blocks reached! This is a bug within magma.");

    // TODO: Insert at 
    vertex_cache_block *new_block = &cache->blocks[cache->block_count++];
    new_block->start = index;
    new_block->count = 1;
    cache->total_count++;
    return true;
}

static bool vertex_cache_find(const vertex_cache *cache, uint16_t index, uint8_t *cache_index)
{
    uint16_t cur_offset = 0;
    for (size_t i = 0; i < cache->block_count; i++)
    {
        const vertex_cache_block *block = &cache->blocks[i];
        uint16_t diff = index - block->start;
        if (diff < block->count) {
            *cache_index = cur_offset + diff;
            return true;
        }
        cur_offset += block->count;
    }

    return false;
}

static void vertex_cache_load(const vertex_cache *cache, int32_t offset)
{
    uint32_t cache_offset = 0;
    for (size_t i = 0; i < cache->block_count; i++)
    {
        if (cache->blocks[i].count == 0) continue;
        mg_cmd_load_vertices(cache->blocks[i].count, cache_offset, cache->blocks[i].start + offset);
        cache_offset += cache->blocks[i].count;
    }
}

static uint32_t prepare_batch(const uint16_t *indices, int32_t vertex_offset, uint32_t max_count, vertex_cache *cache)
{
    vertex_cache_clear(cache);

    uint32_t count = 0;
    for (; count < max_count; count++)
    {
        if (!vertex_cache_try_insert(cache, indices[count])) {
            break;
        }
    }

    assertf(cache->total_count <= MAGMA_VERTEX_CACHE_COUNT, "Vertex batch is too big! This is a bug within magma.");

    vertex_cache_load(cache, vertex_offset);
    return count;
}

static void draw_batch(const uint16_t *indices, uint32_t current_index, uint32_t batch_count, const vertex_cache *cache)
{
    uint8_t prim_indices[3];
    for (size_t i = 0; i < batch_count; i++)
    {
        uint16_t index = indices[current_index + i];

        uint8_t cache_index = 0;
        bool found = vertex_cache_find(cache, index, &cache_index);
        assertf(found, "Index not found in vertex batch! This is a bug within magma.");

        prim_indices[i%3] = cache_index;
        if (i%3 == 2) {
            mg_cmd_draw_indices(prim_indices[0], prim_indices[1], prim_indices[2]);
        }
    }
}

void mg_draw_indexed(mg_input_assembly_parms_t *input_assembly_parms, const uint16_t *indices, uint32_t index_count, int32_t vertex_offset)
{
    vertex_cache cache;
    uint32_t current_index = 0;

    while (current_index < index_count)
    {
        uint32_t batch_index_count = prepare_batch(indices + current_index, vertex_offset, index_count - current_index, &cache);
        draw_batch(indices, current_index, batch_index_count, &cache);
        
        current_index += batch_index_count;
    }
}
