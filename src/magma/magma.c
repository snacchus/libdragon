#include "magma.h"
#include "magma_constants.h"
#include "rspq.h"
#include "utils.h"

typedef struct mg_pipeline_s 
{
    rsp_ucode_t *shader_ucode;
    uint32_t uniform_count;
    mg_uniform_t *uniforms;
} mg_pipeline_t;

typedef struct mg_buffer_s 
{
    void *memory;
    uint32_t size;
    bool is_mapped;
} mg_buffer_t;

typedef struct mg_resource_set_s 
{
    rspq_block_t *block;
    void *inline_data;
} mg_resource_set_t;

DEFINE_RSP_UCODE(rsp_magma);

uint32_t mg_overlay_id;

inline void mg_cmd_load_uniform(const void *buffer, uint32_t size, uint32_t dmem_offset)
{
    mg_cmd_write(MG_CMD_LOAD_UNIFORM, PhysicalAddr(buffer), ((size-1) << 16) | dmem_offset);
}

inline void mg_cmd_load_vertices(uint32_t count, uint8_t cache_offset, uint32_t buffer_offset)
{
    assertf(count <= MAGMA_VERTEX_CACHE_COUNT, "too many vertices");
    assertf(cache_offset + count <= MAGMA_VERTEX_CACHE_COUNT, "offset out of range");
    mg_cmd_write(MG_CMD_LOAD_VERTICES, buffer_offset, (cache_offset<<16) | count);
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

mg_pipeline_t *mg_pipeline_create(mg_pipeline_parms_t *parms)
{
    // TODO: check for binary compatibility

    mg_pipeline_t *pipeline = malloc(sizeof(mg_pipeline_t));
    pipeline->shader_ucode = parms->vertex_shader_ucode;

    // TODO: get uniforms from ucode directly somehow?
    if (parms->uniform_count > 0) {
        pipeline->uniforms = calloc(parms->uniform_count, sizeof(mg_uniform_t));
        memcpy(pipeline->uniforms, parms->uniforms, parms->uniform_count * sizeof(mg_uniform_t));
    }

    return pipeline;
}

void mg_pipeline_free(mg_pipeline_t *pipeline)
{
    if (pipeline->uniforms) {
        free(pipeline->uniforms);
    }
    free(pipeline);
}

mg_uniform_t *mg_pipeline_get_uniform(mg_pipeline_t *pipeline, uint32_t binding)
{
    for (uint32_t i = 0; i < pipeline->uniform_count; i++)
    {
        if (pipeline->uniforms[i].binding == binding) {
            return &pipeline->uniforms[i];
        }
    }
    
    assertf(0, "Uniform with binding number %ld was not found", binding);
}

mg_buffer_t *mg_buffer_create(mg_buffer_parms_t *parms)
{
    mg_buffer_t *buffer = malloc(sizeof(mg_buffer_t));
    buffer->memory = malloc_uncached(parms->size);
    buffer->size = parms->size;

    if (parms->initial_data) {
        memcpy(buffer->memory, parms->initial_data, parms->size);
    }

    return buffer;
}

void mg_buffer_free(mg_buffer_t *buffer)
{
    free_uncached(buffer->memory);
    free(buffer);
}

void *mg_buffer_map(mg_buffer_t *buffer, uint32_t offset, uint32_t size, mg_buffer_map_flags_t flags)
{
    assertf(!buffer->is_mapped, "Buffer is already mapped");
    assertf(offset + size < buffer->size, "Map is out of range");

    // TODO: Optimize for different types of access depending on flags

    buffer->is_mapped = true;
    return (uint8_t*)buffer->memory + offset;
}

void mg_buffer_unmap(mg_buffer_t *buffer)
{
    assertf(buffer->is_mapped, "Buffer is not mapped");
    buffer->is_mapped = false;
}

void mg_buffer_write(mg_buffer_t *buffer, uint32_t offset, uint32_t size, const void *data)
{
    assertf(!buffer->is_mapped, "Buffer is mapped");
    assertf(offset + size < buffer->size, "Out of range");

    memcpy((uint8_t*)buffer->memory + offset, data, size);
}

mg_resource_set_t *mg_resource_set_create(mg_resource_set_parms_t *parms)
{
    mg_resource_set_t *resource_set = malloc(sizeof(mg_resource_set_t));

    // Preprocessing

    uint32_t inline_data_size = 0;

    for (uint32_t i = 0; i < parms->binding_count; i++)
    {
        mg_resource_binding_t *binding = &parms->bindings[i];
        switch (binding->type) {
        case MG_RESOURCE_TYPE_INLINE_UNIFORM:
            mg_uniform_t *uniform = mg_pipeline_get_uniform(parms->pipeline, binding->binding);
            inline_data_size += uniform->size;
            break;

        case MG_RESOURCE_TYPE_UNIFORM_BUFFER:
        case MG_RESOURCE_TYPE_STORAGE_BUFFER:
            break;
        }
    }

    if (inline_data_size > 0) {
        resource_set->inline_data = malloc_uncached(inline_data_size);
    }

    // Record block
    // TODO: optimize

    inline_data_size = 0;

    rspq_block_begin();
    for (uint32_t i = 0; i < parms->binding_count; i++)
    {
        mg_resource_binding_t *binding = &parms->bindings[i];
        mg_uniform_t *uniform = mg_pipeline_get_uniform(parms->pipeline, binding->binding);

        switch (binding->type) {
        case MG_RESOURCE_TYPE_UNIFORM_BUFFER:
            uint8_t *uniform_data = (uint8_t*)binding->buffer->memory + binding->offset;
            mg_cmd_load_uniform(uniform_data, uniform->size, uniform->offset);
            break;

        case MG_RESOURCE_TYPE_STORAGE_BUFFER:
            assertf(uniform->size == 8, "Uniform at binding %ld can't be used as a storage buffer", uniform->binding);
            uint32_t storage_data[2] = { PhysicalAddr(binding->buffer->memory), 0 };
            mg_push_constants(uniform->offset, 8, storage_data);
            break;

        case MG_RESOURCE_TYPE_INLINE_UNIFORM:
            uint8_t *inline_data = (uint8_t*)resource_set->inline_data + inline_data_size;
            assertf(((uint32_t)inline_data&0x7) == 0, "Uniform pointer not aligned to 8 bytes");
            memcpy(inline_data, binding->inline_data, uniform->size);
            inline_data_size += uniform->size;
            mg_cmd_load_uniform(inline_data, uniform->size, uniform->offset);
            break;
        }
    }

    resource_set->block = rspq_block_end();

    return resource_set;
}

void mg_resource_set_free(mg_resource_set_t *resource_set)
{
    rspq_block_free(resource_set->block);
    if (resource_set->inline_data) {
        free_uncached(resource_set->inline_data);
    }
    free(resource_set);
}

inline void mg_load_shader(const rsp_ucode_t *shader_ucode)
{
    uint32_t code = PhysicalAddr(shader_ucode->code);
    uint32_t data = PhysicalAddr(shader_ucode->data);
    uint32_t code_size = (uint8_t*)shader_ucode->code_end - shader_ucode->code;
    uint32_t data_size = (uint8_t*)shader_ucode->data_end - shader_ucode->data;
    uint32_t packed_sizes = ((ROUND_UP(code_size, 8) - 1) << 16) | (ROUND_UP(data_size, 8) - 1);
    mg_cmd_write(MG_CMD_SET_SHADER, code, data, packed_sizes);
}

void mg_bind_pipeline(mg_pipeline_t *pipeline)
{
    // TODO: inline?
    mg_load_shader(pipeline->shader_ucode);
}

void mg_bind_resource_set(mg_resource_set_t *resource_set)
{
    // TODO: inline?
    rspq_block_run(resource_set->block);
}

void mg_push_constants(uint32_t offset, uint32_t size, const void *data)
{
    assertf((offset&7) == 0, "offset must be a multiple of 8");
    assertf((size&3) == 0, "size must be a multiple of 4");
    assertf(size <= MAGMA_MAX_UNIFORM_PAYLOAD_SIZE, "size must not be greater than %d", MAGMA_MAX_UNIFORM_PAYLOAD_SIZE);

    uint32_t aligned_size = ROUND_UP(size, 8);

    uint32_t command_id = MG_CMD_PUSH_CONSTANT_MAX;
    uint32_t command_size = MAGMA_MAX_UNIFORM_PAYLOAD_SIZE - 4;

    if (aligned_size <= 8) {
        command_id = MG_CMD_PUSH_CONSTANT_8;
        command_size = 8;
    } else if (aligned_size <= 16) {
        command_id = MG_CMD_PUSH_CONSTANT_16;
        command_size = 16;
    } else if (aligned_size <= 32) {
        command_id = MG_CMD_PUSH_CONSTANT_32;
        command_size = 32;
    } else if (aligned_size <= 64) {
        command_id = MG_CMD_PUSH_CONSTANT_64;
        command_size = 64;
    } else if (aligned_size <= 128) {
        command_id = MG_CMD_PUSH_CONSTANT_128;
        command_size = 128;
    }

    rspq_write_t w = rspq_write_begin(mg_overlay_id, command_id, (MAGMA_PUSH_CONSTANT_HEADER + command_size)/4);

    void *ptr = (void*)(w.pointer - 1);

    // We want to place the payload at an 8-byte aligned address after the end of the command itself
    // w.pointer is already +1 from the actual start of the command
    // the command itself is 2 words, so advance it by one more word
    uint32_t pointer = PhysicalAddr(w.pointer + 1);
    bool aligned = (pointer & 0x7) == 0;

    // If the address right after the command is not aligned, advance by another word
    rspq_write_arg(&w, aligned ? pointer : pointer + 4);
    rspq_write_arg(&w, ((aligned_size-1) << 16) | offset);

    // Write padding for alignment
    if (!aligned) {
        rspq_write_arg(&w, 0);
    }

    const uint32_t *words = data;

    uint32_t word_count = size / sizeof(uint32_t);
    for (uint32_t i = 0; i < word_count; i++)
    {
        rspq_write_arg(&w, words[i]);
    }

    rspq_write_end(&w);

    debug_hexdump(ptr, MAGMA_PUSH_CONSTANT_HEADER + command_size);
}

void mg_bind_vertex_buffer(mg_buffer_t *buffer, uint32_t offset)
{
    // TODO: inline?
    mg_cmd_set_word(offsetof(mg_rsp_state_t, vertex_buffer), PhysicalAddr(buffer->memory) + offset);
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
