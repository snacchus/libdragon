#include <rsp_magma.inc>
#include <mgfx_constants.h>

MgBeginShaderUniforms
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
MgEndShaderUniforms

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
    #define vclip_factors   $v09
    #define vtexscale       $v10
    #define vtexoffset      $v11
    #define vnormmask       $v12
    #define vnormfactor     $v13

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

    # clear VCO_LO 
    # TODO: Is this necessary?
    vadd v___, vzero, vzero

    # cache_offset * MAGMA_VTX_SIZE, vertex_count * (MAGMA_VTX_SIZE - MGFX_VTX_SIZE)
    vmudh $v02, $v01, vconst
    # cache_offset * MAGMA_VTX_SIZE + vertex_count * (MAGMA_VTX_SIZE - MGFX_VTX_SIZE)
    vadd $v03, $v02, $v02.e1

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

    # load clip factors
    li t1, %lo(MAGMA_CLIP_FACTORS)
    ldv vclip_factors.e0, 0x0,t1
    ldv vclip_factors.e4, 0x0,t1

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

    #define vpos_clip_f     $v23
    #define vpos_clip_i     $v24
    #define vpos_in         $v25
    #define vpos_eye        $v26
    #define vnorm           $v27

    # load vertex position (of two vertices)
    ldv vpos_in.e0, MGFX_VTX_POS + 0,            vtx_in_ptr
    ldv vpos_in.e4, MGFX_VTX_POS + MGFX_VTX_SIZE,vtx_in_ptr

    # Unpack normal
    vand v___, vnormmask, vpos_in.h3
    vmudn vnorm, v___, vnormfactor

    # Set W components to 1
    vmov vpos_in.e3, K32
    vmov vpos_in.e7, K32

    #define vmmv0_i $v14
    #define vmmv0_f $v15
    #define vmmv1_i $v16
    #define vmmv1_f $v17
    #define vmmv2_i $v18
    #define vmmv2_f $v19
    #define vmmv3_i $v20
    #define vmmv3_f $v21

    # transform vertex position to eye space
    li t1, %lo(MATRICES_MV)
    ldv vmmv0_i.e0, 0x00,t1
    ldv vmmv0_i.e4, 0x00,t1
    ldv vmmv1_i.e0, 0x08,t1
    ldv vmmv1_i.e4, 0x08,t1
    ldv vmmv2_i.e0, 0x10,t1
    ldv vmmv2_i.e4, 0x10,t1
    ldv vmmv3_i.e0, 0x18,t1
    ldv vmmv3_i.e4, 0x18,t1
    ldv vmmv0_f.e0, 0x20,t1
    ldv vmmv0_f.e4, 0x20,t1
    ldv vmmv1_f.e0, 0x28,t1
    ldv vmmv1_f.e4, 0x28,t1
    ldv vmmv2_f.e0, 0x30,t1
    ldv vmmv2_f.e4, 0x30,t1
    ldv vmmv3_f.e0, 0x38,t1
    ldv vmmv3_f.e4, 0x38,t1

    vmudn v___,     vmmv0_f, vpos_in.h0
    vmadh v___,     vmmv0_i, vpos_in.h0
    vmadn v___,     vmmv1_f, vpos_in.h1
    vmadh v___,     vmmv1_i, vpos_in.h1
    vmadn v___,     vmmv2_f, vpos_in.h2
    vmadh v___,     vmmv2_i, vpos_in.h2
    vmadn v___,     vmmv3_f, vpos_in.h3
    vmadh vpos_eye, vmmv3_i, vpos_in.h3

    #undef vmmv0_i
    #undef vmmv0_f
    #undef vmmv1_i
    #undef vmmv1_f
    #undef vmmv2_i
    #undef vmmv2_f
    #undef vmmv3_i
    #undef vmmv3_f

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

    # 32-bit right shift by 5, to keep the clip space coordinates unscaled
    vmudm vpos_clip_i, vpos_clip_i, vshift8.e4
    vmadl vpos_clip_f, vpos_clip_f, vshift8.e4

    #define vmn0_i $v14
    #define vmn0_f $v15
    #define vmn1_i $v16
    #define vmn1_f $v17
    #define vmn2_i $v18
    #define vmn2_f $v19

    # transform normal to eye space
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

    vmudn v___,  vmn0_f, vnorm.h0
    vmadh v___,  vmn0_i, vnorm.h0
    vmadn v___,  vmn1_f, vnorm.h1
    vmadh v___,  vmn1_i, vnorm.h1
    vmadn v___,  vmn2_f, vnorm.h2
    vmadh vnorm, vmn2_i, vnorm.h2

    #undef vmn0_i
    #undef vmn0_f
    #undef vmn1_i
    #undef vmn1_f
    #undef vmn2_i
    #undef vmn2_f

    #define vsqdist_i   $v14
    #define vsqdist_f   $v15
    #define vinvdist_i  $v16
    #define vinvdist_f  $v17

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

    # compute lighting

    #define vlight      $v25
    #define vinvdist_i  $v21
    #define vinvdist_f  $v22

    # Initialize the third lane of vinvdist to 1.0
    vxor vinvdist_f, vinvdist_f
    vmov vinvdist_i.e2, K1
    vmov vinvdist_i.e6, K1

    lw t6, %lo(LIGHTING_COUNT)
    li s0, %lo(LIGHTING_LIGHTS)
    ldv vlight.e0, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
    ldv vlight.e4, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
light_loop:
    #define vlpos   $v14

    # If the light is directional, the light vector is simply a direction (pre-normalized on CPU)
    blez t6, light_loop_end
    ldv vlpos.e0, MGFX_LIGHT_POSITION,s0
    ldv vlpos.e4, MGFX_LIGHT_POSITION,s0

    #define vatt_i      $v15
    #define vatt_f      $v16

    lh t4, %lo(MGFX_LIGHT_POSITION) + 0x6(s0)
    vcopy vatt_i, K1
    beqz t4, 1f
    vcopy vatt_f, vzero

    #define vsqdist_i   $v17
    #define vsqdist_f   $v18
    #define vdist_i     $v19
    #define vdist_f     $v20

    # Light is positional: We need to compute light vector, normalize it, and apply attenuation

    # Load attenuation coefficients k0, k1, k2 (constant, linear, quadratic)
    # vattenuation: k0  k1  k2  --  k0  k1  k2  --
    ldv vatt_i.e0, MGFX_LIGHT_ATT_INT,s0
    ldv vatt_i.e4, MGFX_LIGHT_ATT_INT,s0
    ldv vatt_f.e0, MGFX_LIGHT_ATT_FRAC,s0
    ldv vatt_f.e4, MGFX_LIGHT_ATT_FRAC,s0

    # If light is positional, the light vector points from the vertex to the light position
    # This is shifted left by 5 because both values are in s10.5 format
    vsubc vlpos, vpos_eye
    
    # Dot product of light vector with itself
    # Product is shifted left by 10 because two s10.5 values were multiplied,
    # and then shifted right by 16 because of vsar.
    # This means the result is shifted right by 6
    vmudh v___, vlpos, vlpos
    vsar  vsqdist_f, COP2_ACC_MD
    vsar  vsqdist_i, COP2_ACC_HI
    vaddc vdist_f, vsqdist_f, vsqdist_f.h1
    vadd  vdist_i, vsqdist_i, vsqdist_i.h1
    vaddc vsqdist_f, vdist_f, vsqdist_f.h2
    vadd  vsqdist_i, vdist_i, vsqdist_i.h2

    # Compute inverse distance (reciprocal square root)
    # Result is shifted left by 10:
    #   - Taking the square root halves the bit-shift, and the reciprocal then inverts it.
    #     So the original (right) shift of -6 becomes -(-6/2) = 3
    #   - vrsq additionally shifts left by 7
    # vinvdist: --  1/d0  1.0  --  --  1/d1  1.0  --
    vrsqh v___.e0,       vsqdist_i.e0
    vrsql vinvdist_f.e1, vsqdist_f.e0
    vrsqh vinvdist_i.e1, vsqdist_i.e4
    vrsql vinvdist_f.e5, vsqdist_f.e4
    vrsqh vinvdist_i.e5, vzero.e0

    # Get actual distance by multiplying the inverse with the squared distance: d^-1 * d^2 = d^(2-1) = d
    # Because vinvdist.e2 is initialized to 1, the squared distance will be in vdist.e2
    # d is shifted left by 4, d^2 is still shifted right by 6
    # vdist: --  d0  d0^2  --  --  d1  d1^2  --
    vmudl v___,    vinvdist_f, vsqdist_f.h0
    vmadm v___,    vinvdist_i, vsqdist_f.h0
    vmadn vdist_f, vinvdist_f, vsqdist_i.h0
    vmadh vdist_i, vinvdist_i, vsqdist_i.h0

    # Multiply with attenuation coefficients
    # The coefficients are pre-shifted in such a way that all values end up being shifted right by 1, 
    # so the final result ends up non-shifted after the reciprocal below.
    # - d is shifted left by 4, so k1 is pre-shifted right by 4 on CPU
    # - d^2 is shifted right by 6, so k2 is pre-shifted left by 6 on CPU
    # vdist: --  k1*d0  k2*d0^2  --  --  k1*d1  k2*d1^2  --
    vmudl v___,    vdist_f, vatt_f
    vmadm v___,    vdist_i, vatt_f
    vmadn vdist_f, vdist_f, vatt_i
    vmadh vdist_i, vdist_i, vatt_i

    # Compute final attenuation factor
    # Sum is shifted right by 1
    # k0 + k1*d + k2*d^2
    vaddc vatt_f, vdist_f.h1
    vadd  vatt_i, vdist_i.h1
    vaddc vatt_f, vdist_f.h2
    vadd  vatt_i, vdist_i.h2
    # Final factor is not shifted
    # 1 / (k0 + k1*d + k2*d^2)
    vrcph v___.e0,   vatt_i.e0
    vrcpl vatt_f.e0, vatt_f.e0
    vrcph vatt_i.e0, vatt_i.e4
    vrcpl vatt_f.e4, vatt_f.e4
    vrcph vatt_i.e4, vzero.e0

    # Normalize light vector by multiplying the reciprocal distance.
    # Light vector is shifted left by 5 and inverse distance is shifted left by 10.
    # This means the result is shifted left by 15, which makes the result in vlightdir a signed fraction.
    # This happens to match perfectly so we can continue the following calculations without any adjustment.
    vmudm v___,  vlpos, vinvdist_f.h1
    vmadh vlpos, vlpos, vinvdist_i.h1

    #undef vsqdist_i
    #undef vsqdist_f
    #undef vdist_i
    #undef vdist_f

1:
    #define vlcol   $v17
    #define vndl    $v18

    ldv vlcol.e0, MGFX_LIGHT_COLOR,s0
    ldv vlcol.e4, MGFX_LIGHT_COLOR,s0
    
    # Dot product of light vector with vertex normal
    # Both are a signed fraction, so we can just use vmulf
    vmulf vndl, vnorm, vlpos,
    vadd  v___, vndl, vndl.h1
    vadd  vndl, v___, vndl.h2
    vge   vndl, vzero

    # Diffuse light = light_color * dot(light_vec, normal)
    vmulf vlcol, vndl.h0

    # Multiply by attenuation (if the light is directional, it's just 1)
    vmudm v___,  vlcol, vatt_f.h0
    vmadh vlcol, vlcol, vatt_i.h0

    # Accumulate final light color
    vadd vlight, vlcol

    addi t6, -1
    b light_loop
    addi s0, MGFX_LIGHT_SIZE
    #undef vlpos
    #undef vlcol
    #undef vndl
    #undef vatt_i
    #undef vatt_f

    #undef vnorm

light_loop_end:

    #undef vinvdist_i
    #undef vinvdist_f

    #define vrgba $v14

    # load rgba (of two vertices)
    # shuffle values around so they can be loaded in a single luv op
    llv vrgba.e0, MGFX_VTX_RGBA + 0,             vtx_in_ptr
    llv vrgba.e2, MGFX_VTX_RGBA + MGFX_VTX_SIZE, vtx_in_ptr
    sdv vrgba.e0, 0,vtx_out0_ptr
    luv vrgba.e0, 0,vtx_out0_ptr

    # multiply vertex color by accumulated final light color
    vmulf vrgba, vlight

    #undef vlight

    #define vtmp    $v15
    #define vfog_i  $v16
    #define vfog_f  $v17
    andi t0, flags, MGFX_FLAG_FOG
    beqz t0, 1f
    li t1, %lo(FOG)
    llv vfog_i.e0, 0,t1
    llv vfog_i.e4, 0,t1
    llv vfog_f.e0, 4,t1
    llv vfog_f.e4, 4,t1

    # Shade alpha = 0 means no fog.
    # Shade alpha = 1 means full fog.
    # Therefore, we need to compute the following formula:
    # (abs(veyepos.z) - fog_start) / (fog_end - fog_start)

    # Compute abs(veyepos.z).
    # abs(veyepos.z) is an approximation for the distance between the
    # vertex and the origin in eye space, as recommended by the GL spec.
    vsubc vtmp, vzero, vpos_eye.h2
    vge vtmp, vpos_eye.h2

    # vtmp.e0 = abs(veyepos.z) - fog_start
    # Note that fog_start might be negative. In practice this would
    # rarely be the case, but it is not forbidden by the GL spec.
    vsubc vtmp, vfog_i.h1

    # vtmp.e0 = (abs(veyepos.z) - fog_start) / (fog_end - fog_start)
    # The factor is premultiplied so that combined with VTX_SHIFT
    # the product will be in 1.15 precision and saturated to 0x7FFF.
    vmudm v___, vtmp, vfog_f.h0
    vmadh vtmp, vtmp, vfog_i.h0

    # Clamp negative values to 0
    vge vtmp, vzero

    # Save the alpha factor in the vertex color, overwriting the alpha component.
    vmov vrgba.e3, vtmp.e0
    vmov vrgba.e7, vtmp.e4
    #undef vtmp
    #undef vfog_i
    #undef vfog_f
1:

    # store rgba to vertex cache
    # CAUTION: this also overwrites the next 4 bytes after the rgba value
    suv vrgba.e0, MAGMA_VTX_RGBA,vtx_out0_ptr
    suv vrgba.e4, MAGMA_VTX_RGBA,vtx_out1_ptr

    #undef vpos_eye
    #undef vrgba

    #define vtex $v14

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

    #define vinvw_i     $v14
    #define vinvw_f     $v15
    #define vpos_scr_i  $v16
    #define vpos_scr_f  $v17
    #define vviewscale  $v18
    #define vviewoff    $v19

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

    # load viewport
    li t1, %lo(MAGMA_VIEWPORT)
    ldv vviewscale.e0, 0x0,t1
    ldv vviewscale.e4, 0x0,t1
    ldv vviewoff.e0,   0x8,t1
    ldv vviewoff.e4,   0x8,t1

    # transform to screen space
    vmudh v___, vviewoff, K1
    vmadn vpos_scr_f, vviewscale
    vmadh vpos_scr_i, vviewscale

    # store screen space position to vertex cache
    sdv vpos_scr_i.e0,  MAGMA_VTX_XYZ,vtx_out0_ptr
    sdv vpos_scr_i.e4,  MAGMA_VTX_XYZ,vtx_out1_ptr

    #undef vinvw_i
    #undef vinvw_f
    #undef vpos_scr_i
    #undef vpos_scr_f
    #undef vviewscale
    #undef vviewoff

    #define vguard_i        $v14
    #define vguard_f        $v15

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

    #undef vpos_clip_i
    #undef vpos_clip_f
    
    addi vtx_out0_ptr, MAGMA_VTX_SIZE*2
    b vertex_loop
    addi vtx_in_ptr, MGFX_VTX_SIZE*2

    #undef v___
    #undef vconst
MgEndShader
