#include <rsp_magma.inc>
#include <magma_fixed_function_constants.h>

MgBeginShaderConstants
    MgBeginUniform FOG, MGFX_BINDING_FOG
        FOG_FACTOR_INT:     .half   0
        FOG_OFFSET_INT:     .half   0
        FOG_FACTOR_FRAC:    .half   0
        FOG_OFFSET_FRAC:    .half   0
    MgEndUniform

    MgBeginUniform LIGHTING, MGFX_BINDING_LIGHTING
        LIGHTING_LIGHTS:    .dcb.b  MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX
        LIGHTING_AMBIENT:   .dcb.w  4
        LIGHTING_COUNT:     .word   0
    MgEndUniform

    MgBeginUniform TEXTURING, MGFX_BINDING_TEXTURING
        TEXTURING_SCALE:    .dcb.w  2
        TEXTURING_OFFSET:   .dcb.w  2
    MgEndUniform

    MgBeginUniform MODES, MGFX_BINDING_MODES
        MODES_FLAGS:        .word   0
    MgEndUniform

    MgBeginUniform MATRICES, MGFX_BINDING_MATRICES
        MATRICES_MVP:       .dcb.b  MGFX_MATRIX_SIZE
        MATRICES_MV:        .dcb.b  MGFX_MATRIX_SIZE
        MATRICES_NORMAL:    .dcb.b  MGFX_MATRIX_SIZE
    MgEndUniform

    MgBeginUniform MATRIX_PALETTE, MGFX_BINDING_MATRIX_PALETTE
        MATRIX_PALETTE_PTR: .word 0
    MgEndUniform
MgEndShaderConstants

MgBeginShader
    #define flags           v0
    #define vtx_in_ptr      s1
    #define vtx_in_end      s2
    #define vtx_out0_ptr    s3
    #define vtx_out1_ptr    s5
    #define v___            $v29
    #define vconst          $v28

    #define vmvp0_i         $v01
    #define vmvp0_f         $v02
    #define vmvp1_i         $v03
    #define vmvp1_f         $v04
    #define vmvp2_i         $v05
    #define vmvp2_f         $v06
    #define vmvp3_i         $v07
    #define vmvp3_f         $v08
    #define vviewscale      $v09
    #define vviewoff        $v10
    #define vclip_factors   $v11
    #define vtexscale       $v12
    #define vtexoffset      $v13
    #define vnormmask       $v14
    #define vnormfactor     $v15

    #define vpos_clip_i     $v22
    #define vpos_clip_f     $v23
    #define vpos_scr_i      $v24
    #define vpos_scr_f      $v25

    lw flags, %lo(MODES_FLAGS)
    lw s0, %lo(MAGMA_VERTEX_BUFFER)
    li t5, %lo(MAGMA_CONSTANTS)
    addi t1, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER)
    lqv vconst, 0,t5
    llv $v01.e0, -4,t1

    sll t0, a0, MGFX_VTX_SIZE_SHIFT
    add s0, t0

    # TODO: this is just a workaround for not being able to define .data segments in shaders
    li t2, MAGMA_VTX_SIZE - MGFX_VTX_SIZE
    mtc2 t2, vconst.e1

    # cache_offset * MAGMA_VTX_SIZE, vertex_count * (MAGMA_VTX_SIZE - MGFX_VTX_SIZE)
    vmudh $v02, $v01, vconst
    # cache_offset * MAGMA_VTX_SIZE + vertex_count * (MAGMA_VTX_SIZE - MGFX_VTX_SIZE)
    vaddc $v03, $v02, $v02.e1

    # vertex_count * MGFX_VTX_SIZE - 1
    andi t1, a1, 0xFFFF
    sll t1, MGFX_VTX_SIZE_SHIFT
    addi t0, t1, -1

    mfc2 s4, $v03.e0
    mfc2 vtx_out0_ptr, $v02.e0

    jal DMAInAsync
    addi s4, %lo(MAGMA_VERTEX_CACHE)

    andi vtx_in_ptr, s4, 0xFFF
    add vtx_in_end, vtx_in_ptr, t1
    addi vtx_out0_ptr, %lo(MAGMA_VERTEX_CACHE)

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

    # load texture transform
    li t1, %lo(TEXTURING)
    llv vtexscale.e0,  0x0,t1
    llv vtexscale.e4,  0x0,t1
    llv vtexoffset.e0, 0x4,t1
    llv vtexoffset.e4, 0x4,t1

    # load viewport
    li t1, %lo(MAGMA_VIEWPORT)
    ldv vviewscale.e0, 0x0,t1
    ldv vviewscale.e4, 0x0,t1
    ldv vviewoff.e0, 0x8,t1
    ldv vviewoff.e4, 0x8,t1

    # load clip factors
    ldv vclip_factors.e0, 0x10,t1
    ldv vclip_factors.e4, 0x10,t1

    ldv vnormmask.e0,   0x10,t5
    ldv vnormmask.e4,   0x10,t5
    ldv vnormfactor.e0, 0x18,t5
    ldv vnormfactor.e4, 0x18,t5

    jal DMAWaitIdle
    nop

    # TODO: prevent second vertex from being stored on the last iteration if vertex count is odd
vertex_loop:
    bge vtx_in_ptr, vtx_in_end, RSPQ_Loop
    addi vtx_out1_ptr, vtx_out0_ptr, MAGMA_VTX_SIZE

    #define vpos_in $v26
    #define vnorm   $v27

    # load vertex position (of two vertices)
    ldv vpos_in.e0, MGFX_VTX_POS + 0,            vtx_in_ptr
    ldv vpos_in.e4, MGFX_VTX_POS + MGFX_VTX_SIZE,vtx_in_ptr

    # Unpack normal
    vand v___, vnormmask, vpos_in.h3
    vmudn vnorm, v___, vnormfactor

    # Set W components to 1
    vmov vpos_in.e3, K32
    vmov vpos_in.e7, K32

    # transform vertex position into clip space
    vmudn v___,        vmvp0_f, vpos_in.h0
    vmadh v___,        vmvp0_i, vpos_in.h0
    vmadn v___,        vmvp1_f, vpos_in.h1
    vmadh v___,        vmvp1_i, vpos_in.h1
    vmadn v___,        vmvp2_f, vpos_in.h2
    vmadh v___,        vmvp2_i, vpos_in.h2
    vmadn vpos_clip_f, vmvp3_f, vpos_in.h3
    vmadh vpos_clip_i, vmvp3_i, vpos_in.h3

    #undef vpos_in

    #define vmn0_i $v16
    #define vmn0_f $v17
    #define vmn1_i $v18
    #define vmn1_f $v19
    #define vmn2_i $v20
    #define vmn2_f $v21

    # transform normal
    li t1, %lo(MATRICES_NORMAL)
    ldv vmn0_i.e0, 0x00,t1
    ldv vmn0_i.e4, 0x00,t1
    ldv vmn1_i.e0, 0x08,t1
    ldv vmn1_i.e4, 0x08,t1
    ldv vmn2_i.e0, 0x10,t1
    ldv vmn2_i.e4, 0x10,t1
    ldv vmn0_f.e0, 0x20,t1
    ldv vmn0_f.e4, 0x20,t1
    ldv vmn1_f.e0, 0x28,t1
    ldv vmn1_f.e4, 0x28,t1
    ldv vmn2_f.e0, 0x30,t1
    ldv vmn2_f.e4, 0x30,t1

    vmudn v___,        vmn0_f, vnorm.h0
    vmadh v___,        vmn0_i, vnorm.h0
    vmadn v___,        vmn1_f, vnorm.h1
    vmadh v___,        vmn1_i, vnorm.h1
    vmadn v___,        vmn2_f, vnorm.h2
    vmadh vnorm,       vmn2_i, vnorm.h2

    #undef vmn0_i
    #undef vmn0_f
    #undef vmn1_i
    #undef vmn1_f
    #undef vmn2_i
    #undef vmn2_f

    #define vsqdist_i   $v16
    #define vsqdist_f   $v17
    #define vinvdist_i  $v18
    #define vinvdist_f  $v19

    # re-normalize normal
    vmudh v___, vnorm, vnorm
    vsar  vsqdist_f, COP2_ACC_MD
    vsar  vsqdist_i, COP2_ACC_HI
    vaddc vinvdist_f, vsqdist_f, vsqdist_f.h1
    vadd  vinvdist_i, vsqdist_i, vsqdist_i.h1
    vaddc vsqdist_f, vinvdist_f, vsqdist_f.h2
    vadd  vsqdist_i, vinvdist_i, vsqdist_i.h2

    vrsqh v___.e0,       vsqdist_i.e0
    vrsql vinvdist_f.e0, vsqdist_f.e0
    vrsqh vinvdist_i.e0, vsqdist_i.e4
    vrsql vinvdist_f.e4, vsqdist_f.e4
    vrsqh vinvdist_i.e4, vzero.e0

    vmudm v___,  vnorm, vinvdist_f.h0
    vmadh vnorm, vnorm, vinvdist_i.h0

    #undef vsqdist_i
    #undef vsqdist_f
    #undef vinvdist_i
    #undef vinvdist_f

    #define vlight $v26

    # compute lighting
    li s0, %lo(LIGHTING_LIGHTS)
    ldv vlight.e0, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
    ldv vlight.e4, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient

    lw t6, %lo(LIGHTING_COUNT)
light_loop:
    #define vlpos   $v16
    #define vlcol   $v17
    #define vndl    $v18
    ldv vlpos.e0, MGFX_LIGHT_POSITION,s0
    ldv vlpos.e4, MGFX_LIGHT_POSITION,s0
    ldv vlcol.e0, MGFX_LIGHT_COLOR,s0
    ldv vlcol.e4, MGFX_LIGHT_COLOR,s0

    # TODO: point lights

    vmulf vndl, vnorm, vlpos,
    vaddc v___, vndl, vndl.h1
    vaddc vndl, v___, vndl.h2
    vge   vndl, vzero

    vmulf vlcol, vndl.h0
    vaddc vlight, vlcol

    addi t6, -1
    bnez t6, light_loop
    addi s0, MGFX_LIGHT_SIZE
    #undef vlpos
    #undef vlcol
    #undef vndl

    #undef vnorm

    #define vrgba $v27

    # load rgba (of two vertices)
    # shuffle values around so they can be loaded in a single luv op
    llv vrgba.e0, MGFX_VTX_RGBA + 0,             vtx_in_ptr
    llv vrgba.e2, MGFX_VTX_RGBA + MGFX_VTX_SIZE, vtx_in_ptr
    sdv vrgba.e0, 0,vtx_out0_ptr
    luv vrgba.e0, 0,vtx_out0_ptr

    # multiply vertex color by accumulated light color
    vmulf vrgba, vlight

    #undef vlight

    # TODO: Fog

    # store rgba to vertex cache
    # CAUTION: this also overwrites the next 4 bytes after the rgba value
    suv vrgba.e0, MAGMA_VTX_RGBA,vtx_out0_ptr
    suv vrgba.e4, MAGMA_VTX_RGBA,vtx_out1_ptr

    #undef vrgba

    #define vtex $v26

    # load texcoords (of two vertices)
    llv vtex.e0, MGFX_VTX_TEX + 0,            vtx_in_ptr
    llv vtex.e4, MGFX_VTX_TEX + MGFX_VTX_SIZE,vtx_in_ptr

    # transform texcoords
    vmudh v___, vtexoffset, K1
    vmadh vtex, vtex, vtexscale

    # store texcoords to vertex cache
    slv vtex.e0, MAGMA_VTX_ST,vtx_out0_ptr
    slv vtex.e4, MAGMA_VTX_ST,vtx_out1_ptr

    #undef vtex

    # 32-bit right shift by 5, to keep the clip space coordinates unscaled
    vmudm vpos_clip_i, vpos_clip_i, vshift8.e4
    vmadl vpos_clip_f, vpos_clip_f, vshift8.e4

    # store clip space position to vertex cache
    sdv vpos_clip_i.e0,  MAGMA_VTX_CS_POSi,vtx_out0_ptr
    sdv vpos_clip_f.e0,  MAGMA_VTX_CS_POSf,vtx_out0_ptr
    sdv vpos_clip_i.e4,  MAGMA_VTX_CS_POSi,vtx_out1_ptr
    sdv vpos_clip_f.e4,  MAGMA_VTX_CS_POSf,vtx_out1_ptr

    # store W to vertex cache
    ssv vpos_clip_i.e3, MAGMA_VTX_Wi,vtx_out0_ptr
    ssv vpos_clip_f.e3, MAGMA_VTX_Wf,vtx_out0_ptr
    ssv vpos_clip_i.e7, MAGMA_VTX_Wi,vtx_out1_ptr
    ssv vpos_clip_f.e7, MAGMA_VTX_Wf,vtx_out1_ptr

    #define vinvw_i $v26
    #define vinvw_f $v27

    # perspective division
    vrcph v___.e3,    vpos_clip_i.e3
    vrcpl vinvw_f.e3, vpos_clip_f.e3
    vrcph vinvw_i.e3, vpos_clip_i.e7
    vrcpl vinvw_f.e7, vpos_clip_f.e7
    vrcph vinvw_i.e7, vzero.e0

    # store inverse W to vertex cache
    ssv vinvw_i.e3, MAGMA_VTX_INVWi,vtx_out0_ptr
    ssv vinvw_f.e3, MAGMA_VTX_INVWf,vtx_out0_ptr
    ssv vinvw_i.e7, MAGMA_VTX_INVWi,vtx_out1_ptr
    ssv vinvw_f.e7, MAGMA_VTX_INVWf,vtx_out1_ptr

    vmudl v___,       vpos_clip_f, vinvw_f.h3
    vmadm v___,       vpos_clip_i, vinvw_f.h3
    vmadn vpos_scr_f, vpos_clip_f, vinvw_i.h3
    vmadh vpos_scr_i, vpos_clip_i, vinvw_i.h3

    #undef vinvw_i
    #undef vinvw_f

    # transform to screen space
    vmudh v___, vviewoff, K1
    vmadn vpos_scr_f, vpos_scr_f, vviewscale
    vmadh vpos_scr_i, vpos_scr_i, vviewscale

    # store screen space position to vertex cache
    sdv vpos_scr_i.e0,  MAGMA_VTX_XYZ,vtx_out0_ptr
    sdv vpos_scr_i.e4,  MAGMA_VTX_XYZ,vtx_out1_ptr

    #define vguard_i        $v26
    #define vguard_f        $v27

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

    #undef vguard_i
    #undef vguard_f

    # store clip codes to vertex cache
    sb t0, MAGMA_VTX_CLIP_CODE(vtx_out0_ptr)
    sb t2, MAGMA_VTX_CLIP_CODE(vtx_out1_ptr)

    # calculate inverted trivial rejection codes
    vch v___, vpos_clip_i, vpos_clip_i.h3
    vcl v___, vpos_clip_f, vpos_clip_f.h3
    cfc2 t0, COP2_CTRL_VCC
    srl t2, t0, 4
    andi t0, 0x707
    andi t2, 0x707
    srl t1, t0, 5
    srl t3, t2, 5
    or t0, t1
    or t2, t3

    # store trivial rejection codes to vertex cache
    sb t0, MAGMA_VTX_TR_CODE(vtx_out0_ptr)
    sb t2, MAGMA_VTX_TR_CODE(vtx_out1_ptr)
    
    addi vtx_out0_ptr, MAGMA_VTX_SIZE*2
    b vertex_loop
    addi vtx_in_ptr, MGFX_VTX_SIZE*2

    #undef v___
    #undef vconst
MgEndShader
