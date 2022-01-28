#include "gfx_internal.h"
#include "../rspq/rsp_queue.S"

    .section .data.overlay

    # Each overlay requires a header. The first two arguments to overlayHeader are start and end of the area in DMEM that should
    # be saved when the overlay is swapped out. Put your persistent state here.
    # The last argument is the command base (The id of the first command in the overlay).
    RSPQ_OverlayHeader GFX_STATE_START, GFX_STATE_END, 0x20

    .align 1
    # The command table maps the command ids to command descriptors, which are the IMEM address and the size
    # of the command encoded as a 16 bit value.
    # The first argument of RSPQ_DefineCommand is just the text label of the command, the second is the command size in bytes.
COMMAND_TABLE:
    RSPQ_DefineCommand GFXCmd_SetOtherModes,     8  # 0x20
    RSPQ_DefineCommand GFXCmd_ModifyOtherModes, 12  # 0x21

    .align 3
    # Everything between GFX_STATE_START and GFX_STATE_END is persistent state that is automatically saved by the overlay system.
    # Should be kept consistent with gfx_state_t in gfx_internal.h
GFX_STATE_START:
RDP_DMEM_BUFFER:       .ds.b GFX_RDP_DMEM_BUFFER_SIZE
RDP_OTHER_MODES:       .quad  0
GFX_STATE_END:

    .section .bss.overlay

    .text 1

    #############################################################
    # GFXCmd_SetOtherModes
    #
    # Completely ovewrites the internal cache of the RDP other modes with the 
    # values provided in a0 and a1 and sends it to the RDP.
    # The uppermost byte is always set to 0x2F.
    #
    # ARGS:
    #   a0: Command id and upper word of other modes
    #   a1: Lower word of other modes
    #############################################################
    .func GFXCmd_SetOtherModes
GFXCmd_SetOtherModes:
    sw a0, %lo(RDP_OTHER_MODES) + 0x0
    sw a1, %lo(RDP_OTHER_MODES) + 0x4
    li t0, 0x2F
    j GFX_SendOtherModes
    sb t0, %lo(RDP_OTHER_MODES)
    .endfunc

    #############################################################
    # GFXCmd_ModifyOtherModes
    #
    # Modifies a specific part of the other modes and sends the updated value to the RDP.
    # NOTE: The new value in a2 should never have bits set outside of
    #       the inverse bitmask to ensure correct results.
    #
    # ARGS:
    #   a0: Command id and word offset into other modes (0 or 4)
    #   a1: Inverse bit mask of the value to be written
    #   a2: New value
    #############################################################
    .func GFXCmd_ModifyOtherModes
GFXCmd_ModifyOtherModes:
    lw t0, %lo(RDP_OTHER_MODES)(a0)
    and t0, a1
    or t0, a2
    sw t0, %lo(RDP_OTHER_MODES)(a0)
    .endfunc

    #############################################################
    # GFX_SendOtherModes
    #
    # Sends the value in RDP_OTHER_MODES to the RDP.
    #
    #############################################################
    .func GFX_SendOtherModes
GFX_SendOtherModes:
    li s4, %lo(RDP_OTHER_MODES)
    li t0, 8
    jal_and_j RSPQ_RdpSendDynamic, RSPQ_Loop
    .endfunc
