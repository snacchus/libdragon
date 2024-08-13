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
MgEndShaderUniforms

MgBeginVertexInput
    MgBeginVertexAttribute MGFX_ATTRIBUTE_POS_NORM
        MgVertexAttributeLoaders LOAD_POS_NORM0, LOAD_POS_NORM1
    MgEndVertexAttribute 

    MgBeginVertexAttribute MGFX_ATTRIBUTE_COLOR, 1
        MgVertexAttributeLoaders LOAD_COLOR0, LOAD_COLOR1
        MgBeginVertexAttributePatch PATCH_COLOR0
            vnop
        MgEndVertexAttributePatch
    MgEndVertexAttribute 

    MgBeginVertexAttribute MGFX_ATTRIBUTE_TEXCOORD, 1
        MgVertexAttributeLoaders LOAD_TEXCOORD0, LOAD_TEXCOORD1
    MgEndVertexAttribute 
MgEndVertexInput

MgBeginShader
    #define flags           v0
    #define vtx_size        k0
    #define vtx_size2       k1
    #define vtx_in0_ptr     s1
    #define vtx_in1_ptr     s5
    #define vtx_in_end      s2
    #define vtx_out_ptr     s3
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

    li t7, %lo(MAGMA_CLIP_FACTORS)
    addi t1, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 6

    # vertex_size, MAGMA_VTX_SIZE, -vertex_size, vertex_size
    ldv vconst, 8,t7

    # buffer_offset, cache_offset, vertex_count, vertex_count
    ldv $v01.e0, 0,t1
    lsv $v01.e3, 4,t1

    lhu vtx_size, %lo(MAGMA_VERTEX_SIZE)
    lw flags, %lo(MODES_FLAGS)
    li t3, %lo(MATRICES)
    li t5, %lo(MAGMA_NORMAL_MASK)

    # buffer_offset * vertex_size, cache_offset * MAGMA_VTX_SIZE, vertex_count * -vertex_size, vertex_count * vertex_size
    vmudh $v02, $v01, vconst

    sll vtx_size2, vtx_size, 1
    lw s0, %lo(MAGMA_VERTEX_BUFFER)
    li t4, %lo(RSPQ_SCRATCH_MEM)
    li t6, %lo(TEXTURING)

    sdv $v02, 0,t4

    lhu t0,          0(t4)
    lhu vtx_out_ptr, 2(t4)
    lh  s4,          4(t4)
    lhu t1,          6(t4)

    add s0, t0

    # Compensate for misalignment in RDRAM pointer
    andi t0, 0x7
    add t0, t1

    # Round up to next multiple of 8
    addi t0, 0x7
    andi t0, 0xFF8
    addi t0, -1

    addi vtx_out_ptr, %lo(MAGMA_VERTEX_CACHE)

    addi s4, %lo(MAGMA_VERTEX_OVERFLOW)
    
    jal DMAInAsync
    andi s4, 0xFF8
    
    move vtx_in0_ptr, s4
    add vtx_in_end, vtx_in0_ptr, t1

    # load mvp matrix
    ldv vmvp0_i.e0,  0x00,t3
    ldv vmvp0_i.e4,  0x00,t3
    ldv vmvp1_i.e0,  0x08,t3
    ldv vmvp1_i.e4,  0x08,t3
    ldv vmvp2_i.e0,  0x10,t3
    ldv vmvp2_i.e4,  0x10,t3
    ldv vmvp3_i.e0,  0x18,t3
    ldv vmvp3_i.e4,  0x18,t3
    ldv vmvp0_f.e0,  0x20,t3
    ldv vmvp0_f.e4,  0x20,t3
    ldv vmvp1_f.e0,  0x28,t3
    ldv vmvp1_f.e4,  0x28,t3
    ldv vmvp2_f.e0,  0x30,t3
    ldv vmvp2_f.e4,  0x30,t3
    ldv vmvp3_f.e0,  0x38,t3
    ldv vmvp3_f.e4,  0x38,t3

    ldv vnormmask.e0,   0x0,t5
    ldv vnormmask.e4,   0x0,t5
    ldv vnormfactor.e0, 0x8,t5
    ldv vnormfactor.e4, 0x8,t5
    
    # load texture transform
    llv vtexscale.e0,  0x0,t6
    llv vtexscale.e4,  0x0,t6
    llv vtexoffset.e0, 0x4,t6
    llv vtexoffset.e4, 0x4,t6

    # load clip factors
    ldv vclip_factors.e0, 0x0,t7
    jal DMAWaitIdle
    ldv vclip_factors.e4, 0x0,t7

vertex_loop:
    bge vtx_in0_ptr, vtx_in_end, RSPQ_Loop
    add vtx_in1_ptr, vtx_in0_ptr, vtx_size

    #define vpos_in         $v23
    #define vpos_eye        $v24
    #define vnorm           $v25
    #define vpos_clip_f     $v26
    #define vpos_clip_i     $v27

    # load vertex position (of two vertices)
LOAD_POS_NORM0:
    ldv vpos_in.e0, 0,vtx_in0_ptr
LOAD_POS_NORM1:
    ldv vpos_in.e4, 0,vtx_in1_ptr

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

    #define vinvdist_i  $v21
    #define vinvdist_f  $v22
    #define vrgba      $v23

    # Initialize the third lane of vinvdist to 1.0
    vxor vinvdist_f, vinvdist_f
    vmov vinvdist_i.e2, K1
    vmov vinvdist_i.e6, K1

    lw t6, %lo(LIGHTING_COUNT)
    li s0, %lo(LIGHTING_LIGHTS)
    ldv vrgba.e0, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
    ldv vrgba.e4, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
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
    vadd vrgba, vlcol

    addi t6, -1
    b light_loop
    addi s0, MGFX_LIGHT_SIZE
    #undef vlpos
    #undef vlcol
    #undef vndl
    #undef vatt_i
    #undef vatt_f

light_loop_end:

    #undef vinvdist_i
    #undef vinvdist_f

    #define vrgba_in $v14

    # load rgba (of two vertices)
    # shuffle values around so they can be loaded in a single luv op
LOAD_COLOR0:
    llv vrgba_in.e0, 0, vtx_in0_ptr
LOAD_COLOR1:
    llv vrgba_in.e2, 0, vtx_in1_ptr
    sdv vrgba_in.e0, 0,vtx_out_ptr
    luv vrgba_in.e0, 0,vtx_out_ptr

    # multiply vertex color by accumulated final light color
PATCH_COLOR0:
    vmulf vrgba, vrgba_in

    #undef vrgba_in

    #define vtmp    $v14
    #define vfog_i  $v15
    #define vfog_f  $v16
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
    suv vrgba.e0, MAGMA_VTX_RGBA + 0,             vtx_out_ptr
    suv vrgba.e4, MAGMA_VTX_RGBA + MAGMA_VTX_SIZE,vtx_out_ptr

    #undef vrgba

    #define vtex        $v14
    #define vsqdist_i   $v15
    #define vsqdist_f   $v16
    #define vinvdist_i  $v17
    #define vinvdist_f  $v18
    #define veposnorm   $v19
    #define vtmp_i      $v20
    #define vtmp_f      $v21
    #define refl_i      $v22
    #define refl_f      $v23
    #define vdot        $v24

    # load texcoords (of two vertices)
LOAD_TEXCOORD0:
    llv vtex.e0, 0,vtx_in0_ptr
LOAD_TEXCOORD1:
    llv vtex.e4, 0,vtx_in1_ptr

    andi t0, flags, MGFX_FLAG_ENV_MAP
    beqz t0, 1f
    # veposnorm = normalize(veyepos)
    vmudh v___, vpos_eye, vpos_eye
    vsar  vsqdist_f, COP2_ACC_MD
    vsar  vsqdist_i, COP2_ACC_HI
    vaddc vtmp_f, vsqdist_f, vsqdist_f.h1
    vadd  vtmp_i, vsqdist_i, vsqdist_i.h1
    vaddc vsqdist_f, vtmp_f, vsqdist_f.h2
    vadd  vsqdist_i, vtmp_i, vsqdist_i.h2

    vrsqh v___.e0,       vsqdist_i.e0
    vrsql vinvdist_f.e0, vsqdist_f.e0
    vrsqh vinvdist_i.e0, vsqdist_i.e4
    vrsql vinvdist_f.e4, vsqdist_f.e4
    vrsqh vinvdist_i.e4, vzero.e0

    vmudm v___,      vpos_eye, vinvdist_f.h0
    vmadh veposnorm, vpos_eye, vinvdist_i.h0

    #undef vpos_eye

    # vdot = min(dot(veposnorm, vnorm), 0)
    vmulf vdot, veposnorm, vnorm
    vadd vtmp_f, vdot, vdot.h1
    vadd vdot, vtmp_f, vdot.h2
    vlt vdot, vzero

    # negate
    vsub vdot, vzero, vdot

    vcopy vtmp_i, vzero
    vmov vtmp_i.e2, K16384
    vmov vtmp_i.e6, K16384

    # refl = veposnorm - 2 * vdot * vnorm + (0, 0, 1)
    vmulf v___, vnorm, vdot.h0
    vmacf v___, vnorm, vdot.h0 # Add twice to account for multiplication by 2
    vmadh v___, veposnorm, K1
    vmadh v___, vtmp_i, K2

    vsar refl_f, COP2_ACC_MD
    vsar refl_i, COP2_ACC_HI

    # m = 1 / 2 * sqrt(dot(refl, refl))
    vmudl v___,      refl_f, refl_f
    vmadm v___,      refl_i, refl_f
    vmadn vsqdist_f, refl_f, refl_i
    vmadh vsqdist_i, refl_i, refl_i

    vaddc vtmp_f, vsqdist_f, vsqdist_f.h1
    vadd  vtmp_i, vsqdist_i, vsqdist_i.h1
    vaddc vsqdist_f, vtmp_f, vsqdist_f.h2
    vadd  vsqdist_i, vtmp_i, vsqdist_i.h2

    vadd vtmp_i, vzero, K128

    vrsqh v___.e0,       vsqdist_i.e0
    vrsql vinvdist_f.e0, vsqdist_f.e0
    vrsqh vinvdist_i.e0, vsqdist_i.e4
    vrsql vinvdist_f.e4, vsqdist_f.e4
    vrsqh vinvdist_i.e4, vzero.e0

    # vtex = refl * m + 0.5
    vmudh v___, vtmp_i, K1
    vmadl v___, refl_f, vinvdist_f.h0
    vmadm v___, refl_i, vinvdist_f.h0
    vmadn v___, refl_f, vinvdist_i.h0
    vmadh vtex, refl_i, vinvdist_i.h0

    #undef vsqdist_i
    #undef vsqdist_f
    #undef vinvdist_i
    #undef vinvdist_f
    #undef veposnorm
    #undef vtmp_i
    #undef vtmp_f
    #undef refl_i
    #undef refl_f
    #undef vdot
    #undef vnorm
1:

    # transform texcoords
    vmudh v___, vtexoffset, K1
    vmadh vtex, vtex, vtexscale

    # store texcoords to vertex cache
    slv vtex.e0, MAGMA_VTX_ST + 0,             vtx_out_ptr
    slv vtex.e4, MAGMA_VTX_ST + MAGMA_VTX_SIZE,vtx_out_ptr

    #undef vtex

    # store clip space position to vertex cache
    sdv vpos_clip_i.e0,  MAGMA_VTX_CS_POSi + 0,             vtx_out_ptr
    sdv vpos_clip_f.e0,  MAGMA_VTX_CS_POSf + 0,             vtx_out_ptr
    sdv vpos_clip_i.e4,  MAGMA_VTX_CS_POSi + MAGMA_VTX_SIZE,vtx_out_ptr
    sdv vpos_clip_f.e4,  MAGMA_VTX_CS_POSf + MAGMA_VTX_SIZE,vtx_out_ptr

    # store W to vertex cache
    ssv vpos_clip_i.e3, MAGMA_VTX_Wi + 0,             vtx_out_ptr
    ssv vpos_clip_f.e3, MAGMA_VTX_Wf + 0,             vtx_out_ptr
    ssv vpos_clip_i.e7, MAGMA_VTX_Wi + MAGMA_VTX_SIZE,vtx_out_ptr
    ssv vpos_clip_f.e7, MAGMA_VTX_Wf + MAGMA_VTX_SIZE,vtx_out_ptr

    #define vinvw_i     $v14
    #define vinvw_f     $v15
    #define vpos_scr_i  $v16
    #define vpos_scr_f  $v17
    #define vviewscale  $v18
    #define vviewoff    $v19

    # perspective division
    # TODO: Perspective normalization
    vrcph v___.e3,    vpos_clip_i.e3
    vrcpl vinvw_f.e3, vpos_clip_f.e3
    vrcph vinvw_i.e3, vpos_clip_i.e7
    vrcpl vinvw_f.e7, vpos_clip_f.e7
    vrcph vinvw_i.e7, vzero.e0

    # store inverse W to vertex cache
    ssv vinvw_i.e3, MAGMA_VTX_INVWi + 0,             vtx_out_ptr
    ssv vinvw_f.e3, MAGMA_VTX_INVWf + 0,             vtx_out_ptr
    ssv vinvw_i.e7, MAGMA_VTX_INVWi + MAGMA_VTX_SIZE,vtx_out_ptr
    ssv vinvw_f.e7, MAGMA_VTX_INVWf + MAGMA_VTX_SIZE,vtx_out_ptr

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
    sdv vpos_scr_i.e0,  MAGMA_VTX_XYZ + 0,             vtx_out_ptr
    sdv vpos_scr_i.e4,  MAGMA_VTX_XYZ + MAGMA_VTX_SIZE,vtx_out_ptr

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
    sb t0, MAGMA_VTX_CLIP_CODE +              0(vtx_out_ptr)
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
    or t0, t1
    or t2, t3

    # store trivial rejection codes to vertex cache
    sb t0, MAGMA_VTX_TR_CODE +              0(vtx_out_ptr)
    sb t2, MAGMA_VTX_TR_CODE + MAGMA_VTX_SIZE(vtx_out_ptr)

    #undef vpos_clip_i
    #undef vpos_clip_f
    
    addi vtx_out_ptr, MAGMA_VTX_SIZE*2
    b vertex_loop
    add vtx_in0_ptr, vtx_size2

    #undef v___
    #undef vconst
MgEndShader
