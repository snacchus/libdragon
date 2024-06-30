#include "magma.h"
#include "magma_constants.h"
#include "rspq.h"
#include "rdpq.h"
#include "utils.h"
#include "../rdpq/rdpq_internal.h"

typedef struct mg_pipeline_s 
{
    rsp_ucode_t *shader_ucode;
    uint32_t uniform_count;
    mg_uniform_t *uniforms;
} mg_pipeline_t;

typedef struct mg_buffer_s 
{
    mg_buffer_flags_t flags;
    void *memory;
    uint32_t size;
    bool is_mapped;
} mg_buffer_t;

typedef struct mg_resource_set_s 
{
    rspq_block_t *block;
    void *embedded_data;
} mg_resource_set_t;

DEFINE_RSP_UCODE(rsp_magma);

uint32_t mg_overlay_id;

void mg_load_uniform_raw(uint32_t offset, uint32_t size, const void *data)
{
    assertf(size > 0, "size must be greater than 0");
    mg_cmd_write(MG_CMD_LOAD_UNIFORM, PhysicalAddr(data), ((size-1) << 16) | offset);
}

void mg_draw_begin(void)
{
    __rdpq_autosync_use(AUTOSYNC_PIPE | AUTOSYNC_TILES | AUTOSYNC_TMEMS);
}

void mg_draw_end(void)
{
    mg_cmd_write(MG_CMD_DRAW_END);
}

void mg_load_vertices(uint32_t buffer_offset, uint8_t cache_offset, uint32_t count)
{
    assertf(count > 0, "count must be greater than 0");
    assertf(count <= MAGMA_VERTEX_CACHE_COUNT, "too many vertices");
    assertf(cache_offset + count <= MAGMA_VERTEX_CACHE_COUNT, "offset out of range");
    mg_cmd_write(MG_CMD_LOAD_VERTICES, buffer_offset, (cache_offset<<16) | count);
}

void mg_draw_indices(uint8_t index0, uint8_t index1, uint8_t index2)
{
    assertf(index0 <= MAGMA_VERTEX_CACHE_COUNT, "index0 is out of range");
    assertf(index1 <= MAGMA_VERTEX_CACHE_COUNT, "index1 is out of range");
    assertf(index2 <= MAGMA_VERTEX_CACHE_COUNT, "index2 is out of range");
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

mg_pipeline_t *mg_pipeline_create(const mg_pipeline_parms_t *parms)
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

const mg_uniform_t *mg_pipeline_get_uniform(mg_pipeline_t *pipeline, uint32_t binding)
{
    for (uint32_t i = 0; i < pipeline->uniform_count; i++)
    {
        if (pipeline->uniforms[i].binding == binding) {
            return &pipeline->uniforms[i];
        }
    }
    
    assertf(0, "Uniform with binding number %ld was not found", binding);
}

mg_buffer_t *mg_buffer_create(const mg_buffer_parms_t *parms)
{
    mg_buffer_t *buffer = malloc(sizeof(mg_buffer_t));
    buffer->flags = parms->flags;
    buffer->size = parms->size;

    if (parms->backing_memory) {
        buffer->memory = parms->backing_memory;
    } else {
        buffer->memory = malloc_uncached(parms->size);
    }

    return buffer;
}

void mg_buffer_free(mg_buffer_t *buffer)
{
    free_uncached(buffer->memory);
    free(buffer);
}

static void mg_buffer_check_cpu_write(mg_buffer_t *buffer)
{
    assertf((buffer->flags & MG_BUFFER_FLAGS_ACCESS_CPU_WRITE) != 0, "This buffer has no write access on CPU!");
}

static void mg_buffer_check_cpu_read(mg_buffer_t *buffer)
{
    assertf((buffer->flags & MG_BUFFER_FLAGS_ACCESS_CPU_READ) != 0, "This buffer has no read access on CPU!");
}

static void mg_buffer_check_rcp_write(mg_buffer_t *buffer)
{
    assertf((buffer->flags & MG_BUFFER_FLAGS_ACCESS_RCP_WRITE) != 0, "This buffer has no write access on RCP!");
}

static void mg_buffer_check_rcp_read(mg_buffer_t *buffer)
{
    assertf((buffer->flags & MG_BUFFER_FLAGS_ACCESS_RCP_READ) != 0, "This buffer has no read access on RCP!");
}

void *mg_buffer_map(mg_buffer_t *buffer, uint32_t offset, uint32_t size, mg_buffer_map_flags_t flags)
{
    assertf((flags & (MG_BUFFER_MAP_FLAGS_READ|MG_BUFFER_MAP_FLAGS_WRITE)) != 0, "Buffer must be mapped with at least read or write access!");
    assertf(!buffer->is_mapped, "Buffer is already mapped");
    assertf(offset + size < buffer->size, "Map is out of range");

    // TODO: Optimize for different types of access depending on flags

    if (flags & MG_BUFFER_MAP_FLAGS_READ) {
        mg_buffer_check_cpu_read(buffer);
    }

    if (flags & MG_BUFFER_MAP_FLAGS_WRITE) {
        mg_buffer_check_cpu_write(buffer);
    }

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
    assertf(offset + size <= buffer->size, "Out of range");

    mg_buffer_check_cpu_write(buffer);
    
    memcpy((uint8_t*)buffer->memory + offset, data, size);
}

mg_resource_set_t *mg_resource_set_create(const mg_resource_set_parms_t *parms)
{
    mg_resource_set_t *resource_set = malloc(sizeof(mg_resource_set_t));

    // Preprocessing

    uint32_t embedded_data_size = 0;

    for (uint32_t i = 0; i < parms->binding_count; i++)
    {
        const mg_resource_binding_t *binding = &parms->bindings[i];
        switch (binding->type) {
        case MG_RESOURCE_TYPE_EMBEDDED_UNIFORM:
            const mg_uniform_t *uniform = mg_pipeline_get_uniform(parms->pipeline, binding->binding);
            embedded_data_size += uniform->size;
            break;

        case MG_RESOURCE_TYPE_UNIFORM_BUFFER:
        case MG_RESOURCE_TYPE_STORAGE_BUFFER:
            mg_buffer_check_rcp_read(binding->buffer);
            break;
        }
    }

    if (embedded_data_size > 0) {
        resource_set->embedded_data = malloc_uncached(embedded_data_size);
    }

    // Record block
    // TODO: optimize

    embedded_data_size = 0;

    rspq_block_begin();
    for (uint32_t i = 0; i < parms->binding_count; i++)
    {
        const mg_resource_binding_t *binding = &parms->bindings[i];
        const mg_uniform_t *uniform = mg_pipeline_get_uniform(parms->pipeline, binding->binding);

        switch (binding->type) {
        case MG_RESOURCE_TYPE_UNIFORM_BUFFER:
            uint8_t *uniform_data = (uint8_t*)binding->buffer->memory + binding->offset;
            mg_load_uniform(uniform, uniform_data);
            break;

        case MG_RESOURCE_TYPE_STORAGE_BUFFER:
            assertf(uniform->size == 8, "Uniform at binding %ld can't be used as a storage buffer", uniform->binding);
            uint32_t storage_data[2] = { PhysicalAddr(binding->buffer->memory), 0 };
            mg_inline_uniform_raw(uniform->offset, 8, storage_data);
            break;

        case MG_RESOURCE_TYPE_EMBEDDED_UNIFORM:
            uint8_t *embedded_data = (uint8_t*)resource_set->embedded_data + embedded_data_size;
            assertf(((uint32_t)embedded_data&0x7) == 0, "Uniform pointer not aligned to 8 bytes");
            memcpy(embedded_data, binding->embedded_data, uniform->size);
            embedded_data_size += uniform->size;
            mg_load_uniform(uniform, embedded_data);
            break;
        }
    }

    resource_set->block = rspq_block_end();

    return resource_set;
}

void mg_resource_set_free(mg_resource_set_t *resource_set)
{
    rspq_block_free(resource_set->block);
    if (resource_set->embedded_data) {
        free_uncached(resource_set->embedded_data);
    }
    free(resource_set);
}

void mg_load_shader(const rsp_ucode_t *shader_ucode)
{
    uint32_t code = PhysicalAddr(shader_ucode->code);
    uint32_t code_size = (uint8_t*)shader_ucode->code_end - shader_ucode->code;
    mg_cmd_write(MG_CMD_SET_SHADER, code, ROUND_UP(code_size, 8) - 1);
}

void mg_bind_pipeline(mg_pipeline_t *pipeline)
{
    // TODO: inline?
    mg_load_shader(pipeline->shader_ucode);
}

mg_rsp_viewport_t mg_viewport_to_rsp_state(const mg_viewport_t *viewport)
{
    float half_width = viewport->width / 2;
    float half_height = viewport->height / 2;
    float depth_diff = viewport->maxDepth - viewport->minDepth;
    float half_depth = depth_diff / 2;
    return (mg_rsp_viewport_t) {
        .scale = {
            half_width * 8,
            half_height * 8,
            half_depth * 0x7FFF * 2,
            2
        },
        .offset = {
            (viewport->x + half_width) * 4,
            (viewport->y + half_height) * 4,
            (viewport->minDepth + half_depth) * 0x7FFF,
            0
        }
    };
}


void mg_set_viewport(const mg_viewport_t *viewport)
{
    mg_rsp_viewport_t rsp_viewport = mg_viewport_to_rsp_state(viewport);
    uint32_t value0 = (rsp_viewport.scale[0] << 16) | rsp_viewport.scale[1];
    uint32_t value1 = (rsp_viewport.scale[2] << 16) | rsp_viewport.scale[3];
    uint32_t value2 = (rsp_viewport.offset[0] << 16) | rsp_viewport.offset[1];
    uint32_t value3 = (rsp_viewport.offset[2] << 16) | rsp_viewport.offset[3];
    mg_cmd_set_quad(offsetof(mg_rsp_state_t, viewport), value0, value1, value2, value3);
}

void mg_bind_resource_set(mg_resource_set_t *resource_set)
{
    // TODO: inline?
    rspq_block_run(resource_set->block);
}

void mg_inline_uniform_raw(uint32_t offset, uint32_t size, const void *data)
{
    assertf((offset&7) == 0, "offset must be a multiple of 8");
    assertf((size&3) == 0, "size must be a multiple of 4");
    assertf(size <= MAGMA_MAX_UNIFORM_PAYLOAD_SIZE, "size must not be greater than %d", MAGMA_MAX_UNIFORM_PAYLOAD_SIZE);

    uint32_t aligned_size = ROUND_UP(size, 8);

    uint32_t command_id = MG_CMD_INLINE_UNIFORM_MAX;
    uint32_t command_size = MAGMA_MAX_UNIFORM_PAYLOAD_SIZE;

    if (aligned_size <= 8) {
        command_id = MG_CMD_INLINE_UNIFORM_8;
        command_size = 8;
    } else if (aligned_size <= 16) {
        command_id = MG_CMD_INLINE_UNIFORM_16;
        command_size = 16;
    } else if (aligned_size <= 32) {
        command_id = MG_CMD_INLINE_UNIFORM_32;
        command_size = 32;
    } else if (aligned_size <= 64) {
        command_id = MG_CMD_INLINE_UNIFORM_64;
        command_size = 64;
    } else if (aligned_size <= 128) {
        command_id = MG_CMD_INLINE_UNIFORM_128;
        command_size = 128;
    }

    rspq_write_t w = rspq_write_begin(mg_overlay_id, command_id, (MAGMA_INLINE_UNIFORM_HEADER + command_size)/4);

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
}

void mg_bind_vertex_buffer(mg_buffer_t *buffer, uint32_t offset)
{
    // TODO: inline?
    mg_buffer_check_rcp_read(buffer);
    mg_cmd_set_word(offsetof(mg_rsp_state_t, vertex_buffer), PhysicalAddr(buffer->memory) + offset);
}

#define TRI_LIST_ADVANCE_COUNT ROUND_DOWN(MAGMA_VERTEX_CACHE_COUNT, 3)

void mg_draw(const mg_input_assembly_parms_t *input_assembly_parms, uint32_t vertex_count, uint32_t first_vertex)
{
    uint32_t cache_offset = 0;
    uint32_t next_cache_offset = 0;
    uint32_t advance_count = 0;
    uint32_t batch_size = 0;
    switch (input_assembly_parms->primitive_topology) {
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        advance_count = TRI_LIST_ADVANCE_COUNT;
        batch_size = TRI_LIST_ADVANCE_COUNT;
        break;
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        advance_count = MAGMA_VERTEX_CACHE_COUNT - 2;
        batch_size = MAGMA_VERTEX_CACHE_COUNT;
        break;
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        advance_count = MAGMA_VERTEX_CACHE_COUNT - 1;
        batch_size = MAGMA_VERTEX_CACHE_COUNT;
        break;
    }

    for (uint32_t current_vertex = 0; current_vertex < vertex_count; current_vertex += advance_count - cache_offset)
    {
        cache_offset = next_cache_offset;
        uint32_t load_count = MIN(batch_size - cache_offset, vertex_count - current_vertex);
        mg_load_vertices(current_vertex + first_vertex, cache_offset, load_count);

        switch (input_assembly_parms->primitive_topology) {
            case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            {
                size_t prim_count = load_count / 3;
                for (size_t i = 0; i < prim_count; i++) mg_draw_indices(3*i, 3*i+1, 3*i+2);
                break;
            }
            case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            {
                size_t prim_count = MAX(0, load_count - 2);
                for (size_t i = 0; i < prim_count; i++) mg_draw_indices(i, i + 1 + i%2, i + 2 - i%2);
                break;
            }
            case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            {
                size_t prim_count = MAX(0, load_count - 2 + cache_offset);
                for (size_t i = 0; i < prim_count; i++) mg_draw_indices(i+1, i+2, 0);
                next_cache_offset = 1;
                break;
            }
        }
    }
}

#define SPECIAL_INDEX UINT16_MAX

typedef struct vertex_cache_block_s vertex_cache_block;

typedef struct vertex_cache_block_s
{
    uint16_t start;
    uint16_t count;
    vertex_cache_block *next;
} vertex_cache_block;

typedef struct 
{
    vertex_cache_block blocks[MAGMA_VERTEX_CACHE_COUNT];
    vertex_cache_block *head;
    vertex_cache_block *unused;
    uint32_t total_count;

} vertex_cache;

static void vertex_cache_clear(vertex_cache *cache)
{
    cache->total_count = 0;
    for (size_t i = 0; i < MAGMA_VERTEX_CACHE_COUNT-1; i++)
    {
        cache->blocks[i].next = &cache->blocks[i+1];
    }
    cache->blocks[MAGMA_VERTEX_CACHE_COUNT-1].next = NULL;
    cache->unused = &cache->blocks[0];
    cache->head = NULL;
}

static vertex_cache_block* vertex_cache_insert_after(vertex_cache *cache, vertex_cache_block *block)
{
    // Make sure there are still unused blocks remaining!
    assertf(cache->unused, "Maximum number of blocks reached! This is a bug within magma.");

    // Remove an entry from the list of unused blocks and use it as the new one
    vertex_cache_block *new_block = cache->unused;
    cache->unused = new_block->next;

    if (block) {
        // If block is set, insert after it
        new_block->next = block->next;
        block->next = new_block;
    } else {
        // Otherwise insert at the start of the list
        new_block->next = cache->head;
        cache->head = new_block;
    }

    return new_block;
}

static void vertex_cache_merge_with_next(vertex_cache *cache, vertex_cache_block *block)
{
    vertex_cache_block *next = block->next;
    if (!next) {
        // Nothing to do if there is no next block
        return;
    }

    // Check if the two blocks are exactly bordering each other.
    // This is suffient since blocks can normally only grow by one at a time.
    uint16_t block_end = block->start + block->count;

    if (block_end == next->start) {
        // Grow this block to encompass the next one
        block->count += next->count;

        // Remove the next block from the list
        block->next = next->next;

        // Return it to the list of unused blocks
        next->next = cache->unused;
        cache->unused = next;
    } else {
        // Check invariant
        assertf(block_end < next->start, "Blocks are overlapping! This is a bug within magma.");
    }
}

static bool vertex_cache_try_insert(vertex_cache *cache, uint16_t index)
{
    if (cache->total_count >= MAGMA_VERTEX_CACHE_COUNT) {
        return false;
    }

    // Find existing block to insert into
    vertex_cache_block *block = cache->head;
    vertex_cache_block *prev = NULL;
    while (block)
    {
        if (block->count > 0) {
            uint16_t block_end = block->start + block->count;
            if (index >= block->start && index < block_end) {
                // Index is already contained in this block
                return true;
            }

            if (index == block_end) {
                // Index is the next one after this block. Grow it by one.
                block->count++;
                cache->total_count++;

                // Try to merge with the next block in case they are now bordering
                vertex_cache_merge_with_next(cache, block);

                return true;
            }

            if (block->start - index == 1) {
                // Index is the previous one before this block. Grow it backwards by one.
                block->count++;
                block->start--;
                cache->total_count++;

                // Try to merge with the previous block in case they are now bordering
                if (prev) vertex_cache_merge_with_next(cache, prev);

                return true;
            }

            if (block->start > index) {
                // Blocks are always sorted. That means if the current block's start
                // is after the index, we already know that none of the following blocks
                // will contain the index. The new block we need to create has to be inserted
                // before the current block.
                break;
            }
        }

        prev = block;
        block = block->next;
    }

    // If no existing block contained the index, insert new block after "prev". 
    // If prev is null, there are no blocks yet and this will create the new block at the start of the list.
    vertex_cache_block *new_block = vertex_cache_insert_after(cache, prev);
    new_block->start = index;
    new_block->count = 1;
    cache->total_count++;
    return true;
}

static bool vertex_cache_find(const vertex_cache *cache, uint16_t index, uint8_t *cache_index)
{
    uint16_t cur_offset = 0;
    vertex_cache_block *block = cache->head;
    while (block)
    {
        uint16_t diff = index - block->start;
        if (diff < block->count) {
            *cache_index = cur_offset + diff;
            return true;
        }
        cur_offset += block->count;
        block = block->next;
    }

    return false;
}

static uint8_t vertex_cache_get(const vertex_cache *cache, uint16_t index)
{
    uint8_t cache_index;
    bool found = vertex_cache_find(cache, index, &cache_index);
    assertf(found, "Index %d not found in vertex batch! This is a bug within magma.", index);
    return cache_index;
}

static void vertex_cache_load(const vertex_cache *cache, int32_t offset, uint32_t cache_offset)
{
    vertex_cache_block *block = cache->head;
    while (block)
    {
        if (block->count > 0) {
            mg_load_vertices(block->start + offset, cache_offset, block->count);
            cache_offset += block->count;
        }

        block = block->next;
    }
}

static void vertex_cache_dump(const vertex_cache *cache)
{
    debugf("vertex cache dump:\n");
    vertex_cache_block *block = cache->head;
    while (block)
    {
        debugf("%d %d\n", block->start, block->count);
        block = block->next;
    }
}

static uint32_t prepare_batch(const uint16_t *indices, int32_t vertex_offset, uint32_t max_count, vertex_cache *cache, uint32_t windup, uint32_t advance, bool restart_enabled, uint32_t cache_offset)
{
    vertex_cache_clear(cache);

    uint32_t count = 0;
    uint32_t required = windup + advance;

    while (count + required <= max_count)
    {
        bool restart = false;
        uint32_t need_insertion = 0;
        for (size_t i = 0; i < required; i++)
        {
            uint16_t index = indices[count + i];
            if (restart_enabled && index == SPECIAL_INDEX) {
                required = windup + advance;
                need_insertion = 0;
                count++;
                restart = true;
                break;
            }

            uint8_t dummy;
            need_insertion += vertex_cache_find(cache, index, &dummy) ? 0 : 1;
        }

        if (restart) {
            continue;
        }

        if (cache->total_count + need_insertion + cache_offset > MAGMA_VERTEX_CACHE_COUNT) {
            break;
        }

        for (size_t i = 0; i < required; i++)
        {
            vertex_cache_try_insert(cache, indices[count + i]);
        }

        count += required;
        required = advance;

        //vertex_cache_dump(cache);
    }

    assertf(cache->total_count + cache_offset <= MAGMA_VERTEX_CACHE_COUNT, "Vertex batch is too big! This is a bug within magma.");

    vertex_cache_load(cache, vertex_offset, cache_offset);
    return count;
}

static void draw_triangle_list_batch(const uint16_t *indices, uint32_t current_index, uint32_t batch_count, const vertex_cache *cache)
{
    uint8_t prim_indices[3];
    for (size_t i = 0; i < batch_count; i++)
    {
        uint8_t cache_index = vertex_cache_get(cache, indices[current_index + i]);

        prim_indices[i%3] = cache_index;
        if (i%3 == 2) {
            mg_draw_indices(prim_indices[0], prim_indices[1], prim_indices[2]);
        }
    }
}

static void draw_triangle_strip_batch(const uint16_t *indices, uint32_t current_index, uint32_t batch_count, const vertex_cache *cache, bool restart_enabled)
{
    size_t prim_counter = 0;
    uint8_t prim_indices[3];
    for (size_t i = 0; i < batch_count; i++)
    {
        uint16_t index = indices[current_index + i];

        if (restart_enabled && index == SPECIAL_INDEX) {
            prim_counter = 0;
            continue;
        }

        uint8_t cache_index = vertex_cache_get(cache, index);

        prim_indices[prim_counter%3] = cache_index;
        if (prim_counter>1) {
            int p = prim_counter-2;
            int p0 = p;
            int p1 = p+(1+p%2);
            int p2 = p+(2-p%2);
            mg_draw_indices(prim_indices[p0%3], prim_indices[p1%3], prim_indices[p2%3]);
        }

        prim_counter++;
    }
}

static void draw_triangle_fan_batch(const uint16_t *indices, uint32_t current_index, uint32_t batch_count, const vertex_cache *cache, bool restart_enabled, uint32_t cache_offset)
{
    uint8_t prim_indices[3] = {0};
    size_t prim_counter = cache_offset;
    for (size_t i = 0; i < batch_count; i++)
    {
        uint16_t index = indices[current_index + i];

        if (restart_enabled && index == SPECIAL_INDEX) {
            prim_counter = 0;
            continue;
        }

        uint8_t cache_index = vertex_cache_get(cache, index) + cache_offset;

        if (prim_counter == 0) {
            prim_indices[2] = cache_index;
        } else {
            prim_indices[prim_counter%2] = cache_index;
        }

        if (prim_counter>1) {
            int p = prim_counter-2;
            int p0 = p+1;
            int p1 = p+2;
            mg_draw_indices(prim_indices[p0%2], prim_indices[p1%2], prim_indices[2]);
        }

        prim_counter++;
    }
}

static uint32_t get_advance_count(mg_primitive_topology_t topology)
{
    switch (topology) {
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        return 3;
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        return 1;
    default:
        assertf(0, "Unknown topology");
    }
}

static uint32_t get_windup_count(mg_primitive_topology_t topology)
{
    switch (topology) {
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        return 0;
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
    case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        return 2;
    default:
        assertf(0, "Unknown topology");
    }
}

void mg_draw_indexed(const mg_input_assembly_parms_t *input_assembly_parms, const uint16_t *indices, uint32_t index_count, int32_t vertex_offset)
{
    vertex_cache cache;
    uint32_t current_index = 0;

    mg_primitive_topology_t topology = input_assembly_parms ? input_assembly_parms->primitive_topology : MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    bool restart_enabled = input_assembly_parms != NULL && input_assembly_parms->primitive_restart_enabled;

    assertf(!(restart_enabled && topology == MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST), "Primitive restart is not supported for triangle lists!");

    uint32_t windup = get_windup_count(topology);
    uint32_t advance = get_advance_count(topology);

    uint32_t cache_offset = 0;

    while (current_index < index_count - windup + cache_offset)
    {
        uint32_t batch_index_count = prepare_batch(indices + current_index, vertex_offset, index_count - current_index, &cache, windup, advance, restart_enabled, cache_offset);

        switch (topology) {
        case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
            draw_triangle_list_batch(indices, current_index, batch_index_count, &cache);
            break;
        case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
            draw_triangle_strip_batch(indices, current_index, batch_index_count, &cache, restart_enabled);
            break;
        case MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
            draw_triangle_fan_batch(indices, current_index, batch_index_count, &cache, restart_enabled, cache_offset);
            cache_offset = 1;
            break;
        }

        current_index += batch_index_count - windup + cache_offset;
    }
}
