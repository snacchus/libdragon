#ifndef __LIBDRAGON_MGFX_MACROS_H
#define __LIBDRAGON_MGFX_MACROS_H

#define MGFX_S10_5(v)       ((v)*(1<<5))
#define MGFX_S8_8(v)        ((v)*(1<<8))

#define MGFX_POS(x, y, z)   { MGFX_S10_5(x), MGFX_S10_5(y), MGFX_S10_5(z) }
#define MGFX_TEX(s, t)      { MGFX_S8_8(s), MGFX_S8_8(t) }
#define MGFX_NRM(x, y, z)   ((((x) & 0x1F)<<11) | \
                             (((y) & 0x3F)<<5)  | \
                             (((z) & 0x1F)<<0))

#endif
