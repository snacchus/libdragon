#include <rsp_queue.inc>

    .data

	RSPQ_BeginOverlayHeader
        RSPQ_DefineCommand MgCmd_PushConstant,  8
        RSPQ_DefineCommand MgCmd_LoadVertices,  8
        RSPQ_DefineCommand MgCmd_DrawTriangles, 8
	RSPQ_EndOverlayHeader

    RSPQ_EmptySavedState

    .text

    #############################################################
    # MgCmd_PushConstant
    #############################################################
    .func MgCmd_PushConstant
MgCmd_PushConstant:
    jr ra
    nop
    .endfunc

    #############################################################
    # MgCmd_LoadVertices
    #############################################################
    .func MgCmd_LoadVertices
MgCmd_LoadVertices:
    jr ra
    nop
    .endfunc

    #############################################################
    # MgCmd_DrawTriangles
    #############################################################
    .func MgCmd_DrawTriangles
MgCmd_DrawTriangles:
    jr ra
    nop
    .endfunc
