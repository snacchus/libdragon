#ifndef __LIBDRAGON_MAGMA_H
#define __LIBDRAGON_MAGMA_H

#include <stdbool.h>
#include <stdint.h>
#include <graphics.h>
#include <rsp.h>

/** @brief A precompiled vertex loader that will load vertices in a certain format. */
typedef struct mg_vertex_loader_s    mg_vertex_loader_t;

// TODO: Should "shader" and "pipeline" be separate objects?
/** @brief A piece of ucode that is compatible with the magma pipeline */
typedef struct mg_shader_s          mg_shader_t;

/** @brief An instance of the magma pipeline, with an attached vertex shader */
typedef struct mg_pipeline_s        mg_pipeline_t;

// TODO: Is the separation of "memory" and "buffer" useful in our case?
/** @brief Represents a contiguous block of memory, located in RDRAM */
typedef struct mg_memory_s          mg_memory_t;

/** @brief A linear array of data, which can be bound to a pipeline for various purposes */
typedef struct mg_buffer_s          mg_buffer_t;

// TODO: The term "descriptor set" is taken from Vulkan. Come up with a better name?
/** @brief A set of resources, that can be bound for use by a shader */
typedef struct mg_descriptor_set_s  mg_descriptor_set_t;

// TODO: figure out better naming scheme
// TODO: complete?
typedef enum
{
    MG_VERTEX_FORMAT_SCAL_8                 = 0,
    MG_VERTEX_FORMAT_VEC2_8                 = 1,
    MG_VERTEX_FORMAT_VEC3_8                 = 2,
    MG_VERTEX_FORMAT_VEC4_8                 = 3,
    MG_VERTEX_FORMAT_SCAL_16                = 4,
    MG_VERTEX_FORMAT_VEC2_16                = 5,
    MG_VERTEX_FORMAT_VEC3_16                = 6,
    MG_VERTEX_FORMAT_VEC4_16                = 7,
    MG_VERTEX_FORMAT_SCAL_32                = 8,
    MG_VERTEX_FORMAT_VEC2_32                = 9,
    MG_VERTEX_FORMAT_VEC3_32                = 10,
    MG_VERTEX_FORMAT_VEC4_32                = 11,
} mg_vertex_format_t;

typedef enum
{
    MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST     = 0,
    MG_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP    = 1,
    MG_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN      = 2,
} mg_primitive_topology_t;

typedef enum
{
    MG_CULL_FLAGS_NONE                      = 0,
    MG_CULL_FLAGS_BACK                      = 0x1,
    MG_CULL_FLAGS_FRONT                     = 0x2,
    MG_CULL_FLAGS_FRONT_AND_BACK            = MG_CULL_FLAGS_BACK | MG_CULL_FLAGS_FRONT,
} mg_cull_flags_t;

typedef enum
{
    MG_FRONT_FACE_COUNTER_CLOCKWISE         = 0,
    MG_FRONT_FACE_CLOCKWISE                 = 1,
} mg_front_face_t;

// TODO: "Host visible"? "Host coherent"?
typedef enum
{
    MG_MEMORY_FLAGS_MIRRORED                = 0x1,
    MG_MEMORY_FLAGS_WRITABLE                = 0x2,
    MG_MEMORY_FLAGS_COPY_ON_WRITE           = 0x4,
    MG_MEMORY_FLAGS_HOST_READABLE           = 0x8,
} mg_memory_flags_t;

typedef enum
{
    MG_MEMORY_MAP_FLAGS_READ                = 0x1,
    MG_MEMORY_MAP_FLAGS_WRITE               = 0x2,
} mg_memory_map_flags_t;

typedef enum
{
    MG_DESCRIPTOR_TYPE_UNIFORM_BUFFER       = 0,
    MG_DESCRIPTOR_TYPE_STORAGE_BUFFER       = 1,
    MG_DESCRIPTOR_TYPE_INLINE_UNIFORM       = 2,
} mg_descriptor_type_t;

typedef struct
{
    mg_cull_flags_t cull_flags;
    mg_front_face_t front_face;
} mg_culling_parms_t;

typedef struct
{
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
} mg_viewport_t;

typedef struct
{
    mg_shader_t *vertex_shader;
    mg_culling_parms_t culling;
    mg_viewport_t viewport;
} mg_graphics_pipeline_parms_t;

typedef struct
{
    uint32_t location;
    mg_vertex_format_t format;
    uint32_t offset;
} mg_vertex_attribute_descriptor_t;

typedef struct
{
    mg_vertex_attribute_descriptor_t *attribute_descriptors;
    uint32_t attribute_descriptor_count;
    uint32_t stride;
} mg_vertex_loader_parms_t;

typedef struct
{
    mg_primitive_topology_t primitive_topology;
    bool primitive_restart_enabled;
} mg_input_assembly_parms_t;

typedef struct
{
    mg_memory_flags_t flags;
    void *pointer;
    uint32_t size;
} mg_memory_parms_t;

typedef struct
{
} mg_buffer_parms_t;

typedef struct
{
    uint32_t binding;
    mg_descriptor_type_t type;
    mg_buffer_t *buffer;
    const void *inline_data;
    uint32_t offset;
} mg_descriptor_binding_t;

typedef struct
{
    mg_shader_t *shader;
    uint32_t binding_count;
    mg_descriptor_binding_t *bindings;
} mg_descriptor_set_parms_t;

#ifdef __cplusplus
extern "C" {
#endif

void mg_init(void);
void mg_close(void);

// NOTE: The following functions are not commands, so they are not automatically synchronized with RSP!

/* Shaders */

mg_shader_t *mg_shader_create(rsp_ucode_t *ucode);
void mg_shader_free(mg_shader_t *vertex_shader);

/* Vertex input */

mg_vertex_loader_t *mg_vertex_loader_create(mg_vertex_loader_parms_t *parms);
void mg_vertex_loader_free(mg_vertex_loader_t *vertex_loader);

/* Pipelines */

mg_pipeline_t *mg_pipeline_create_graphics(mg_graphics_pipeline_parms_t *parms);
void mg_pipeline_free(mg_pipeline_t *pipeline);

/* Memory */

mg_memory_t *mg_memory_create(mg_memory_parms_t *parms);
void mg_memory_free(mg_memory_t *memory);
void *mg_memory_map(mg_memory_t *memory, uint32_t offset, uint32_t size, mg_memory_map_flags_t flags);
void mg_memory_unmap(mg_memory_t *memory);
void mg_memory_write(mg_memory_t *memory, uint32_t offset, uint32_t size, const void *data);

/* Buffers */

mg_buffer_t *mg_buffer_create(mg_buffer_parms_t *parms);
void mg_buffer_free(mg_buffer_t *buffer);

/** @brief Swaps out the currently bound block of memory with a new one */
void mg_buffer_bind_memory(mg_buffer_t *buffer, mg_memory_t *memory, uint32_t size, uint32_t offset);

/* Descriptors */

mg_descriptor_set_t *mg_descriptor_set_create(mg_descriptor_set_parms_t *parms);
void mg_descriptor_set_free(mg_descriptor_set_t *descriptor_set);


/* Commands (these will generate rspq commands) */

/** @brief Bind the pipeline for subsequent use, uploading the attached shader to IMEM */
void mg_bind_pipeline(mg_pipeline_t *pipeline);

/** @brief Set culling flags */
void mg_set_culling(mg_culling_parms_t *culling);

/** @brief Set the viewport */
void mg_set_viewport(mg_viewport_t *mg_viewport_t);

/** @brief Bind a descriptor set, uploading the bound resources to DMEM */
void mg_bind_descriptor_set(mg_descriptor_set_t *descriptor_set);

/** @brief Push a block of memory directly to DMEM, embedding the data in the command */
void mg_push_constants(uint32_t offset, uint32_t size, const void *data);

/** @brief Bind a vertex buffer to be used by subsequent drawing commands */
void mg_bind_vertex_buffer(mg_buffer_t *buffer, uint32_t offset);

/** @brief Bind a vertex loader to be used by subsequent drawing commands */
void mg_bind_vertex_loader(mg_vertex_loader_t *vertex_loader);

/** @brief Draw primitives */
void mg_draw(mg_input_assembly_parms_t *input_assembly_parms, uint32_t vertex_count, uint32_t first_vertex);

/** @brief Draw indexed primitives */
void mg_draw_indexed(mg_input_assembly_parms_t *input_assembly_parms, uint32_t index_count, uint16_t *indices, int32_t vertex_offset);

// TODO: Instanced draw calls?
// TODO: Indirect draw calls?

#ifdef __cplusplus
}
#endif

#endif
