#ifndef VERTEX_H
#define VERTEX_H

#include <stdint.h>
#include <libdragon.h>

#define S10_5(v) ((v)*(1<<5))
#define S8_8(v) ((v)*(1<<8))
#define N8(v) ((v)*0x7F)

#define POS(x, y, z) { S10_5(x), S10_5(y), S10_5(z) }
#define TEX(s, t) { S8_8(s), S8_8(t) }
#define NRM(x, y, z) (((int)((x)*0xF)<<11) | ((int)((y)*0x1F)<<5) | ((int)((z)*0xF)))

#define VERTEX(x, y, z, s, t, nx, ny, nz, c) { .position = POS((x), (y), (z)), .texcoord = TEX((s), (t)), .packed_normal = NRM((nx), (ny), (nz)), .color = (c) }

#endif
