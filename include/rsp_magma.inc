#include <rsp_queue.inc>
#include "magma_constants.h"

.macro MgBeginShaderConstants
    .section .bss.magma_shader_constants
    .align 4
_MG_VTX_SHADER_CONST:
.endm

.macro MgEndShaderConstants
    .align 3
_MG_VTX_SHADER_CONST_END:
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
        RSPQ_DefineCommand MgCmd_LoadUniform,   8
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_PUSH_CONSTANT_HEADER + 8
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_PUSH_CONSTANT_HEADER + 16
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_PUSH_CONSTANT_HEADER + 32
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_PUSH_CONSTANT_HEADER + 64
        RSPQ_DefineCommand MgCmd_LoadUniform,   MAGMA_PUSH_CONSTANT_HEADER + 128
        RSPQ_DefineCommand MgCmd_LoadUniform,   248 # TODO: get rid of magic number
	RSPQ_EndOverlayHeader

    .align 4
    .ascii "Dragon RSP Magma"
    .ascii "Code by snacchus"

    .align 4
MAGMA_CONSTANTS:                .half   MAGMA_VTX_SIZE, 0, 0, 0, 0, 0, 0, 0
MAGMA_NORMAL_MASK:              .half   0xF800, 0x07E0, 0x001F, 0, 1, 1<<5, 1<<11, 0

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

    RSPQ_EndSavedState

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
    #       Bit 15..0: Offset into _MG_VTX_SHADER_CONST
    #############################################################
    .func MgCmd_LoadUniform
MgCmd_LoadUniform:
    move s0, a0
    addi s4, a1, %lo(_MG_VTX_SHADER_CONST)
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

    # TODO: Shader data not needed?
    # lw s0, %lo(MAGMA_SHADER_DATA)
    # lh t0, %lo(MAGMA_SHADER_DATA_SIZE)
    # li s4, %lo(_MG_VTX_SHADER_CONST)
    # add s0, s4
    # sub t0, s4
    # emux_log_string "dma shader data\n"
    # j DMAIn
    # move ra, ra2

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
    #define v___        $v29
    #define vconst      $v28
    #define vindices    $v27
    #define voffsets    $v26
    #define vvertices   $v25
    #define vtx1        a1
    #define vtx2        a2
    #define vtx3        a3
    #define trcode1     t6
    #define trcode2     t7
    #define trcode3     t8
    #define ra3         s8
    li t0, %lo(MAGMA_CONSTANTS)
    lqv vconst, 0,t0
    addi t1, rspq_dmem_buf_ptr, %lo(RSPQ_DMEM_BUFFER) - 3
    luv vindices, 0,t1
    li t0, %lo(MAGMA_VERTEX_CACHE) + MAGMA_VTX_XYZ
    mtc2 t0, vconst.e1
    lhu a0, %lo(MAGMA_TRI_CMD)
    vsrl vindices, vindices, 7
    vmudh voffsets, vindices, vconst.e0
    vaddc vvertices, voffsets, vconst.e1
    lb v0, %lo(MAGMA_CULL_MODE)
    mfc2 vtx1, vvertices.e0
    mfc2 vtx2, vvertices.e1
    # TODO: clipping
    li s3, %lo(RDPQ_CMD_STAGING)
    jal RDPQ_Triangle_Send_Async
    mfc2 vtx3, vvertices.e2
    jal_and_j RDPQ_Triangle_Send_End, RSPQ_Loop
    #undef v___
    #undef vconst
    #undef vindices
    #undef voffsets
    #undef vvertices
    #undef vtx1
    #undef vtx2
    #undef vtx3
    #undef trcode1
    #undef trcode2
    #undef trcode3
    #undef ra3
    .endfunc

#include "rsp_rdpq.inc"

    #############################################################
    # ARGS:
    #   a0: Bit 31..24: Command id
    #       Bit 23..0:  Vertex buffer offset
    #   a1: Bit 31..16: Cache offset
    #       Bit 15..0:  Vertex count
    #############################################################
MgCmd_LoadVertices:
    # We can't fall through here because RDPQ_Triangle_Send_Async etc. is always placed after the .text segment
    # and we therefore need to put the vertex shader into it's own segment to force it being placed at the end.
    # That means we can't register the command with the shader's address directly because it is not in the .text segment.
    # TODO: is there a better way?
    j _MG_VTX_SHADER
    nop
