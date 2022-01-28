
#include <stdint.h>
#include <malloc.h>
#include "rdp_commands.h"

void test_rdp_start(TestContext *ctx)
{
    //static volatile bool dp_done;
//
    //void dp_intr_handler()
    //{
    //    dp_done = true;
    //}
//
    //register_DP_handler(dp_intr_handler);
    //DEFER(unregister_DP_handler(dp_intr_handler));
    //set_DP_interrupt(1);
    //DEFER(set_DP_interrupt(0));

    uint32_t fbsize = 320 * 240 * 2;
    void *fb = memalign(64, fbsize);
    data_cache_hit_invalidate(fb, fbsize);

    static uint64_t buffer1[1024];
    uint64_t *ptr = buffer1 + 512;
    *(ptr++) = (0x3FULL << 56) | ((uint64_t)RDP_TILE_FORMAT_RGBA << 53) | ((uint64_t)RDP_TILE_SIZE_16BIT << 51) | (319ULL << 32) | PhysicalAddr(fb);
    *(ptr++) = (0x2DULL << 56) | (320 << 14) | (240 << 2);
    *(ptr++) = (0x2FULL << 56) | SOM_CYCLE_FILL;
    *(ptr++) = (0x37ULL << 56) | 0x00000000;
    *(ptr++) = (0x27ULL << 56);
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    *(ptr++) = (0x37ULL << 56) | 0x11111111;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    void *after1 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0x22222222;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    //void *after2 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0x33333333;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    //void *after3 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0x44444444;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    //void *after4 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0x55555555;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    //void *after5 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0x66666666;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    //void *after6 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0x77777777;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x27ULL << 56);
    void *after7 = ptr;
    *(ptr++) = (0x37ULL << 56) | 0xFFFFFFFF;
    *(ptr++) = (0x36ULL << 56) | (320ULL << 46) | (240ULL << 34);
    *(ptr++) = (0x29ULL << 56);

    static uint64_t buffer2[] = {
        (0x29ULL << 56)
    };

    data_cache_hit_writeback(buffer1, sizeof(buffer1));
    data_cache_hit_writeback(buffer2, sizeof(buffer2));

    debugf("buffer start: %08lX, buffer end: %08lX\n", PhysicalAddr(buffer1), PhysicalAddr(buffer1) + sizeof(buffer1));

    uint32_t status[8];
    uint32_t current[8];
    uint32_t start[8];
    uint32_t end[8];

    #define RECORD_STATUS(i) ({ \
        start[i] = ((volatile uint32_t *)0xA4100000)[0]; \
        end[i] = ((volatile uint32_t *)0xA4100000)[1]; \
        current[i] = ((volatile uint32_t *)0xA4100000)[2]; \
        status[i] = ((volatile uint32_t *)0xA4100000)[3]; \
    })

    //dp_done = 0;

    RECORD_STATUS(0);
    MEMORY_BARRIER();
    ((volatile uint32_t *)0xA4100000)[0] = PhysicalAddr(buffer1);
    MEMORY_BARRIER();
    RECORD_STATUS(1);

    RECORD_STATUS(2);
    MEMORY_BARRIER();
    ((volatile uint32_t *)0xA4100000)[1] = PhysicalAddr(after1);
    MEMORY_BARRIER();
    RECORD_STATUS(3);

    RECORD_STATUS(4);
    MEMORY_BARRIER();
    ((volatile uint32_t *)0xA4100000)[1] = PhysicalAddr(buffer1) + sizeof(buffer1);
    MEMORY_BARRIER();
    RECORD_STATUS(5);

    RECORD_STATUS(6);
    MEMORY_BARRIER();
    ((volatile uint32_t *)0xA4100000)[1] = PhysicalAddr(after7);
    MEMORY_BARRIER();
    RECORD_STATUS(7);


    //MEMORY_BARRIER();
    //while (!dp_done);
    //MEMORY_BARRIER();

    wait_ms(1000);

    #define LOG_STATUS(i, msg) debugf("%08lX %08lX %08lX %08lX %s\n", start[i], end[i], current[i], status[i], msg)

    LOG_STATUS(0, "before setting start\n");
    LOG_STATUS(1, "after setting start\n");
    LOG_STATUS(2, "before setting end (1)\n");
    LOG_STATUS(3, "after setting end (1)\n");
    LOG_STATUS(4, "before setting end (2)\n");
    LOG_STATUS(5, "after setting end (2)\n");
    LOG_STATUS(6, "before setting end (3)\n");
    LOG_STATUS(7, "after setting end (3)\n");

    for (uint32_t i = 0; i < 320 * 240; i++)
    {
        ASSERT_EQUAL_HEX(UncachedUShortAddr(fb)[i], 0x1111, "Framebuffer was not cleared properly! Index: %lu", i);
    }
}
