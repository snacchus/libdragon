#ifndef VERTEX_H
#define VERTEX_H

#include <stdint.h>
#include <libdragon.h>

#define VTX_TEX_SHIFT   8
#define RDP_TEX_SHIFT   5
#define TEX_SIZE_SHIFT  (VTX_TEX_SHIFT-RDP_TEX_SHIFT)
#define RDP_HALF_TEXEL  (1<<(RDP_TEX_SHIFT-1))

#define S10_5(v) ((v)*(1<<5))
#define S8_8(v) ((v)*(1<<VTX_TEX_SHIFT))
#define N8(v) ((v)*0x7F)

#define POS(x, y, z) { S10_5(x), S10_5(y), S10_5(z) }
#define TEX(s, t) { S8_8(s), S8_8(t) }
#define NRM(x, y, z) (((int)((x)*0xF)<<11) | ((int)((y)*0x1F)<<5) | ((int)((z)*0xF)))

#define VERTEX(x, y, z, s, t, nx, ny, nz, c) { .position = POS((x), (y), (z)), .texcoord = TEX((s), (t)), .packed_normal = NRM((nx), (ny), (nz)), .color = (c) }

#endif
