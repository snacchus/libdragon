#include <rsp_queue.inc>
#include "magma_constants.h"

#define TRIANGLE_SIZE 0xB0

.macro MgBeginShaderUniforms
    .section .bss.magma_shader_uniforms
    .align 4
_MG_VTX_SHADER_UNIFORMS:
.endm

.macro MgEndShaderUniforms
    .align 3
_MG_VTX_SHADER_UNIFORMS_END:
.endm

.equ _mg_defining_uniform, 0

.macro MgBeginUniform name binding
    .ifne _mg_defining_uniform
        .error "MgBeginUniform must not be preceded by another MgBeginUniform"
        .exitm
    .endif
    .equ _mg_defining_uniform, 1

    .align 4
\name:
_MG_BINDING_\binding:
.endm

.macro MgEndUniform
    .ifeq _mg_defining_uniform
        .error "MgEndUniform must be preceded by MgBeginUniform"
        .exitm
    .endif
    .equ _mg_defining_uniform, 0
.endm

.macro MgBeginShader
    .section .text.magma_vertex_shader
    .align 3

_MG_VTX_SHADER:
.endm

.macro MgEndShader
    .align 3
_MG_VTX_SHADER_END:
.endm

    .data

	RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MgCmd_SetByte,       8
        RSPQ_DefineCommand MgCmd_SetShort,      8
        RSPQ_DefineCommand MgCmd_SetWord,       8
        RSPQ_DefineCommand MgCmd_SetQuad,       20
        RSPQ_DefineCommand MgCmd_SetShader,     12
        RSPQ_DefineCommand MgCmd_LoadVertices,  8
        RSPQ_DefineCommand MgCmd_DrawIndices,   4
        RSPQ_DefineCommand MgCmd_DrawEnd,       4
        RSPQ_DefineCommand MgCmd_LoadUniform,   8
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_INLINE_UNIFORM_HEADER + 8
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_INLINE_UNIFORM_HEADER + 16
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_INLINE_UNIFORM_HEADER + 32
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_INLINE_UNIFORM_HEADER + 64
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_INLINE_UNIFORM_HEADER + 128
        RSPQ_DefineCommand MgCmd_LoadUniform,   248 # TODO: get rid of magic number
	RSPQ_EndOverlayHeader

    .align 4
    .ascii "Dragon RSP Magma"
    .ascii "Code by snacchus"

    .align 4
MAGMA_CONSTANTS:                .half   MAGMA_VTX_SIZE, 0, 0, 0, 0, 0, 0, 0
MAGMA_NORMAL_MASK:              .half   0xF800, 0x07E0, 0x001F, 0, 1, 1<<5, 1<<11, 0
MAGMA_CLIP_CACHE_OFFSETS:       .half   2,4,6,8,10,12,14,16,18

MAGMA_CLIP_PLANES:              
    .half 1, 0, 0,  1
    .half 0, 1, 0,  1
    .half 0, 0, 1,  1
    .half 1, 0, 0, -1
    .half 0, 1, 0, -1
    .half 0, 0, 1, -1

    RSPQ_BeginSavedState
    
    .align 3
    MAGMA_VERTEX_CACHE:         .dcb.b  MAGMA_VERTEX_CACHE_COUNT * MAGMA_VTX_SIZE
    MAGMA_VERTEX_OVERFLOW:      .dcb.b  MAGMA_VTX_SIZE

    .align 4
MAGMA_STATE:
    MAGMA_VIEWPORT:
        MAGMA_VIEWPORT_SCALE:   .half   0,0,0,0
        MAGMA_VIEWPORT_OFFSET:  .half   0,0,0,0
    MAGMA_CLIP_FACTORS:         .half   1,1,MAGMA_DEFAULT_GUARD_BAND,MAGMA_DEFAULT_GUARD_BAND
    MAGMA_SHADER_TEXT:          .word   0
    MAGMA_SHADER_DATA:          .word   0
    MAGMA_SHADER_TEXT_SIZE:     .half   0
    MAGMA_SHADER_DATA_SIZE:     .half   0
    MAGMA_VERTEX_BUFFER:        .word   0    
    MAGMA_TRI_CMD:              .half   0x800
    MAGMA_CULL_MODE:            .byte   0
    MAGMA_OUTPUT_OFFSET:        .byte   0    

    RSPQ_EndSavedState

    .bss
    .align 4
MAGMA_CLIP_CACHE_USED:          .dcb.w  16
    .align 3
MAGMA_OUTPUT_BUFFER:            .dcb.b  TRIANGLE_SIZE * 2

MAGMA_CLIP_CACHE:               .dcb.b  MAGMA_VTX_SIZE * MAGMA_CLIP_CACHE_SIZE
MAGMA_CLIP_CACHE_END:

MAGMA_CLIP_LIST0:               .dcb.w  MAGMA_CLIP_CACHE_SIZE
MAGMA_CLIP_LIST1:               .dcb.w  MAGMA_CLIP_CACHE_SIZE

    .text

    #############################################################
    # MgCmd_SetByte
    #############################################################
    .func MgCmd_SetByte
MgCmd_SetByte:
    jr ra
    sb a1, %lo(MAGMA_STATE)(a0)
    .endfunc

    #############################################################
    # MgCmd_SetShort
    #############################################################
    .func MgCmd_SetShort
MgCmd_SetShort:
    jr ra
    sh a1, %lo(MAGMA_STATE)(a0)
    .endfunc

    #############################################################
    # MgCmd_SetWord
    #############################################################
    .func MgCmd_SetWord
MgCmd_SetWord:
    jr ra
    sw a1, %lo(MAGMA_STATE)(a0)
    .endfunc

    #############################################################
    # MgCmd_SetQuad
    #############################################################
    .func MgCmd_SetQuad
MgCmd_SetQuad:
    addi s0, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER)
    lqv $v01, -0x10,s0
    lrv $v01,  0x00,s0
    addi a0, %lo(MAGMA_STATE)
    jr ra
    sqv $v01,  0x00,a0
    .endfunc

    #############################################################
    # MgCmd_LoadUniform
    # ARGS:
    #   a0: RDRAM pointer
    #   a1: Bit 31..16: Size
    #       Bit 15..0: Offset into _MG_VTX_SHADER_UNIFORMS
    #############################################################
    .func MgCmd_LoadUniform
MgCmd_LoadUniform:
    move s0, a0
    addi s4, a1, %lo(_MG_VTX_SHADER_UNIFORMS)
    j DMAIn
    srl t0, a1, 16
    
    .endfunc

    #############################################################
    # MgCmd_SetShader
    # ARGS:
    #   a0: RDRAM pointer to shader ucode text
    #   a1: RDRAM pointer to shader ucode data
    #   a2: Bit 31..16: shader text size
    #       Bit 15..0: shader data size
    #############################################################
    .func MgCmd_SetShader
MgCmd_SetShader:
    sw a0, %lo(MAGMA_SHADER_TEXT)
    sw a1, %lo(MAGMA_SHADER_DATA)
    sw a2, %lo(MAGMA_SHADER_TEXT_SIZE)
    # fallthrough
    .endfunc

Mg_LoadShader:
    # move ra2, ra
    lw s0, %lo(MAGMA_SHADER_TEXT)
    lhu t0, %lo(MAGMA_SHADER_TEXT_SIZE)
    li s4, %lo(_MG_VTX_SHADER)
    andi t1, s4, 0xFFF
    add s0, t1
    j DMAIn
    sub t0, t1

    # TODO: Uniforms are not being saved currently!
    # TODO: Shader data not needed?
    # lw s0, %lo(MAGMA_SHADER_DATA)
    # lh t0, %lo(MAGMA_SHADER_DATA_SIZE)
    # li s4, %lo(_MG_VTX_SHADER_UNIFORMS)
    # add s0, s4
    # sub t0, s4
    # emux_log_string "dma shader data\n"
    # j DMAIn
    # move ra, ra2

    #############################################################
    # MgCmd_LoadVertices
    # ARGS:
    #   a0: Bit 31..24: Command id
    #       Bit 23..0:  Vertex buffer offset
    #   a1: Bit 31..16: Cache offset
    #       Bit 15..0:  Vertex count
    #############################################################
    .func MgCmd_LoadVertices
MgCmd_LoadVertices:
    # We can't fall through here because RDPQ_Triangle_Send_Async etc. is always placed after the .text segment
    # and we therefore need to put the vertex shader into it's own segment to force it being placed at the end.
    # That means we can't register the command with the shader's address directly because it is not in the .text segment.
    # TODO: is there a better way?
    j _MG_VTX_SHADER
    nop
    .endfunc

    #############################################################
    # MgCmd_DrawIndices
    # 
    # ARGS:
    #   a0: Bit 31..24: Command id
    #       Bit 23..16: Index 0
    #       Bit 15..8:  Index 1
    #       Bit 7..0:   Index 2
    #############################################################
    .func MgCmd_DrawIndices
MgCmd_DrawIndices:
    #define v___            $v29
    #define vconst          $v28
    #define vindices        $v27
    #define voffsets        $v26
    #define vvertices       $v25
    #define vtx1            a1
    #define vtx2            a2
    #define vtx3            a3
    #define clip_flags_out  t3
    #define clip_flags      t5
    li t0, %lo(MAGMA_CONSTANTS)
    lqv vconst, 0,t0
    addi t1, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 3
    luv vindices, 0,t1
    li t0, %lo(MAGMA_VERTEX_CACHE) + MAGMA_VTX_XYZ
    mtc2 t0, vconst.e1
    lbu v0, %lo(MAGMA_CULL_MODE)
    vsrl vindices, vindices, 7
    vmudh voffsets, vindices, vconst.e0
    # clear VCO_LO 
    # TODO: Is this necessary?
    vadd v___, vzero, vzero
    vadd vvertices, voffsets, vconst.e1
    li s1, %lo(MAGMA_CLIP_LIST0)
    move s2, s1
    sdv vvertices.e0, 0,s1
    lbu t7, %lo(MAGMA_OUTPUT_OFFSET)
    li t8, %lo(MAGMA_OUTPUT_BUFFER)
    add t8, t7
    move s3, t8
    
    lhu vtx1, 0(s1)
    lhu vtx2, 2(s2)
    lhu vtx3, 4(s2)
    jal RDPQ_Triangle_Send_Async
    lhu a0, %lo(MAGMA_TRI_CMD)

    # Check if RDPQ_Triangle_Send_Async emitted a triangle:
    beq s3, t8, 1f
    xor t7, TRIANGLE_SIZE
    # If it did, swap the output buffer
    sb t7, %lo(MAGMA_OUTPUT_OFFSET)

1:
    # Check if clipping is required
    beqz v1, RSPQ_Loop
    srl clip_flags, clip_flags_out, 8

    #undef v___
    #undef vconst
    #undef vindices
    #undef voffsets
    #undef vvertices

    #define out_count       v1
    #define plane_flag      t6
    #define in_count        t7
    #define in_end          t8
    #define in_list         s0
    #define out_list        s1
    #define plane           s2
    #define intersection    s3
    #define cur_ptr         s4
    #define prev_ptr        s5
    #define cur_vtx         s6
    #define prev_vtx        s7
    #define p0              k0
    #define p1              k1

    #define vplane          $v01
    #define vint_f          $v02
    #define vint_i          $v03
    #define vdot_i          $v04
    #define vdot_f          $v05
    #define vdiff_i         $v06
    #define vdiff_f         $v07
    #define va_i            $v08
    #define va_f            $v09
    #define vpos_i          $v10
    #define vpos_f          $v11
    #define vattr0          $v12
    #define vattr1          $v13
    #define voff0           $v14
    #define voff1           $v15
    #define vcache0         $v16
    #define vcache1         $v17
    #define vclip_factors   $v18
    #define vviewscale      $v19
    #define vviewoff        $v20
    #define v___            $v29

    li in_list, %lo(MAGMA_CLIP_LIST1)
    move in_count, zero

    # The three original vertices are already in the out_list
    # (after the initial swap they will be in the in_list)
    li out_count, 3*2

    li plane, %lo(MAGMA_CLIP_PLANES)
    li plane_flag, 1

    # Load cache offsets    
    li t0, %lo(MAGMA_CLIP_CACHE_OFFSETS)
    vxor voff1, voff1
    lqv voff0,  0,t0
    lsv voff1, 16,t0

    # Temporarily use the output buffer as a map of which cache slots are used
    # Init to zero
    li t0, %lo(MAGMA_CLIP_CACHE_USED)
    sqv vzero,  0,t0
    sqv vzero, 16,t0

    li t0, %lo(MAGMA_VIEWPORT)
    ldv vviewscale, 0,t0
    ldv vviewoff,   8,t0

    li t0, %lo(MAGMA_CLIP_FACTORS)
    ldv vclip_factors.e0, 0x0,t0
    ldv vclip_factors.e4, 0x0,t0

    # Iterate over the 6 clipping planes
mg_clip_plane_loop:
    and t0, clip_flags, plane_flag
    beqz t0, mg_clip_plane_loop_end
    move t1, in_list

    # Swap in and out lists

    # If the out list is empty from the last iteration, 
    # the triangle has no visible points and we are done
    beqz out_count, RSPQ_Loop
    move in_list, out_list
    move out_list, t1
    move in_count, out_count
    move out_count, zero

    # Repeat plane coefficients twice
    ldv vplane.e0,  0,plane
    ldv vplane.e4,  0,plane

    vmudh vplane, vclip_factors

    # Iterate over the egdes of the polygon in the input list
    # The current edge is between cur_vtx and prev_vtx
    move cur_ptr, in_list
    add in_end, in_list, in_count
    # Init the "previous" vertex to the last in the list for the wrap-around
    addi prev_ptr, in_end, -2

mg_clip_edge_loop:
    #define cur_flag  t3
    #define prev_flag t4

    # Check which side of the plane the two vertices are on
    lhu cur_vtx, 0(cur_ptr)
    lhu prev_vtx, 0(prev_ptr)
    lbu cur_flag, MAGMA_VTX_CLIP_CODE(cur_vtx)
    lbu prev_flag, MAGMA_VTX_CLIP_CODE(prev_vtx)
    and cur_flag, plane_flag
    and prev_flag, plane_flag

    # If they are on opposite sides, there is an intersection
    xor t0, cur_flag, prev_flag
    beqz t0, mg_clip_no_intersection
    move p0, cur_vtx

    # Swap the two points if necessary to make intersection calculation consistent
    # This will make sure p0 is always inside and p1 is always outside
    bnez prev_flag, 1f
    move p1, prev_vtx
    xor p0, p0, p1
    xor p1, p0, p1
    xor p0, p0, p1

    #undef prev_flag

1:
    # Calculate intersection of the line segment and the plane

    li t0, %lo(MAGMA_CLIP_CACHE_USED)
    lqv vcache0,    0,t0
    lqv vcache1,   16,t0

    # vpos: x0  y0  z0  w0  x1  y1  z1  w1
    ldv vpos_i.e0,  MAGMA_VTX_CS_POSi,p0
    ldv vpos_f.e0,  MAGMA_VTX_CS_POSf,p0
    ldv vpos_i.e4,  MAGMA_VTX_CS_POSi,p1
    ldv vpos_f.e4,  MAGMA_VTX_CS_POSf,p1

    # vint: x1  y1  z1  w1
    ldv vint_i.e0,  MAGMA_VTX_CS_POSi,p1
    ldv vint_f.e0,  MAGMA_VTX_CS_POSf,p1

    # vattr0: r0  g0  b0  a0  s0  t0
    luv vattr0.e0,  MAGMA_VTX_RGBA   ,p0
    llv vattr0.e4,  MAGMA_VTX_ST     ,p0

    # vattr1: r1  g1  b1  a1  s1  t1
    luv vattr1.e0,  MAGMA_VTX_RGBA   ,p1
    llv vattr1.e4,  MAGMA_VTX_ST     ,p1

    # Find first free slot in clip cache

    # Add the values from the "used slots map" to the cache offsets
    # After this, each lane will contain the offset of its corresponding cache slot,
    # but only if the slot is not used. If it is used, it will contain some large value.
    vaddc vcache0, voff0
    vaddc vcache1, voff1

    # Look for the smallest value, which will end up in vcache.e0
    # Because used slots are marked as large values, they will never be found.
    vlt vcache0, vcache0.q1
    vlt vcache0, vcache0.h2
    vlt vcache0, vcache0.e4
    vlt vcache0, vcache1.e0

    mfc2 t0, vcache0.e0

    # Mark slot as used by storing some large value (careful of overflows!)
    li t1, 0xFF
    sh t1, %lo(MAGMA_CLIP_CACHE_USED)-2(t0)

    # t0 is the index multiplied by 2
    # intersection = t0 * 20 = t0 * 16 + t0 * 4
    sll intersection, t0, 4
    sll t1, t0, 2
    add intersection, t1

    # CAUTION: intersection might point to the same address as either p0 or p1,
    # because one of them is the previous point, which could have been marked unused
    # in the previous iteration. As long as we don't access p0 or p1 after writing to
    # intersection, this is fine.
    addi intersection, %lo(MAGMA_CLIP_CACHE) - MAGMA_VTX_SIZE

    # Store the cache offset in unused memory (used later when finding the cache slot to mark as unused)
    ori t0, 0x80
    sb t0, MAGMA_VTX_TR_CODE(intersection)

    # Compute dot products of both positions with the clip plane
    # vdot.e0: d0 = dot(p0, plane)
    # vdot.e4: d1 = dot(p1, plane)
    vmudn vdot_f, vpos_f, vplane
    vmadh vdot_i, vpos_i, vplane
    vaddc vdot_f, vdot_f.q1
    vadd  vdot_i, vdot_i.q1
    vaddc vdot_f, vdot_f.h2
    vadd  vdot_i, vdot_i.h2

    # d0 - d1
    vsubc vdiff_f, vdot_f, vdot_f.e4
    vsub  vdiff_i, vdot_i, vdot_i.e4

    # 1 / (d0 - d1)
    vrcph v___.e0,  vdiff_i.e0
    vrcpl va_f.e0, vdiff_f.e0
    vrcph va_i.e0, vzero.e0

    # a = d0 / (d0 - d1)
    vmudl v___, va_f, vdot_f.e0
    vmadm v___, va_i, vdot_f.e0
    vmadn va_f, va_f, vdot_i.e0

    # Prepare 0x7FFF in va_i.e0
    vsubc va_i, vshift8, K1

    # a = min(a, 1)
    vge  v___, va_f, vzero
    vmrg va_f, va_f, va_i.e0

    # Account for right shift introduced by vrcp
    vmudn va_f, va_f, K2

    # p1 - p0
    vsubc vint_f, vpos_f
    vsub  vint_i, vpos_i
    # attr1 - attr0
    vsubc vattr1, vattr0

    # Result of linear interpolation:
    # p0 + a * (p1 - p0)
    vmudl v___,   vint_f, va_f.e0
    vmadm v___,   vint_i, va_f.e0
    vmadn vint_f, vpos_f, K1
    vmadh vint_i, vpos_i, K1

    # a * (attr1 - attr0)
    vmudm vattr1, vattr1, va_f.e0

    # attr0 + a * (attr1 - attr0)
    vaddc vattr0, vattr1

    # Store results
    sdv vint_i.e0,  MAGMA_VTX_CS_POSi,intersection
    sdv vint_f.e0,  MAGMA_VTX_CS_POSf,intersection
    suv vattr0.e0,  MAGMA_VTX_RGBA   ,intersection
    slv vattr0.e4,  MAGMA_VTX_ST     ,intersection

    # Re-calculate clip codes
    vmudn vint_f, vclip_factors
    vmadh vint_i, vclip_factors
    vch v___, vint_i, vint_i.h3
    vcl v___, vint_f, vint_f.h3
    cfc2 t0, COP2_CTRL_VCC
    andi t0, 0x707
    srl t1, t0, 5
    or t0, t1
    sb t0, MAGMA_VTX_CLIP_CODE(intersection)

    # Add intersection to the output list
    add t0, out_list, out_count
    sh intersection, 0(t0)
    addi out_count, 2

mg_clip_no_intersection:
    # If cur_vtx is inside, add it to the output list
    bnez cur_flag, 1f
    add t0, out_list, out_count
    sh cur_vtx, 0(t0)
    b mg_clip_edge_loop_end
    addi out_count, 2

    #undef cur_flag

1:
    # Check if the vertex is stored in the clip cache
    lb t0, MAGMA_VTX_TR_CODE(cur_vtx)
    bgez t0, mg_clip_edge_loop_end
    andi t0, 0x7F

    # Reset the padding field to zero, so the screen space values won't be recalculated below
    sb zero, MAGMA_VTX_TR_CODE(cur_vtx)
    # If so, mark it as unused
    sh zero, %lo(MAGMA_CLIP_CACHE_USED)-2(t0)
    
mg_clip_edge_loop_end:
    # Advance to the next edge
    addi cur_ptr, 2
    blt cur_ptr, in_end, mg_clip_edge_loop
    addi prev_ptr, cur_ptr, -2

mg_clip_plane_loop_end:
    # Advance to the next clipping plane
    sll plane_flag, 1
    blt plane_flag, (1<<MAGMA_CLIP_PLANE_COUNT), mg_clip_plane_loop
    addi plane, MAGMA_CLIP_PLANE_SIZE

    #define cache_vtx   s3
    #define cache_end   s5
    #define vinvw_i     $v21
    #define vinvw_f     $v22
    #define vpos_scr_i  $v23
    #define vpos_scr_f  $v24

    # Calculate screen space values for new vertices (in the clip cache)
    # TODO: maybe iterate over out_list instead
    li cache_vtx, %lo(MAGMA_CLIP_CACHE)
    li cache_end, %lo(MAGMA_CLIP_CACHE_END) - MAGMA_VTX_SIZE
mg_clip_finalize_loop:
    lb t0, MAGMA_VTX_TR_CODE(cache_vtx)

    # Only calculate screen space values if the vertex is actually used
    beqz t0, mg_clip_finalize_loop_end
    ldv vint_i,  MAGMA_VTX_CS_POSi,cache_vtx
    ldv vint_f,  MAGMA_VTX_CS_POSf,cache_vtx

    vrcph v___.e3,    vint_i.e3
    vrcpl vinvw_f.e3, vint_f.e3
    vrcph vinvw_i.e3, vzero.e0

    vmudl v___,       vint_f, vinvw_f.h3
    vmadm v___,       vint_i, vinvw_f.h3
    vmadn vpos_scr_f, vint_f, vinvw_i.h3
    vmadh vpos_scr_i, vint_i, vinvw_i.h3

    vmudh v___, vviewoff, K1
    vmadn vpos_scr_f, vviewscale
    vmadh vpos_scr_i, vviewscale

    sdv vpos_scr_i, MAGMA_VTX_XYZ,  cache_vtx
    ssv vint_i.e3,  MAGMA_VTX_Wi,   cache_vtx
    ssv vint_f.e3,  MAGMA_VTX_Wf,   cache_vtx
    ssv vinvw_i.e3, MAGMA_VTX_INVWi,cache_vtx
    ssv vinvw_f.e3, MAGMA_VTX_INVWf,cache_vtx
    sh zero, MAGMA_VTX_CLIP_CODE(cache_vtx)

mg_clip_finalize_loop_end:
    blt cache_vtx, cache_end, mg_clip_finalize_loop
    addi cache_vtx, MAGMA_VTX_SIZE

    #undef cache_vtx
    #undef cache_end
    #undef vinvw_i
    #undef vinvw_f
    #undef vpos_scr_i
    #undef vpos_scr_f

    #undef clip_flags
    #undef plane_flag
    #undef in_count
    #undef in_end
    #undef in_list
    #undef plane
    #undef intersection
    #undef cur_ptr
    #undef prev_ptr
    #undef cur_vtx
    #undef prev_vtx
    #undef p0
    #undef p1
    #undef vplane
    #undef vint_f
    #undef vint_i
    #undef vdot_i
    #undef vdot_f
    #undef vdiff_i
    #undef vdiff_f
    #undef va_i
    #undef va_f
    #undef vpos_i
    #undef vpos_f
    #undef vattr0
    #undef vattr1
    #undef voff0
    #undef voff1
    #undef vcache0
    #undef vcache1
    #undef vclip_factors
    #undef vviewscale
    #undef vviewoff
    #undef v___

    #define out_ptr s2
    #define out_end k0

    lbu t7, %lo(MAGMA_OUTPUT_OFFSET)
    li t8, %lo(MAGMA_OUTPUT_BUFFER)

    move out_ptr, out_list
    add out_end, out_list, out_count
    addi out_end, -4
mg_draw_clipped_triangles_loop:
    bge out_ptr, out_end, RSPQ_Loop
    lhu a0, %lo(MAGMA_TRI_CMD)
    lbu v0, %lo(MAGMA_CULL_MODE)
    lhu vtx1, 0(out_list)
    lhu vtx2, 2(out_ptr)
    lhu vtx3, 4(out_ptr)
    add t9, t7, t8
    jal RDPQ_Triangle_Send_Async
    move s3, t9

    beq s3, t9, 1f
    xor t7, TRIANGLE_SIZE
    # If it did, swap the output buffer
    sb t7, %lo(MAGMA_OUTPUT_OFFSET)

1:
    b mg_draw_clipped_triangles_loop
    addi out_ptr, 2
    #undef out_list
    #undef out_count
    #undef vtx1
    #undef vtx2
    #undef vtx3
    .endfunc

    #############################################################
    # MgCmd_DrawEnd
    #############################################################
    .func MgCmd_DrawEnd
MgCmd_DrawEnd:
    j RDPQ_Triangle_Send_End
    sb zero, %lo(MAGMA_OUTPUT_OFFSET)
    .endfunc

#define RDPQ_TRIANGLE_VTX1_DMEM 0,s1
#define RDPQ_TRIANGLE_VTX2_DMEM 2,s2
#define RDPQ_TRIANGLE_VTX3_DMEM 4,s2
#include "rsp_rdpq.inc"
