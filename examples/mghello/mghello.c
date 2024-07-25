#include <libdragon.h>

typedef struct {
    int16_t pos[3];
} vertex;

int main()
{
    const resolution_t resolution = RESOLUTION_320x240;

	debug_init(DEBUG_FEATURE_LOG_ISVIEWER | DEBUG_FEATURE_LOG_USB);
    display_init(resolution, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);
    rdpq_init();
    mg_init();

    // Get the builtin rendering pipeline
    mg_pipeline_t *pipeline = mgfx_create_pipeline(&(mgfx_pipeline_parms_t) {
        .vtx_layout = 0
    });

    // Shader uniforms are not initialized to 0's automatically, so we need to create
    // a resource set that sets everything to sane values.
    mgfx_fog_t fog = {0};
    mgfx_lighting_t lighting = {0};
    mgfx_texturing_t texturing = {0};
    mgfx_modes_t modes = {0};
    mgfx_matrices_t matrices = {0};

    // Lighting is never explicitly turned off, but it can be set to "pass through"
    // by configuring 0 lights and fully white ambient light.
    mgfx_get_lighting(&lighting, &(mgfx_lighting_parms_t) {
        .light_count = 0,
        .ambient_color = color_from_packed32(0xFFFFFFFF)
    });

    // Initialize all matrices to identity
    const float identity[] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    mgfx_get_matrices(&matrices, &(mgfx_matrices_parms_t) {
        .model_view_projection = identity,
        .model_view = identity,
        .normal = identity,
    });

    // Create the resource set. Since we're never changing any of the uniforms,
    // just embed everything into the resource set itself by using the binding type
    // "MG_RESOURCE_TYPE_EMBEDDED_UNIFORM". This will internally allocate some memory,
    // and copy the data from "embedded_data" when mg_resource_set_create is called. 
    // We can safely throw away the input afterwards.
    mg_resource_binding_t resource_bindings[] = {
        {
            .binding = MGFX_BINDING_FOG,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &fog
        },
        {
            .binding = MGFX_BINDING_LIGHTING,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &lighting
        },
        {
            .binding = MGFX_BINDING_TEXTURING,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &texturing,
        },
        {
            .binding = MGFX_BINDING_MODES,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &modes,
        },
        {
            .binding = MGFX_BINDING_MATRICES,
            .type = MG_RESOURCE_TYPE_EMBEDDED_UNIFORM,
            .embedded_data = &matrices,
        }
    };
    mg_resource_set_t *resource_set = mg_resource_set_create(&(mg_resource_set_parms_t) {
        .pipeline = pipeline,
        .binding_count = sizeof(resource_bindings)/sizeof(resource_bindings[0]),
        .bindings = resource_bindings
    });

    // Create and fill a vertex buffer.
    vertex vertices[] = {
        { .pos = MGFX_POS(0, -0.5f, 0) },
        { .pos = MGFX_POS(-0.5f, 0.5f, 0) },
        { .pos = MGFX_POS(0.5f, 0.5f, 0) }
    };
    mg_buffer_t *vertex_buffer = mg_buffer_create(&(mg_buffer_parms_t) {
        .size = sizeof(vertices),
    });
    mg_buffer_write(vertex_buffer, 0, sizeof(vertices), vertices);

    // Everything we need is initialised. Start the main rendering loop!
    while (true)
    {
        // This is just the regular display + rdpq setup.
        surface_t *disp = display_get();

        rdpq_attach_clear(disp, NULL);

        rdpq_mode_begin();
            rdpq_set_mode_standard();
            rdpq_mode_dithering(DITHER_SQUARE_SQUARE);
            rdpq_mode_antialias(AA_STANDARD);
            rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_mode_end();

        rdpq_set_prim_color(color_from_packed32(0xFFFFFFFF));

        // Set the vertex pipeline
        mg_bind_pipeline(pipeline);

        // Set the viewport to the full screen
        mg_set_viewport(&(mg_viewport_t) {
            .x = 0,
            .y = 0,
            .width = resolution.width,
            .height = resolution.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        });

        // Set the culling mode.
        mg_set_culling(&(mg_culling_parms_t) {
            .cull_mode = MG_CULL_MODE_NONE
        });

        // Configure the type of triangles that should be emitted to the RDP.
        // We just want vertex colors, so use SHADE.
        mg_set_geometry_flags(MG_GEOMETRY_FLAGS_SHADE_ENABLED);

        // Apply the resource set that was created above. This must be done
        // every frame to guarantee that the uniforms have the desired values.
        mg_bind_resource_set(resource_set);

        // Bind the vertex buffer that was created above. All subsequent drawing
        // commands will now read from this buffer.
        mg_bind_vertex_buffer(vertex_buffer, 0);

        // All drawing commands (including mg_draw and mg_draw_indexed) must be put 
        // between mg_draw_begin and mg_draw_end, which delimit a drawing "batch". 
        // This is to guarantee proper synchronisation with render modes. 
        // Render modes must not be changed during a drawing batch.
        mg_draw_begin();
            // Load all vertices from the buffer into the internal cache
            mg_load_vertices(0, 0, 3);
            // Draw a triangle using those vertices
            mg_draw_indices(0, 1, 2);
        mg_draw_end();

        // End the frame as usual.
        rdpq_detach_show();
    }
}
