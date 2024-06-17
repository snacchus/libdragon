#include <libdragon.h>
#include <rspq_profile.h>

#include "matrix.h"
#include "quat.h"
#include "utility.h"
#include "cube_mesh.h"
#include "scene_data.h"

#define FB_COUNT    3

#define ENABLE_RDPQ_DEBUG 0
#define SINGLE_FRAME      0

#define VTX_TEX_SHIFT   8
#define RDP_TEX_SHIFT   5
#define TEX_SIZE_SHIFT  (VTX_TEX_SHIFT-RDP_TEX_SHIFT)
#define RDP_HALF_TEXEL  (1<<(RDP_TEX_SHIFT-1))

typedef struct
{
    mgfx_fog_t fog;
    mgfx_lighting_t lighting;
} scene_raw_data;

typedef struct
{
    mg_resource_set_t *resource_set;
    sprite_t *texture;
    rdpq_texparms_t rdpq_tex_parms;
    mgfx_modes_t modes;
    mg_geometry_flags_t geometry_flags;
    color_t color;
} material_data;

typedef struct
{
    mg_buffer_t *vertex_buffer;
    const uint16_t *indices;
    uint32_t index_count;
    rspq_block_t *block;
} mesh_data;

typedef struct
{
    mat4x4_t mvp_matrix;
    mat4x4_t mv_matrix;
    mat4x4_t n_matrix;
    quat_t rotation;
    float position[3];
    float rotation_axis[3];
    float rotation_angle;
    float rotation_rate;
    uint32_t material_id;
    uint32_t mesh_id;
} object_data;

void init();
void update();
void render();
void create_scene_resources();
void material_create(material_data *mat, sprite_t *texture, mgfx_modes_parms_t *mode_parms, mg_geometry_flags_t geometry_flags, color_t color);
void mesh_create(mesh_data *mesh, const mgfx_vertex_t *vertices, uint32_t vertex_count, const uint16_t *indices, uint32_t index_count);
void update_object_matrices(object_data *object);

static surface_t zbuffer;

static mg_viewport_t viewport;
static mg_culling_parms_t culling;
static mg_pipeline_t *pipeline;
static mg_buffer_t *scene_resource_buffer;
static mg_resource_set_t *scene_resource_set;

static sprite_t *textures[TEXTURE_COUNT];
static material_data materials[MATERIAL_COUNT];
static mesh_data meshes[MESH_COUNT];
static object_data objects[OBJECT_COUNT];
static mgfx_light_parms_t lights[LIGHT_COUNT];

static mat4x4_t projection_matrix;
static mat4x4_t view_matrix;
static mat4x4_t vp_matrix;
static float camera_position[3];
static quat_t camera_rotation;

static uint32_t last_frame_ticks;

static uint64_t frames = 0;

int main()
{
    init();

#if ENABLE_RDPQ_DEBUG
    rdpq_debug_start();
    rdpq_debug_log(true);
#endif

    rspq_profile_start();

    while (true) 
    {
        update();
        render();

#if SINGLE_FRAME
        rspq_wait();
        break;
#endif
    }
}

void init()
{
    const resolution_t resolution = RESOLUTION_320x240;

    // Initialize the required subsystems
	debug_init(DEBUG_FEATURE_LOG_ISVIEWER | DEBUG_FEATURE_LOG_USB);
    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(resolution, DEPTH_16_BPP, FB_COUNT, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);
    rdpq_init();
    mg_init();

    // Create depth buffer
    zbuffer = surface_alloc(FMT_RGBA16, resolution.width, resolution.height);

    // Initialize viewport
    viewport = (mg_viewport_t) {
        .x = 0,
        .y = 0,
        .width = resolution.width,
        .height = resolution.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    
    // Initialize culling mode
    culling = (mg_culling_parms_t) {
        .cull_mode = MG_CULL_MODE_BACK
    };

    // This function returns the builtin pipeline provided by libdragon.
    // When done rendering with it, the pipeline needs to be freed using mg_pipeline_free.
    pipeline = mgfx_create_pipeline();

    create_scene_resources();

    // Initialize lighting parameters
    for (size_t i = 0; i < LIGHT_COUNT; i++)
    {
        lights[i].color = color_from_packed32(light_colors[i]);
        lights[i].radius = light_radii[i];
    }

    // Load textures
    for (size_t i = 0; i < TEXTURE_COUNT; i++)
    {
        textures[i] = sprite_load(texture_files[i]);
    }

    // Create materials
    for (size_t i = 0; i < MATERIAL_COUNT; i++)
    {
        material_create(
            &materials[i], 
            textures[material_texture_indices[i]], 
            &(mgfx_modes_parms_t) {
                .flags = MGFX_MODES_FLAGS_FOG_ENABLED
            },
            MG_GEOMETRY_FLAGS_Z_ENABLED | MG_GEOMETRY_FLAGS_TEX_ENABLED | MG_GEOMETRY_FLAGS_SHADE_ENABLED,
            color_from_packed32(material_diffuse_colors[i]));
    }

    // Create meshes
    for (size_t i = 0; i < MESH_COUNT; i++)
    {
        mesh_create(&meshes[i], mesh_vertices[i], mesh_vertex_counts[i], mesh_indices[i], mesh_index_counts[i]);
    }

    // Initialize objects
    for (size_t i = 0; i < OBJECT_COUNT; i++)
    {
        objects[i] = (object_data) {
            .material_id = object_material_ids[i],
            .mesh_id = object_mesh_ids[i],
        };
        memcpy(objects[i].position, object_positions[i], sizeof(objects[i].position));
        quat_make_identity(&objects[i].rotation);

        // Create a random rotation axis (just an approximation, not actually uniformly distributed)
        objects[i].rotation_axis[0] = RAND_FLT() * 2.0f - 1.0f;
        objects[i].rotation_axis[1] = RAND_FLT() * 2.0f - 1.0f;
        objects[i].rotation_axis[2] = RAND_FLT() * 2.0f - 1.0f;
        vec3_normalize(objects[i].rotation_axis, objects[i].rotation_axis);

        objects[i].rotation_rate = RAND_FLT() * 5.0f;
        objects[i].rotation_angle = RAND_FLT() * M_TWOPI;
    }

    // Initialize camera properties
    float aspect_ratio = (float)resolution.width / (float)resolution.height;
    mat4x4_make_projection(&projection_matrix, camera_fov, aspect_ratio, camera_near_plane, camera_far_plane);
    memcpy(camera_position, camera_starting_position, sizeof(camera_position));
    quat_make_identity(&camera_rotation);

    last_frame_ticks = TICKS_READ();
}

void create_scene_resources()
{
    // These are resources that are expected to stay constant during the entire scene.
    // These will be provided to the shader by writing the data to a uniform buffer, and attaching that buffer
    // to a resource set. By using a resource set, uploading the data to DMEM during rendering will be 
    // automatically optimized for us. Using uniform buffers also allows us to modify the buffer contents dynamically,
    // for example to update lighting.

    // Create the uniform buffer. It's important that the buffer contents need to be in a format that is optimized for
    // access by the RSP. This is what the scene_raw_data struct is, which itself contains the predefined structs
    // provided by the fixed function pipeline.
    scene_resource_buffer = mg_buffer_create(&(mg_buffer_parms_t) {
        .size = sizeof(scene_raw_data),
        .flags = MG_BUFFER_FLAGS_USAGE_UNIFORM
    });

    // Create the resource set. A resource set contains a number of resource bindings.
    // Each resource binding describes the type of resource, where to copy it from, and which binding location to copy it to.
    // Binding locations are defined by the vertex shader. Therefore we use predefined binding locations that are provided by
    // the fixed function pipeline.
    mg_resource_binding_t scene_bindings[] = {
        {
            .binding = MGFX_BINDING_FOG,
            .type = MG_RESOURCE_TYPE_UNIFORM_BUFFER,
            .buffer = scene_resource_buffer,
            .offset = offsetof(scene_raw_data, fog)
        },
        {
            .binding = MGFX_BINDING_LIGHTING,
            .type = MG_RESOURCE_TYPE_UNIFORM_BUFFER,
            .buffer = scene_resource_buffer,
            .offset = offsetof(scene_raw_data, lighting)
        },
    };

    // By bundling multiple resource bindings in a resource set, magma can automatically optimize the operation
    // (for example by detecting that some bindings can be coalesced into a contiguous DMA). During rendering, 
    // the set can be "bound" with a single function call, which will load all attached resources into DMEM at once.
    scene_resource_set = mg_resource_set_create(&(mg_resource_set_parms_t) {
        .pipeline = pipeline,
        .binding_count = ARRAY_COUNT(scene_bindings),
        .bindings = scene_bindings
    });

    // Note that even though we've created the resource set above by pointing it towards a buffer, we haven't actually initialised
    // any of the contents yet. This will be done at beginning of each frame.
}

void material_create(material_data *material, sprite_t *texture, mgfx_modes_parms_t *mode_parms, mg_geometry_flags_t geometry_flags, color_t color)
{
    // Similarly to the scene resources, we will provide materials to the shader via resource sets.
    // We separate the material from scene resources, because they are expected to change during the scene.
    // Not all objects will have the same material after all. To show off all features of magma in this demo,
    // we will make the assumption that the materials themselves will stay constant. Therefore we won't store
    // this data inside buffers, but attach it to the resource set directly via so called "embedded uniforms".

    // 1. Initialize the raw material data by using the functions provided by the fixed function pipeline.
    //    Just as with uniform buffers, the shader expects the data in a format that is optimized for the RSP.
    mgfx_texturing_t tex;
    mgfx_modes_t modes;
    mgfx_get_texturing(&tex, &(mgfx_texturing_parms_t) {
        .scale[0] = texture->width  >> TEX_SIZE_SHIFT,
        .scale[1] = texture->height >> TEX_SIZE_SHIFT,
        .offset[0] = -RDP_HALF_TEXEL,
        .offset[1] = -RDP_HALF_TEXEL
    });
    mgfx_get_modes(&modes, mode_parms);

    // 2. Create the resource set. This time, we use the resource type "embedded uniform" and set
    //    the "embedded_data" pointer to pass in the data we initialized above.
    mg_resource_binding_t bindings[] = {
        {
            .binding = MGFX_BINDING_TEXTURING,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &tex
        },
        {
            .binding = MGFX_BINDING_MODES,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &modes
        },
    };

    // When this call returns, the "embedded_data" has been consumed and a copy embedded inside the resource set itself.
    // This effectively does the same as a uniform buffer, but there is one less object to manage. The drawback is that 
    // once the resource set has been created, there is no way to modify the embedded data. But due to the assumption we made,
    // that won't be required anyway.
    material->resource_set = mg_resource_set_create(&(mg_resource_set_parms_t) {
        .pipeline = pipeline,
        .binding_count = ARRAY_COUNT(bindings),
        .bindings = bindings,
    });

    // Additionally prepare texture data for rdpq, but this is completely unrelated to magma.
    material->texture = texture;
    material->rdpq_tex_parms = (rdpq_texparms_t) {
        .s.repeats = REPEAT_INFINITE,
        .t.repeats = REPEAT_INFINITE,
    };

    material->geometry_flags = geometry_flags;
    material->color = color;
}

void mesh_create(mesh_data *mesh, const mgfx_vertex_t *vertices, uint32_t vertex_count, const uint16_t *indices, uint32_t index_count)
{
    // Preparing mesh data is relatively straightforward. Simply load vertex and index data into buffers.
    // By setting "initial_data", the buffer will already contain this data after creation, so we don't 
    // need to map it and manually copy the data in.

    mesh->vertex_buffer = mg_buffer_create(&(mg_buffer_parms_t) {
        .size = sizeof(mgfx_vertex_t) * vertex_count,
        .initial_data = vertices,
        .flags = MG_BUFFER_FLAGS_USAGE_VERTEX
    });

    mesh->indices = indices;
    mesh->index_count = index_count;

    // To increase performance, we can record the drawing command into a block, since the topology of the mesh doesn't change in this case.
    // Note that we could still modify the vertices themselves if we wanted, by writing to the vertex buffer. This would require some manual
    // synchronisation, however.
    rspq_block_begin();
        // This function internally just reads the list of indices and emits a sequence of calls to mg_load_vertices and mg_draw_indices.
        // Those function can also be called manually, if more customisation of the mesh layout is desired.
        mg_draw_indexed(&(mg_input_assembly_parms_t) {
            .primitive_topology = MG_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        }, mesh->indices, mesh->index_count, 0);
    mesh->block = rspq_block_end();
}

void update()
{
    // Very basic delta time setup. It's enough for this demo.
    const uint32_t new_ticks = TICKS_READ();
    const uint32_t delta_ticks = TICKS_DISTANCE(last_frame_ticks, new_ticks);
    const float delta_seconds = (float)delta_ticks / TICKS_PER_SECOND;
    last_frame_ticks = new_ticks;

    for (size_t i = 0; i < OBJECT_COUNT; i++)
    {
        quat_from_axis_angle(&objects[i].rotation, objects[i].rotation_axis, objects[i].rotation_angle);
        objects[i].rotation_angle = wrap_angle(objects[i].rotation_angle + objects[i].rotation_rate * delta_seconds);
    }
}

void update_camera()
{
    // Update camera matrices.
    const float up[3] = {0, 1, 0};
    const float target[3] = {0, 0, 0};
    mat4x4_make_lookat(&view_matrix, camera_position, up, target);
    mat4x4_mult(&vp_matrix, &projection_matrix, &view_matrix);
}

void update_lights()
{
    // Here we are updating the contents of the scene resources that we created during initialisation above.

    // Because lighting is computed in eye space and we specify light positions/directions in world space,
    // we need to manually transform the lights into eye space each frame and update the corresponding uniform.
    for (size_t i = 0; i < LIGHT_COUNT; i++)
    {
        // Transform light position into eye space
        mat4x4_mult_vec(lights[i].position, &view_matrix, light_positions[i]);
    }

    // TODO: synchronize
    // Map the buffer for writing access and write the uniform data into it. It's important to always unmap the buffer once done.
    scene_raw_data *raw_data = mg_buffer_map(scene_resource_buffer, 0, sizeof(raw_data), MG_BUFFER_MAP_FLAGS_WRITE);
        // These mgfx_get_* functions will take the parameters in a convenient format and convert them into the RSP-optimized format that the buffer is supposed to contain.
        mgfx_get_fog(&raw_data->fog, &(mgfx_fog_parms_t) {
            .start = fog_start,
            .end = fog_end
        });
        mgfx_get_lighting(&raw_data->lighting, &(mgfx_lighting_parms_t) {
            .ambient_color = color_from_packed32(ambient_light_color),
            .light_count = ARRAY_COUNT(lights),
            .lights = lights
        });
    mg_buffer_unmap(scene_resource_buffer);
}

void update_object_matrices(object_data *object)
{
    // Update object matrices from its current transform.

    // TODO: Do (parts of) this on RSP instead
    mat4x4_t model_matrix;
    mat4x4_make_rotation_translation(&model_matrix, object->position, object->rotation.v);
    mat4x4_mult(&object->mvp_matrix, &vp_matrix, &model_matrix);
    mat4x4_mult(&object->mv_matrix, &view_matrix, &model_matrix);
    mat4x4_to_normal_matrix(&object->n_matrix, &object->mv_matrix);
}

void render()
{
    // Update frame specific data
    update_camera();
    update_lights();

    // Get framebuffer
    surface_t *disp = display_get();
    rdpq_debug_log_msg("---> Frame");
    rdpq_attach_clear(disp, &zbuffer);

    // Set up render modes with rdpq. This could be set per material, but for simplicity's sake we use the same render mode for all objects in this demo.
    rdpq_mode_begin();
        rdpq_set_mode_standard();
        rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
        rdpq_mode_zbuf(true, true);
        rdpq_mode_antialias(AA_STANDARD);
        rdpq_mode_persp(true);
        rdpq_mode_filter(FILTER_BILINEAR);
        rdpq_mode_combiner(RDPQ_COMBINER2((TEX0,0,SHADE,0), (TEX0,0,SHADE,0), (COMBINED,0,PRIM,0), (COMBINED,0,PRIM,0)));
        rdpq_mode_fog(RDPQ_BLENDER((FOG_RGB, SHADE_ALPHA, IN_RGB, INV_MUX_ALPHA)));
    rdpq_mode_end();

    rdpq_set_fog_color(color_from_packed32(fog_color));

    // Set viewport, culling mode and geometry flags
    mg_set_viewport(&viewport);
    mg_set_culling(&culling);

    // All our materials use the same pipeline in this demo, so bind it once for the entire scene
    mg_bind_pipeline(pipeline);

    // Bind resources that stay constant for the entire scene (for example lighting)
    mg_bind_resource_set(scene_resource_set);

    uint32_t current_material_id = -1;
    uint32_t current_mesh_id = -1;

    material_data *current_material = NULL;
    mesh_data *current_mesh = NULL;
    object_data *object = NULL;

    // Iterate over all objects
    for (size_t i = 0; i < OBJECT_COUNT; i++)
    {
        rdpq_debug_log_msg("-----> Object");
        // Recalculate object matrices.
        object = &objects[i];
        update_object_matrices(object);

        // Swap out the current material resources. This will automatically upload all uniform data to DMEM that is bound to the set.
        // To avoid redundant uploads, we keep track of the current material and only make this call when it actually changes.
        // This will be most optimal if the list of objects has been sorted by material.
        if (object->material_id != current_material_id) {
            rdpq_debug_log_msg("-------> Material");
            current_material_id = object->material_id;
            current_material = &materials[current_material_id];
            mg_bind_resource_set(current_material->resource_set);

            // Also set the geometry flags, which determine the type of triangle to be drawn.
            mg_set_geometry_flags(current_material->geometry_flags);

            // Additionally, upload the material's texture and change the material color via rdpq, which can be done completely independently from magma.
            rdpq_set_prim_color(current_material->color);
            if (current_material->texture) {
                rdpq_sprite_upload(TILE0, current_material->texture, &current_material->rdpq_tex_parms);
            }
            rdpq_debug_log_msg("<------- Material");
        }

        // Swap out the currently bound vertex buffer. Similar to materials, we only do this when it changes.
        // Since swapping buffers is very quick, objects should be sorted by material first, and then by mesh.
        if (object->mesh_id != current_mesh_id) {
            current_mesh_id = object->mesh_id;
            current_mesh = &meshes[current_mesh_id];
            mg_bind_vertex_buffer(current_mesh->vertex_buffer, 0);
        }

        // Because the matrices are expected to change every frame and for every object, we upload them "inline", which embeds their data within the command stream itself.
        // After the call returns, the matrix data has been consumed entirely and we won't need to worry about keeping it in memory.
        // If we were to use resource sets for matrices as well, we would have to manually synchronize updating them on the CPU.
        // TODO: an example how to do manual synchronization
        // This function uses "mg_inline_uniform" internally with a predefined offset and size, and automatically converts the data to a RSP-native format.
        mgfx_set_matrices_inline(&(mgfx_matrices_parms_t) {
            .model_view_projection = object->mvp_matrix.m[0],
            .model_view = object->mv_matrix.m[0],
            .normal = object->mv_matrix.m[0],
        });

        assert(current_mesh != NULL);

        // Perform the draw call. This will assemble the triangles from the currently bound vertex/index buffers, process them with the
        // currently bound pipeline (using the attached vertex shader), applying all currently bound resources such as matrices, lighting and material parameters etc.
        rdpq_debug_log_msg("-------> Draw");

        // Even when drawing commands are recorded into a block, we need to put them into a drawing batch to ensure proper synchronisation with rdpq.
        mg_draw_begin();
            rspq_block_run(current_mesh->block);
        mg_draw_end();

        rdpq_debug_log_msg("<------- Draw");

        rdpq_debug_log_msg("<----- Object");
    }

    // Done. Detach from the framebuffer and present it.
    rdpq_detach_show();

    rdpq_debug_log_msg("<--- Frame");

    rspq_profile_next_frame();

    if (((frames++) % 60) == 0) {
        rspq_profile_dump();
        rspq_profile_reset();
    }
}
