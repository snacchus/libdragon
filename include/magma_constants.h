#ifndef __MAGMA_CONSTANTS
#define __MAGMA_CONSTANTS

#define MAGMA_PUSH_CONSTANT_HEADER      12
#define MAGMA_MAX_UNIFORM_PAYLOAD_SIZE  (RSPQ_MAX_COMMAND_SIZE*4 - MAGMA_PUSH_CONSTANT_HEADER)

#define MAGMA_VERTEX_CACHE_COUNT        32

#define MAGMA_DEFAULT_GUARD_BAND        2

#define MAGMA_CLIP_PLANE_COUNT          6
#define MAGMA_CLIP_PLANE_SIZE           8
#define MAGMA_CLIP_CACHE_SIZE           9

#define MAGMA_VTX_XYZ                   0
#define MAGMA_VTX_CLIP_CODE             6
#define MAGMA_VTX_TR_CODE               7
#define MAGMA_VTX_RGBA                  8
#define MAGMA_VTX_ST                    12
#define MAGMA_VTX_Wi                    16
#define MAGMA_VTX_Wf                    18
#define MAGMA_VTX_INVWi                 20
#define MAGMA_VTX_INVWf                 22
#define MAGMA_VTX_CS_POSi               24
#define MAGMA_VTX_CS_POSf               32
#define MAGMA_VTX_SIZE                  40

#endif
