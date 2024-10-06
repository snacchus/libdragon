#ifndef __LIBDRAGON_MODEL64_H
#define __LIBDRAGON_MODEL64_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODEL64_MAX_ATTR_COUNT 5

typedef enum {
    MODEL64_VTX_FMT_GL   = 0,
    MODEL64_VTX_FMT_MGFX = 1
} model64_vtx_fmt_t;

typedef enum {
    MODEL64_ATTR_POSITION   = 0,    ///< Vertex position.
    MODEL64_ATTR_COLOR      = 1,    ///< Vertex color.
    MODEL64_ATTR_TEXCOORD   = 2,    ///< Texture coordinate.
    MODEL64_ATTR_NORMAL     = 3,    ///< Vertex normal.
    MODEL64_ATTR_MTX_INDEX  = 4     ///< Matrix/Bone index.
} model64_attr_t;

typedef enum {
    MODEL64_ATTR_TYPE_I8,                   ///< Signed 8-bit integer.
    MODEL64_ATTR_TYPE_U8,                   ///< Unsigned 8-bit integer.
    MODEL64_ATTR_TYPE_I16,                  ///< Signed 16-bit integer.
    MODEL64_ATTR_TYPE_U16,                  ///< Unsigned 16-bit integer.
    MODEL64_ATTR_TYPE_FX16,                 ///< Signed 16-bit fixed point.
    MODEL64_ATTR_TYPE_I32,                  ///< Signed 32-bit integer.
    MODEL64_ATTR_TYPE_U32,                  ///< Unsigned 32-bit integer.
    MODEL64_ATTR_TYPE_F32,                  ///< Single-precision IEEE 754 floating point.
    MODEL64_ATTR_TYPE_PACKED_NORMAL_16      ///< 5-6-5 Packed normal vector.
} model64_attr_type_t;

typedef enum {
    MODEL64_ANIM_SLOT_0 = 0,
    MODEL64_ANIM_SLOT_1 = 1,
    MODEL64_ANIM_SLOT_2 = 2,
    MODEL64_ANIM_SLOT_3 = 3
} model64_anim_slot_t;

struct model64_s;
typedef struct model64_s model64_t;

struct mesh_s;
typedef struct mesh_s mesh_t;

struct primitive_s;
typedef struct primitive_s primitive_t;

struct model64_node_s;
typedef struct model64_node_s model64_node_t;

/** @brief Description of a vertex attribute. */
typedef struct {
    model64_attr_t attribute;   ///< Specifies which attribute is being described.
    model64_attr_type_t type;   ///< The type of the attribute's values.
    uint32_t component_count;   ///< The number of component values the attribute consists of.
    uint32_t offset;            ///< The offset in bytes relative to the start of a vertex.
} model64_vertex_attr_t;

/** @brief Description of a primitive's vertex layout. */
typedef struct {
    model64_vertex_attr_t attributes[MODEL64_MAX_ATTR_COUNT];   ///< The list of vertex attributes.
    uint32_t attribute_count;                                   ///< The number of vertex attributes.
    uint32_t stride;                                            ///< The distance in bytes between two consecutive vertices.
} model64_vertex_layout_t;

model64_t *model64_load(const char *fn);
model64_t *model64_load_buf(void *buf, int sz);
void model64_free(model64_t *model);
model64_t *model64_clone(model64_t *model);

/**
 * @brief Return the vertex format of this model.
 */
model64_vtx_fmt_t model64_get_vertex_format(model64_t *model);

/**
 * @brief Return the number of meshes in this model.
 */
uint32_t model64_get_mesh_count(model64_t *model);

/**
 * @brief Return the mesh at the specified index.
 */
mesh_t *model64_get_mesh(model64_t *model, uint32_t mesh_index);


/**
 * @brief Return the number of nodes in this model.
 */
uint32_t model64_get_node_count(model64_t *model);

/**
 * @brief Return the node at the specified index.
 */
model64_node_t *model64_get_node(model64_t *model, uint32_t node_index);

/**
 * @brief Return the first node with the specified name in the model.
 */
model64_node_t *model64_search_node(model64_t *model, const char *name);

/**
 * @brief Sets the position of a node in a model relative to its parent.
 */
void model64_set_node_pos(model64_t *model, model64_node_t *node, float x, float y, float z);

/**
 * @brief Sets the rotation of a node in a model relative to its parent in the form of an euler angle (ZYX rotation order) in radians.
 */
void model64_set_node_rot(model64_t *model, model64_node_t *node, float x, float y, float z);

/**
 * @brief Sets the rotation of a node in a model relative to its parent in the form of a quaternion.
 */
void model64_set_node_rot_quat(model64_t *model, model64_node_t *node, float x, float y, float z, float w);

/**
 * @brief Sets the scale of a node in a model relative to its parent.
 */
void model64_set_node_scale(model64_t *model, model64_node_t *node, float x, float y, float z);

/**
 * @brief Gets the transformation matrix between a model's root node and a node in a model.
 */
void model64_get_node_world_mtx(model64_t *model, model64_node_t *node, float dst[16]);

/**
 * @brief Return the number of primitives in this mesh.
 */
uint32_t model64_get_primitive_count(mesh_t *mesh);

/**
 * @brief Return the primitive at the specified index.
 */
primitive_t *model64_get_primitive(mesh_t *mesh, uint32_t primitive_index);

/**
 * @brief Return a pointer to the first vertex in this primitive.
 */
void *model64_get_primitive_vertices(primitive_t *primitive);

/**
 * @brief Return the number of vertices in this primitive.
 */
uint32_t model64_get_primitive_vertex_count(primitive_t *primitive);

/**
 * @brief Return a pointer to the first index in this primitive.
 */
void *model64_get_primitive_indices(primitive_t *primitive);

/**
 * @brief Return the number of indices in this primitive.
 */
uint32_t model64_get_primitive_index_count(primitive_t *primitive);

/**
 * @brief Query the vertex layout of the primitive.
 * 
 * @param[in]   primitive   The primitive to be queried.
 * @param[out]  layout      Will contain a description of the vertex layout.
 */
void model64_get_primitive_vertex_layout(primitive_t* primitive, model64_vertex_layout_t *layout);

/**
 * @brief Draw an entire model.
 * 
 * This will draw all nodes that are contained in the given model while applying the relevant node matrices.
 */
void model64_draw(model64_t *model);

/**
 * @brief Draw a single mesh.
 * 
 * This will draw all of the given mesh's primitives.
 */
void model64_draw_mesh(mesh_t *mesh);

/**
 * @brief Draw a single node.
 * 
 * This will draw a single mesh node.
 */
void model64_draw_node(model64_t *model, model64_node_t *node);

/**
 * @brief Draw a single primitive.
 */
void model64_draw_primitive(primitive_t *primitive);

void model64_anim_play(model64_t *model, const char *anim, model64_anim_slot_t slot, bool paused, float start_time);
void model64_anim_stop(model64_t *model, model64_anim_slot_t slot);
float model64_anim_get_length(model64_t *model, const char *anim);
float model64_anim_get_time(model64_t *model, model64_anim_slot_t slot);
float model64_anim_set_time(model64_t *model, model64_anim_slot_t slot, float time);
float model64_anim_set_speed(model64_t *model, model64_anim_slot_t slot, float speed);
bool model64_anim_set_loop(model64_t *model, model64_anim_slot_t slot, bool loop);
bool model64_anim_set_pause(model64_t *model, model64_anim_slot_t slot, bool paused);
void model64_update(model64_t *model, float deltatime);
#ifdef __cplusplus
}
#endif

#endif
