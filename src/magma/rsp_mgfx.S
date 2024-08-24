#include <rsp_magma.inc>
#include <mgfx_constants.h>

#define MGFX_TRACE_VTX_LOOP 1

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
    #define mtx_ptr         s6
    #define tex_ptr         s7
    #define v___            $v29

    li t7, %lo(MG_STATE)
    addi t1, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 6

    #define vvtx_size       $v01
    #define vinputs         $v02
    #define vproducts       $v03
    #define vbuf_offset_hi  $v04
    #define vbuf_offset_lo  $v05

    # vertex_size, MG_VTX_SIZE, -vertex_size, vertex_size
    ldv vvtx_size, 0x18,t7

    # buffer_offset, cache_offset, vertex_count, vertex_count
    ldv vinputs.e0, 0,t1
    lsv vinputs.e3, 4,t1

    lhu vtx_size, %lo(MG_VERTEX_SIZE)
    lw flags, %lo(MODES_FLAGS)
    li mtx_ptr, %lo(MATRICES)
    li t5, %lo(MG_NORMAL_MASK)

    # buffer_offset * vertex_size, cache_offset * MG_VTX_SIZE, vertex_count * -vertex_size, vertex_count * vertex_size
    vmudh vproducts, vinputs, vvtx_size;        sll vtx_size2, vtx_size, 1
    vsar vbuf_offset_hi, COP2_ACC_HI;           lw s0, %lo(MG_VERTEX_BUFFER)
    vsar vbuf_offset_lo, COP2_ACC_MD;           li t4, %lo(RSPQ_SCRATCH_MEM)
                                                li tex_ptr, %lo(TEXTURING)

    sdv vproducts, 0,t4
    ssv vbuf_offset_hi.e0, 8,t4
    ssv vbuf_offset_lo.e0, 10,t4

    #undef vvtx_size
    #undef vinputs
    #undef vproducts
    #undef vbuf_offset_hi
    #undef vbuf_offset_lo

    lw  t0,          8(t4)
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

    addi vtx_out_ptr, %lo(MG_VERTEX_CACHE)

    addi s4, %lo(MG_VERTEX_OVERFLOW)
    
    jal DMAInAsync
    andi s4, 0xFF8
    
    move vtx_in0_ptr, s4
    add vtx_in_end, vtx_in0_ptr, t1

    #define vnormmask       $v28
    #define vnormfactor     $v27

    # load mvp matrix

    ldv vnormmask.e0,   0x0,t5
    ldv vnormmask.e4,   0x0,t5
    ldv vnormfactor.e0, 0x8,t5
    jal DMAWaitIdle
    ldv vnormfactor.e4, 0x8,t5

    # SU: 136
    # VU: 166

vertex_loop:
    bge vtx_in0_ptr, vtx_in_end, RSPQ_Loop
    add vtx_in1_ptr, vtx_in0_ptr, vtx_size

#if MGFX_TRACE_VTX_LOOP
    emux_trace_start
#endif

    #define vpos_eye        $v26
    #define vnorm           $v25
    #define vpos_clip_f     $v24
    #define vpos_clip_i     $v23
    #define vrgba_in        $v22
    #define vtex            $v21

    #define vpos_in         $v01

    # load vertex position (of two vertices)
LOAD_POS_NORM0:
    ldv vpos_in.e0, 0,vtx_in0_ptr
LOAD_POS_NORM1:
    ldv vpos_in.e4, 0,vtx_in1_ptr

    #define vmn0_i $v02
    #define vmn0_f $v03
    #define vmn1_i $v04
    #define vmn1_f $v05
    #define vmn2_i $v06
    #define vmn2_f $v07

    #define vmmv0_i $v02
    #define vmmv0_f $v03
    #define vmmv1_i $v04
    #define vmmv1_f $v05
    #define vmmv2_i $v06
    #define vmmv2_f $v07
    #define vmmv3_i $v08
    #define vmmv3_f $v09

    li t1, %lo(MATRICES_NORMAL)
    ldv vmn0_f.e0, 0x20,t1
    ldv vmn0_f.e4, 0x20,t1
    ldv vmn0_i.e0, 0x00,t1

    # Unpack normal
    vand v___, vnormmask, vpos_in.h3;                   ldv vmn0_i.e4, 0x00,t1
    vmov vpos_in.e3, K32;                               ldv vmn1_f.e0, 0x28,t1
    vmov vpos_in.e7, K32;                               ldv vmn1_f.e4, 0x28,t1
                                                        ldv vmn1_i.e0, 0x08,t1
    vmudn vnorm, v___, vnormfactor;                     ldv vmn1_i.e4, 0x08,t1
                                                        ldv vmn2_f.e0, 0x30,t1
                                                        ldv vmn2_f.e4, 0x30,t1
                                                        ldv vmn2_i.e0, 0x10,t1
    vmudn v___,  vmn0_f, vnorm.h0;                      ldv vmn2_i.e4, 0x10,t1
    vmadh v___,  vmn0_i, vnorm.h0;                      ldv vmmv0_f.e0, 0x60,mtx_ptr
    vmadn v___,  vmn1_f, vnorm.h1;                      ldv vmmv0_f.e4, 0x60,mtx_ptr
    vmadh v___,  vmn1_i, vnorm.h1;                      ldv vmmv0_i.e0, 0x40,mtx_ptr
    vmadn v___,  vmn2_f, vnorm.h2;                      ldv vmmv0_i.e4, 0x40,mtx_ptr
    vmadh vnorm, vmn2_i, vnorm.h2;                      ldv vmmv1_f.e0, 0x68,mtx_ptr
                                                        ldv vmmv1_f.e4, 0x68,mtx_ptr

    #define vinvd_i      $v17
    #define vinvd_f      $v18

                                                        lsv vinvd_i.e2, 0x8,t5
                                                        lsv vinvd_i.e6, 0x8,t5

    #undef vmn0_i
    #undef vmn0_f
    #undef vmn1_i
    #undef vmn1_f
    #undef vmn2_i
    #undef vmn2_f

    #define vmvp0_i         $v02
    #define vmvp0_f         $v03
    #define vmvp1_i         $v04
    #define vmvp1_f         $v05
    #define vmvp2_i         $v06
    #define vmvp2_f         $v07
    #define vmvp3_i         $v08
    #define vmvp3_f         $v09

    #define vinvdist_i  $v10
    #define vinvdist_f  $v11
    #define vsqdist_i   $v12
    #define vsqdist_f   $v13

    #define fog_enabled  t8
    #define env_enabled  t9

    # re-normalize normal
    vmudh v___, vnorm, vnorm;                           ldv vmmv1_i.e0, 0x48,mtx_ptr
    vsar  vsqdist_f, COP2_ACC_MD;                       ldv vmmv1_i.e4, 0x48,mtx_ptr
    vsar  vsqdist_i, COP2_ACC_HI;                       ldv vmmv2_f.e0, 0x70,mtx_ptr
    vmudn v___, vmmv0_f, vpos_in.h0;                    ldv vmmv2_f.e4, 0x70,mtx_ptr
    vmadh v___, vmmv0_i, vpos_in.h0;                    ldv vmmv2_i.e0, 0x50,mtx_ptr
    vaddc vinvdist_f, vsqdist_f, vsqdist_f.h1;          ldv vmmv2_i.e4, 0x50,mtx_ptr
    vadd  vinvdist_i, vsqdist_i, vsqdist_i.h1;          ldv vmmv3_f.e0, 0x78,mtx_ptr
    vmadn v___,     vmmv1_f, vpos_in.h1;                ldv vmmv3_f.e4, 0x78,mtx_ptr
    vmadh v___,     vmmv1_i, vpos_in.h1;                ldv vmmv3_i.e0, 0x58,mtx_ptr
    vaddc vsqdist_f, vinvdist_f, vsqdist_f.h2;          ldv vmmv3_i.e4, 0x58,mtx_ptr
    vadd  vsqdist_i, vinvdist_i, vsqdist_i.h2;          ldv vmvp0_f.e0, 0x20,mtx_ptr
    vmadn v___,     vmmv2_f, vpos_in.h2;                ldv vmvp0_f.e4, 0x20,mtx_ptr
    vmadh v___,     vmmv2_i, vpos_in.h2;                ldv vmvp0_i.e0, 0x00,mtx_ptr
    vmadn v___,     vmmv3_f, vpos_in.h3;                ldv vmvp0_i.e4, 0x00,mtx_ptr
    vrsqh v___.e0,       vsqdist_i.e0;                  ldv vmvp1_f.e0, 0x28,mtx_ptr
    vrsql vinvdist_f.e0, vsqdist_f.e0;                  ldv vmvp1_f.e4, 0x28,mtx_ptr
    vrsqh vinvdist_i.e0, vsqdist_i.e4;                  ldv vmvp1_i.e0, 0x08,mtx_ptr
    vrsql vinvdist_f.e4, vsqdist_f.e4;                  ldv vmvp1_i.e4, 0x08,mtx_ptr
    vrsqh vinvdist_i.e4, vzero.e0;                      ldv vmvp2_f.e0, 0x30,mtx_ptr

    #undef vsqdist_i
    #undef vsqdist_f

    vmadh vpos_eye, vmmv3_i, vpos_in.h3;                ldv vmvp2_f.e4, 0x30,mtx_ptr
    vxor vinvd_f, vinvd_f;                              ldv vmvp2_i.e0, 0x10,mtx_ptr
    vmudn v___,        vmvp0_f, vpos_in.h0;             ldv vmvp2_i.e4, 0x10,mtx_ptr
    vmadh v___,        vmvp0_i, vpos_in.h0;             ldv vmvp3_f.e0, 0x38,mtx_ptr

    #define vfogtmp         $v20
    #define vfog_i          $v02
    #define vfog_f          $v03
    #define vviewscale      $v04

    vsubc vfogtmp, vzero, vpos_eye.h2;                  ldv vmvp3_f.e4, 0x38,mtx_ptr
    vmadn v___,        vmvp1_f, vpos_in.h1;             ldv vmvp3_i.e0, 0x18,mtx_ptr
    vmadh v___,        vmvp1_i, vpos_in.h1;             ldv vmvp3_i.e4, 0x18,mtx_ptr
    vmadn v___,        vmvp2_f, vpos_in.h2;             li t1, %lo(FOG)
    vge vfogtmp, vpos_eye.h2;                           llv vfog_i.e0, 0,t1
    vmadh v___,        vmvp2_i, vpos_in.h2;             llv vfog_i.e4, 0,t1
    vmadn vpos_clip_f, vmvp3_f, vpos_in.h3;             ldv vviewscale.e0, 0x0,t7
    vmadh vpos_clip_i, vmvp3_i, vpos_in.h3;             ldv vviewscale.e4, 0x0,t7

    #undef vmmv0_i
    #undef vmmv0_f
    #undef vmmv1_i
    #undef vmmv1_f
    #undef vmmv2_i
    #undef vmmv2_f
    #undef vmmv3_i
    #undef vmmv3_f                 
    #undef vmvp0_i
    #undef vmvp0_f
    #undef vmvp1_i
    #undef vmvp1_f
    #undef vmvp2_i
    #undef vmvp2_f
    #undef vmvp3_i
    #undef vmvp3_f
    #undef vpos_in

    vmudm v___,  vnorm, vinvdist_f.h0;                  llv vfog_f.e0, 4,t1
    vmadh vnorm, vnorm, vinvdist_i.h0;                  llv vfog_f.e4, 4,t1

    #undef vinvdist_i
    #undef vinvdist_f

    #define vrgba           $v01
    #define vtexscale       $v02
    #define vtexoffset      $v03

    #define vguard_i        $v04
    #define vguard_f        $v05
    #define vclip_factors   $v06
    #define vviewscale_i    $v07
    #define vviewscale_f    $v08
    #define vinvw_i         $v09
    #define vinvw_f         $v10
    #define vviewoff        $v11
    #define vnormw_i        $v12
    #define vnormw_f        $v13

    vsubc vfogtmp, vfog_i.h1;                           ldv vclip_factors.e0, 0x10,t7
    vmudm vpos_clip_i, vpos_clip_i, vshift8.e4;         ldv vclip_factors.e4, 0x10,t7
    vmadl vpos_clip_f, vpos_clip_f, vshift8.e4;         ldv vviewoff.e0,   0x8,t7
    vmudm vviewscale_i, vviewscale, vviewscale.h3;      ldv vviewoff.e4,   0x8,t7
    vmadn vviewscale_f, vzero, vzero;                   LOAD_COLOR0: llv vrgba_in.e0, 0, vtx_in0_ptr
    vmudm v___, vfogtmp, vfog_f.h0;                     LOAD_COLOR1: llv vrgba_in.e2, 0, vtx_in1_ptr
    vmadh vfogtmp, vfogtmp, vfog_i.h0;                  LOAD_TEXCOORD0: llv vtex.e0, 0,vtx_in0_ptr
    #undef vfog_i
    #undef vfog_f
    vmudl v___,     vpos_clip_f, vviewscale.h3;         LOAD_TEXCOORD1: llv vtex.e4, 0,vtx_in1_ptr
    vmadm vnormw_i, vpos_clip_i, vviewscale.h3;         andi fog_enabled, flags, MGFX_FLAG_FOG
    #undef vviewscale
    vmadn vnormw_f, vzero, vzero;                       li s0, %lo(LIGHTING_LIGHTS)

    vmudn vguard_f, vpos_clip_f, vclip_factors;         sdv vpos_clip_i.e0,  MG_VTX_CS_POSi + 0,          vtx_out_ptr
    vmadh vguard_i, vpos_clip_i, vclip_factors;         sdv vpos_clip_f.e0,  MG_VTX_CS_POSf + 0,          vtx_out_ptr
    #undef vclip_factors

    vrcph v___.e3,    vnormw_i.e3;                      sdv vpos_clip_i.e4,  MG_VTX_CS_POSi + MG_VTX_SIZE,vtx_out_ptr
    vrcpl vinvw_f.e3, vnormw_f.e3;                      sdv vpos_clip_f.e4,  MG_VTX_CS_POSf + MG_VTX_SIZE,vtx_out_ptr
    vrcph vinvw_i.e3, vnormw_i.e7;                      ldv vrgba.e0, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
    vrcpl vinvw_f.e7, vnormw_f.e7;                      ldv vrgba.e4, MGFX_LIGHT_SIZE * MGFX_LIGHT_COUNT_MAX,s0 # ambient
    vrcph vinvw_i.e7, vzero.e0;                         sltu fog_enabled, zero, fog_enabled
    vch v___, vguard_i, vguard_i.h3;                    lw t6, %lo(LIGHTING_COUNT)
    vcl v___, vguard_f, vguard_f.h3;                    cfc2 t0, COP2_CTRL_VCC
    #undef vguard_i
    #undef vguard_f

    #define vpos_scr_i      $v04
    #define vpos_scr_f      $v05
    vmudl v___,       vpos_clip_f, vinvw_f.h3;          andi env_enabled, flags, MGFX_FLAG_ENV_MAP
    vmadm v___,       vpos_clip_i, vinvw_f.h3;          li v1, 0x7FFF
    vmadn vpos_scr_f, vpos_clip_f, vinvw_i.h3;          ssv vnormw_i.e3, MG_VTX_Wi + 0,          vtx_out_ptr
    vmadh vpos_scr_i, vpos_clip_i, vinvw_i.h3;          ssv vnormw_f.e3, MG_VTX_Wf + 0,          vtx_out_ptr
    vge vfogtmp, vzero;                                 ssv vnormw_i.e7, MG_VTX_Wi + MG_VTX_SIZE,vtx_out_ptr
    vmudh v___, vviewoff, K1;                           ssv vnormw_f.e7, MG_VTX_Wf + MG_VTX_SIZE,vtx_out_ptr
    #undef viewoff
    #undef vnormw_i
    #undef vnormw_f
    vmadl v___,       vpos_scr_f, vviewscale_f;         ssv vinvw_i.e3, MG_VTX_INVWi + 0,          vtx_out_ptr
    vmadm v___,       vpos_scr_i, vviewscale_f;         ssv vinvw_f.e3, MG_VTX_INVWf + 0,          vtx_out_ptr
    vmadn vpos_scr_f, vpos_scr_f, vviewscale_i;         ssv vinvw_i.e7, MG_VTX_INVWi + MG_VTX_SIZE,vtx_out_ptr
    vmadh vpos_scr_i, vpos_scr_i, vviewscale_i;         ssv vinvw_f.e7, MG_VTX_INVWf + MG_VTX_SIZE,vtx_out_ptr
    #undef vinvw_i
    #undef vinvw_f
    #undef vviewscale_i
    #undef vviewscale_f
    vch v___, vpos_clip_i, vpos_clip_i.h3;              srl t2, t0, 4
    vcl v___, vpos_clip_f, vpos_clip_f.h3;              andi t0, 0x707

    andi t2, 0x707
    srl t1, t0, 5
    srl t3, t2, 5
    or t0, t1
    or t2, t3

    # store screen space position to vertex cache
    sdv vpos_scr_i.e0,  MG_VTX_XYZ + 0,          vtx_out_ptr
    sdv vpos_scr_i.e4,  MG_VTX_XYZ + MG_VTX_SIZE,vtx_out_ptr

    #undef vpos_scr_i
    #undef vpos_scr_f


    # store clip codes to vertex cache
    sb t0, MG_VTX_CLIP_CODE +           0(vtx_out_ptr)
    sb t2, MG_VTX_CLIP_CODE + MG_VTX_SIZE(vtx_out_ptr)
    

    cfc2 t0, COP2_CTRL_VCC
    srl t2, t0, 4
    andi t0, 0x707
    andi t2, 0x707
    srl t1, t0, 5
    srl t3, t2, 5
    or t0, t1
    or t2, t3

    # store trivial rejection codes to vertex cache
    sb t0, MG_VTX_TR_CODE +           0(vtx_out_ptr)
    sb t2, MG_VTX_TR_CODE + MG_VTX_SIZE(vtx_out_ptr)
    
    #define vlpos        $v04

    # compute lighting
    ldv vlpos.e0, MGFX_LIGHT_POSITION,s0
    ldv vlpos.e4, MGFX_LIGHT_POSITION,s0
    
light_loop:
    #define vlcol        $v05
    #define vatt_i       $v06
    #define vatt_f       $v07
    #define vdiff        $v08
    #define vsqdist_i    $v09
    #define vsqdist_f    $v10
    #define vdist_i      $v11
    #define vdist_f      $v12

    # If the light is directional, the light vector is simply a direction (pre-normalized on CPU)
    
    

    vcopy vatt_i, K1;                                   blez t6, light_loop_end
                                                        lh t0, %lo(MGFX_LIGHT_POSITION) + 0x6(s0)
    vcopy vatt_f, vzero;                                ldv vlcol.e0, MGFX_LIGHT_COLOR,s0

    # Light is positional: We need to compute light vector, normalize it, and apply attenuation

    # Load attenuation coefficients k0, k1, k2 (constant, linear, quadratic)
    # vattenuation: k0  k1  k2  --  k0  k1  k2  --

    # If light is positional, the light vector points from the vertex to the light position
    # This is shifted left by 5 because both values are in s10.5 format
    vsubc vdiff, vlpos, vpos_eye;                       ldv vlcol.e4, MGFX_LIGHT_COLOR,s0
    
    
    # Dot product of light vector with itself
    # Product is shifted left by 10 because two s10.5 values were multiplied,
    # and then shifted right by 16 because of vsar.
    # This means the result is shifted right by 6
    vmudh v___, vdiff, vdiff;                           beqz t0, 1f
                                                        addi t6, -1
    vsar  vsqdist_f, COP2_ACC_MD;                       ldv vatt_i.e0, MGFX_LIGHT_ATT_INT,s0
    vsar  vsqdist_i, COP2_ACC_HI;                       ldv vatt_i.e4, MGFX_LIGHT_ATT_INT,s0
    vaddc vdist_f, vsqdist_f, vsqdist_f.h1;             ldv vatt_f.e0, MGFX_LIGHT_ATT_FRAC,s0
    vadd  vdist_i, vsqdist_i, vsqdist_i.h1;             ldv vatt_f.e4, MGFX_LIGHT_ATT_FRAC,s0
    vaddc vsqdist_f, vdist_f, vsqdist_f.h2;             
    vadd  vsqdist_i, vdist_i, vsqdist_i.h2;             

    # Compute inverse distance (reciprocal square root)
    # Result is shifted left by 10:
    #   - Taking the square root halves the bit-shift, and the reciprocal then inverts it.
    #     So the original (right) shift of -6 becomes -(-6/2) = 3
    #   - vrsq additionally shifts left by 7
    # vinvd: --  1/d0  1.0  --  --  1/d1  1.0  --
    vrsqh v___.e0,    vsqdist_i.e0
    vrsql vinvd_f.e1, vsqdist_f.e0
    vrsqh vinvd_i.e1, vsqdist_i.e4
    vrsql vinvd_f.e5, vsqdist_f.e4
    vrsqh vinvd_i.e5, vzero.e0

    # Get actual distance by multiplying the inverse with the squared distance: d^-1 * d^2 = d^(2-1) = d
    # Because vinvd.e2 is initialized to 1, the squared distance will be in vdist.e2
    # d is shifted left by 4, d^2 is still shifted right by 6
    # vdist: --  d0  d0^2  --  --  d1  d1^2  --
    vmudl v___,    vinvd_f, vsqdist_f.h0
    vmadm v___,    vinvd_i, vsqdist_f.h0
    vmadn vdist_f, vinvd_f, vsqdist_i.h0
    vmadh vdist_i, vinvd_i, vsqdist_i.h0

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

    # Normalize light vector by multiplying the reciprocal distance.
    # Light vector is shifted left by 5 and inverse distance is shifted left by 10.
    # This means the result is shifted left by 15, which makes the result in vlightdir a signed fraction.
    # This happens to match perfectly so we can continue the following calculations without any adjustment.
    vmudm v___,  vdiff, vinvd_f.h1
    vmadh vlpos, vdiff, vinvd_i.h1

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

    #undef vdiff
    #undef vsqdist_i
    #undef vsqdist_f
    #undef vdist_i
    #undef vdist_f

1:
    #define vndl    $v09
    
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
    addi s0, MGFX_LIGHT_SIZE
    vadd vrgba, vlcol

    ldv vlpos.e0, MGFX_LIGHT_POSITION,s0
    b light_loop
    ldv vlpos.e4, MGFX_LIGHT_POSITION,s0
    #undef vlpos
    #undef vlcol
    #undef vndl
    #undef vatt_i
    #undef vatt_f

light_loop_end:

    #undef vinvd_i
    #undef vinvd_f

    llv vtexscale.e0,  0x0,tex_ptr
    llv vtexscale.e4,  0x0,tex_ptr
    llv vtexoffset.e0, 0x4,tex_ptr
    llv vtexoffset.e4, 0x4,tex_ptr

    # load rgba (of two vertices)
    # shuffle values around so they can be loaded in a single luv op


    sdv vrgba_in.e0, 0,t4
    luv vrgba_in.e0, 0,t4

    # multiply vertex color by accumulated final light color
PATCH_COLOR0:
    vmulf vrgba, vrgba_in

    #undef vrgba_in

    beqz fog_enabled, 1f
    nop

    # Save the alpha factor in the vertex color, overwriting the alpha component.
    vmov vrgba.e3, vfogtmp.e0
    vmov vrgba.e7, vfogtmp.e4
    #undef vfogtmp
1:

    # store rgba to vertex cache
    # CAUTION: this also overwrites the next 4 bytes after the rgba value
    suv vrgba.e0, MG_VTX_RGBA + 0,          vtx_out_ptr
    suv vrgba.e4, MG_VTX_RGBA + MG_VTX_SIZE,vtx_out_ptr

    #undef vrgba

    #define vsqdist_i   $v04
    #define vsqdist_f   $v05
    #define vinvdist_i  $v06
    #define vinvdist_f  $v07
    #define veposnorm   $v08
    #define vtmp_i      $v09
    #define vtmp_f      $v10
    #define refl_i      $v11
    #define refl_f      $v12
    #define vdot        $v13

    # load texcoords (of two vertices)


    
    beqz env_enabled, 1f
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

    #undef vtexscale
    #undef vtexoffset

    # store texcoords to vertex cache
    slv vtex.e0, MG_VTX_ST + 0,          vtx_out_ptr
    slv vtex.e4, MG_VTX_ST + MG_VTX_SIZE,vtx_out_ptr

    #undef vtex

    #undef vpos_clip_i
    #undef vpos_clip_f

#if MGFX_TRACE_VTX_LOOP
    emux_trace_stop
#endif

    addi vtx_out_ptr, MG_VTX_SIZE*2
    b vertex_loop
    add vtx_in0_ptr, vtx_size2

    #undef v___
MgEndShader
