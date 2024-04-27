#include <rsp_queue.inc>
#include "magma_constants.h"

    .data

	RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MgCmd_SetByte,       8
        RSPQ_DefineCommand MgCmd_SetShort,      8
        RSPQ_DefineCommand MgCmd_SetWord,       8
        RSPQ_DefineCommand MgCmd_SetQuad,       20
        RSPQ_DefineCommand MgCmd_LoadVertices,  8
        RSPQ_DefineCommand MgCmd_DrawIndices,   4
	RSPQ_EndOverlayHeader

    .align 4
    .ascii "Dragon RSP Magma"
    .ascii "Code by snacchus"

    RSPQ_BeginSavedState

MAGMA_STATE:
    MAGMA_VIEWPORT:
        MAGMA_VIEWPORT_SCALE:   .half   0,0,0,0
        MAGMA_VIEWPORT_OFFSET:  .half   0,0,0,0
    MAGMA_VERTEX_BUFFER:        .word   0    
    MAGMA_VERTEX_STRIDE:        .half   0
    MAGMA_TRI_CMD:              .half   0
    MAGMA_CULL_MODE:            .byte   0
    MAGMA_PRIM_TOPOLOGY:        .byte   0
    
    .align 3
    MAGMA_VERTEX_CACHE:         .dcb.b  MAGMA_VERTEX_CACHE_COUNT * MAGMA_VERTEX_SIZE
    MAGMA_VERTEX_CACHE_END:

    .align 4
    MAGMA_SHADER_STATE:         .dcb.b  MAGMA_MAX_SHADER_STATE_SIZE

    RSPQ_EndSavedState

    .align 4
MAGMA_CONSTANTS:                .half   MAGMA_VERTEX_SIZE, 0, 0, 0, 0, 0, 0, 0

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
    jr ra
    sw a1, %lo(MAGMA_STATE)(a0)
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
    li t0, %lo(MAGMA_VERTEX_CACHE)
    mtc2 t0, vconst.e1
    lhu a0, %lo(MAGMA_TRI_CMD)
    vmudh voffsets, vindices, vconst.e0
    vaddc vvertices, voffsets, vconst.e1
    lb v0, %lo(MAGMA_CULL_MODE)
    mfc2 vtx1, vvertices.e0
    mfc2 vtx2, vvertices.e1
    mfc2 vtx3, vvertices.e2

    lb trcode1, MAGMA_VTX_TR_CODE(vtx1)
    lb trcode2, MAGMA_VTX_TR_CODE(vtx2)
    lb trcode3, MAGMA_VTX_TR_CODE(vtx3)
    and t0, trcode1, trcode2
    and t0, trcode3
    andi t0, 0x3F
    beqz t0, JrRa
    nop

    # TODO: clipping
    j RDPQ_Triangle
    nop
    #undef v___
    #undef vconst
    #undef vindices
    #undef voffsets
    #undef vvertices
    .endfunc

#include "rsp_rdpq.inc"

    .text

    #############################################################
    # MgCmd_LoadVertices
    # 
    # ARGS:
    #   a0: Bit 31..24: Command id
    #       Bit 15..0:  Vertex count
    #   a1: Bit 31..24: Cache offset
    #       Bit 23..0:  Vertex buffer offset
    #############################################################
    .func MgCmd_LoadVertices
MgCmd_LoadVertices:
    lw s0, %lo(MAGMA_VERTEX_BUFFER)
    .align 3
magma_vertex_loader:
    .ds.l MAGMA_MAX_VERTEX_LOADER_SIZE
    .align 3
    jr ra
    nop
    .endfunc
