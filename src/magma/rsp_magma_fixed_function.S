#include <rsp_magma.inc>
#include <magma_fixed_function_constants.h>

MgBeginShaderConstants
    MgBeginUniform MODES, 0
        MODES_FLAGS:        .word   0
    MgEndUniform

    MgBeginUniform FOG, 1
        FOG_FACTOR_INT:     .half   0
        FOG_OFFSET_INT:     .half   0
        FOG_FACTOR_FRAC:    .half   0
        FOG_OFFSET_FRAC:    .half   0
    MgEndUniform

    MgBeginUniform LIGHTING, 2
        LIGHTING_POSITION:  .dcb.w  MGFX_LIGHT_COUNT_MAX * 4
        LIGHTING_DIFFUSE:   .dcb.w  MGFX_LIGHT_COUNT_MAX * 4
        LIGHTING_ATT_INT:   .dcb.w  MGFX_LIGHT_COUNT_MAX * 4
        LIGHTING_ATT_FRAC:  .dcb.w  MGFX_LIGHT_COUNT_MAX * 4
        LIGHTING_AMBIENT:   .dcb.w  4
        LIGHTING_COUNT:     .word   0
    MgEndUniform

    MgBeginUniform MATERIAL, 3
        MATERIAL_DIFFUSE:   .dcb.w  4
        MATERIAL_EMISSIVE:  .dcb.w  4
        MATERIAL_COL_TGT:   .word   0
    MgEndUniform

    MgBeginUniform TEXTURING, 4
        TEXTURING_SCALE:    .dcb.w  2
        TEXTURING_OFFSET:   .dcb.w  2
    MgEndUniform

    MgBeginUniform MATRICES, 5
        MATRICES_MVP:       .dcb.b  MGFX_MATRIX_SIZE
        MATRICES_MV:        .dcb.b  MGFX_MATRIX_SIZE
        MATRICES_NORMAL:    .dcb.b  MGFX_MATRIX_SIZE
    MgEndUniform

    MgBeginUniform MATRIX_PALETTE, 6
        MATRIX_PALETTE_PTR: .word 0
    MgEndUniform
MgEndShaderConstants

    .data
MGFX_CONSTANTS:     .half   MAGMA_VTX_SIZE, MAGMA_VTX_SIZE - MGFX_VTX_SIZE, 1<<MGFX_VTX_POS_SHIFT, 0, 0, 0, 0, 0

    #############################################################
    # ARGS:
    #   a0: Bit 31..24: Command id
    #       Bit 23..0:  Vertex buffer offset
    #   a1: Bit 31..16: Cache offset
    #       Bit 15..0:  Vertex count
    #############################################################
MgBeginShader
    #define vtx_out_ptr     s1
    #define vtx_in_ptr      s2
    #define vtx_in_end      s3
    #define v___            $v29
    #define vconst          $v28
    #define vpos_in         $v01
    #define vpos_clip_i     $v02
    #define vpos_clip_f     $v03
    #define vinvw_i         $v04
    #define vinvw_f         $v05
    #define vpos_scr_i      $v06
    #define vpos_scr_f      $v07
    #define vmvp0_i         $v08
    #define vmvp0_f         $v09
    #define vmvp1_i         $v10
    #define vmvp1_f         $v11
    #define vmvp2_i         $v12
    #define vmvp2_f         $v13
    #define vmvp3_i         $v14
    #define vmvp3_f         $v15
    #define vviewscale      $v16
    #define vviewoff        $v17
    #define vclip_factors   $v18
    #define vguard_i        $v19
    #define vguard_f        $v20
    #define vtex            $v21
    #define vrgba           $v22
    #define default_w       vconst.e2

    lw s0, %lo(MAGMA_VERTEX_BUFFER)
    li t0, %lo(MGFX_CONSTANTS)
    addi t1, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER)
    lqv vconst, 0,t0
    llv $v01.e0, -4,t1

    sll t0, a0, MGFX_VTX_SIZE_SHIFT
    add s0, t0

    # cache_offset * MAGMA_VTX_SIZE, vertex_count * (MAGMA_VTX_SIZE - MGFX_VTX_SIZE)
    vmudh $v02, $v01, vconst
    # cache_offset * MAGMA_VTX_SIZE + vertex_count * (MAGMA_VTX_SIZE - MGFX_VTX_SIZE)
    vaddc $v03, $v02, $v02.e1

    # vertex_count * MGFX_VTX_SIZE - 1
    sll t1, a1, MGFX_VTX_SIZE_SHIFT
    addi t0, t1, -1

    mfc2 s4, $v03.e0
    mfc2 vtx_out_ptr, $v02.e0

    jal DMAInAsync
    addi s4, %lo(MAGMA_VERTEX_CACHE)

    move vtx_in_ptr, s4
    add vtx_in_end, s4, t1
    addi vtx_out_ptr, %lo(MAGMA_VERTEX_CACHE)

    # load mvp matrix
    li t1, %lo(MATRICES)
    ldv vmvp0_i.e0,  0x00,t1
    ldv vmvp0_i.e4,  0x00,t1
    ldv vmvp1_i.e0,  0x08,t1
    ldv vmvp1_i.e4,  0x08,t1
    ldv vmvp2_i.e0,  0x10,t1
    ldv vmvp2_i.e4,  0x10,t1
    ldv vmvp3_i.e0,  0x18,t1
    ldv vmvp3_i.e4,  0x18,t1
    ldv vmvp0_f.e0,  0x20,t1
    ldv vmvp0_f.e4,  0x20,t1
    ldv vmvp1_f.e0,  0x28,t1
    ldv vmvp1_f.e4,  0x28,t1
    ldv vmvp2_f.e0,  0x30,t1
    ldv vmvp2_f.e4,  0x30,t1
    ldv vmvp3_f.e0,  0x38,t1
    ldv vmvp3_f.e4,  0x38,t1

    # load viewport
    li t1, %lo(MAGMA_VIEWPORT)
    ldv vviewscale.e0, 0x0,t1
    ldv vviewscale.e4, 0x0,t1
    ldv vviewoff.e0, 0x8,t1
    ldv vviewoff.e4, 0x8,t1

    ldv vclip_factors.e0, 0x10,t1
    ldv vclip_factors.e4, 0x10,t1

    jal DMAWaitIdle
    nop

    # TODO: prevent second vertex from being stored on the last iteration if vertex count is odd
vertex_loop:
    beq vtx_in_ptr, vtx_in_end, RSPQ_Loop
    nop

    # load vertex position (of two vertices)
    # X0 Y0 Z0 1  X1 Y1 Z1 1
    vcopy vpos_in, default_w
    ldv vpos_in.e0, MGFX_VTX_POS + 0,            vtx_in_ptr
    ldv vpos_in.e4, MGFX_VTX_POS + MGFX_VTX_SIZE,vtx_in_ptr

    # load rgba (of two vertices)
    # shuffle values around so they can be loaded in a single luv op
    llv vrgba.e0, MGFX_VTX_RGBA + 0,             vtx_in_ptr
    llv vrgba.e2, MGFX_VTX_RGBA + MGFX_VTX_SIZE, vtx_in_ptr
    sdv vrgba.e0, 0,vtx_out_ptr
    luv vrgba.e0, 0,vtx_out_ptr

    # load texcoords (of two vertices)
    llv vtex.e0, MGFX_VTX_TEX + 0,            vtx_in_ptr
    llv vtex.e4, MGFX_VTX_TEX + MGFX_VTX_SIZE,vtx_in_ptr

    # TODO: load normal

    # TODO: transform texcoords

    # TODO: lighting

    # store texcoords to vertex cache
    slv vtex.e0, MAGMA_VTX_ST + 0,            vtx_out_ptr
    slv vtex.e4, MAGMA_VTX_ST + MGFX_VTX_SIZE,vtx_out_ptr

    # transform vertex position into clip space
    vmudn v___,        vmvp0_f, vpos_in
    vmadh v___,        vmvp0_i, vpos_in
    vmadn v___,        vmvp1_f, vpos_in
    vmadh v___,        vmvp1_i, vpos_in
    vmadn v___,        vmvp2_f, vpos_in
    vmadh v___,        vmvp2_i, vpos_in
    vmadn vpos_clip_f, vmvp3_f, vpos_in
    vmadh vpos_clip_i, vmvp3_i, vpos_in

    # 32-bit right shift by 5, to keep the clip space coordinates unscaled
    vmudm vpos_clip_i, vpos_clip_i, vshift8.e4
    vmadl vpos_clip_f, vpos_clip_f, vshift8.e4

    # store clip space position to vertex cache
    sdv vpos_clip_i.e0,  MAGMA_VTX_CS_POSi + 0,             vtx_out_ptr
    sdv vpos_clip_f.e0,  MAGMA_VTX_CS_POSf + 0,             vtx_out_ptr
    sdv vpos_clip_i.e4,  MAGMA_VTX_CS_POSi + MAGMA_VTX_SIZE,vtx_out_ptr
    sdv vpos_clip_f.e4,  MAGMA_VTX_CS_POSf + MAGMA_VTX_SIZE,vtx_out_ptr

    # store rgba to vertex cache
    # CAUTION: this also overwrites the next 4 bytes after the rgba value
    suv vrgba.e0, MAGMA_VTX_RGBA + 0,             vtx_out_ptr
    suv vrgba.e4, MAGMA_VTX_RGBA + MAGMA_VTX_SIZE,vtx_out_ptr

    # store W to vertex cache
    ssv vpos_clip_i.e3, MAGMA_VTX_Wi + 0,              vtx_out_ptr
    ssv vpos_clip_f.e3, MAGMA_VTX_Wf + 0,              vtx_out_ptr
    ssv vpos_clip_i.e7, MAGMA_VTX_Wi + MAGMA_VTX_SIZE, vtx_out_ptr
    ssv vpos_clip_f.e7, MAGMA_VTX_Wf + MAGMA_VTX_SIZE, vtx_out_ptr

    # perspective division
    vrcph v___.e3,    vpos_clip_i.e3
    vrcpl vinvw_f.e3, vpos_clip_f.e3
    vrcph vinvw_i.e3, vpos_clip_i.e7
    vrcpl vinvw_f.e7, vpos_clip_f.e7
    vrcpl vinvw_i.e7, vzero.e0

    # store inverse W to vertex cache
    ssv vinvw_i.e3, MAGMA_VTX_INVWi + 0,             vtx_out_ptr
    ssv vinvw_f.e3, MAGMA_VTX_INVWf + 0,             vtx_out_ptr
    ssv vinvw_i.e7, MAGMA_VTX_INVWi + MAGMA_VTX_SIZE,vtx_out_ptr
    ssv vinvw_f.e7, MAGMA_VTX_INVWf + MAGMA_VTX_SIZE,vtx_out_ptr

    vmudl v___,       vpos_clip_f, vinvw_f.h3
    vmadm v___,       vpos_clip_i, vinvw_f.h3
    vmadn vpos_scr_f, vpos_clip_f, vinvw_i.h3
    vmadh vpos_scr_i, vpos_clip_i, vinvw_i.h3

    # transform to screen space
    vmudh v___, vviewoff, K1
    vmadn vpos_scr_f, vpos_scr_f, vviewscale
    vmadh vpos_scr_i, vpos_scr_i, vviewscale

    # store screen space position to vertex cache
    sdv vpos_scr_i.e0,  MAGMA_VTX_XYZ + 0,             vtx_out_ptr
    sdv vpos_scr_i.e4,  MAGMA_VTX_XYZ + MAGMA_VTX_SIZE,vtx_out_ptr

    # calculate clipping codes
    vmudn vguard_f, vpos_clip_f, vclip_factors
    vmadh vguard_i, vpos_clip_i, vclip_factors
    vch v___, vguard_i, vguard_i.h3
    vcl v___, vguard_f, vguard_f.h3
    cfc2 t0, COP2_CTRL_VCC
    srl t2, t0, 4
    andi t0, 0x707
    andi t2, 0x707
    srl t1, t0, 5
    srl t3, t2, 5
    or t0, t1
    or t2, t3

    # store clip codes to vertex cache
    sb t0, MAGMA_VTX_CLIP_CODE + 0             (vtx_out_ptr)
    sb t2, MAGMA_VTX_CLIP_CODE + MAGMA_VTX_SIZE(vtx_out_ptr)

    # calculate inverted trivial rejection codes
    vch v___, vpos_clip_i, vpos_clip_i.h3
    vcl v___, vpos_clip_f, vpos_clip_f.h3
    cfc2 t0, COP2_CTRL_VCC
    srl t2, t0, 4
    andi t0, 0x707
    andi t2, 0x707
    srl t1, t0, 5
    srl t3, t2, 5
    nor t0, t1
    nor t2, t3

    # store trivial rejection codes to vertex cache
    sb t0, MAGMA_VTX_TR_CODE + 0             (vtx_out_ptr)
    sb t2, MAGMA_VTX_TR_CODE + MAGMA_VTX_SIZE(vtx_out_ptr)
    
    addi vtx_out_ptr, MAGMA_VTX_SIZE*2
    b vertex_loop
    addi vtx_in_ptr, MGFX_VTX_SIZE*2

    #undef v___
    #undef vconst
MgEndShader
