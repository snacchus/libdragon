#include <magma.h>
#include "../src/rspq/rspq_internal.h"

#define RSPQ_INIT() \
    rspq_init(); DEFER(rspq_close());

#define MG_INIT() \
    RSPQ_INIT(); \
    mg_init(); DEFER(mg_close());

void assert_block_contents(const uint32_t *expected_commands, uint32_t expected_commands_count, const rspq_block_t *block, TestContext *ctx) 
{
    const uint32_t *current_cmd = block->cmds;
    for (size_t i = 0; i < expected_commands_count; i++)
    {
        uint32_t expected_cmd = expected_commands[i];
        uint32_t actual_cmd = *(current_cmd++);
        ASSERT_EQUAL_HEX(actual_cmd, expected_cmd, "unexpected block content at word %d", i);

        // Check if we need to jump to the next buffer
        uint32_t cmd = *current_cmd;
        if ((cmd>>24) == RSPQ_CMD_JUMP) {
            current_cmd = (const uint32_t*)UncachedAddr(0x80000000 | (cmd & 0xFFFFFF));
        }
    }

    uint32_t last_cmd = *current_cmd;
    const uint32_t return_cmd = RSPQ_CMD_RET<<24;
    ASSERT_EQUAL_HEX(last_cmd, return_cmd, "block is not %ld words long", expected_commands_count);
}

void assert_draw_indexed(const uint32_t *expected_commands, uint32_t expected_commands_count, const uint16_t *indices, uint32_t count, uint32_t offset, TestContext *ctx)
{
    rspq_block_begin();
        mg_draw_indexed(NULL, indices, count, offset);
    rspq_block_t *block = rspq_block_end();

    assert_block_contents(expected_commands, expected_commands_count, block, ctx);
}

#define VTX(cnt, off, buf) (mg_overlay_id | (MG_CMD_LOAD_VERTICES<<24) | (cnt)), ((off<<24) | (buf))
#define TRI(i0, i1, i2) (mg_overlay_id | (MG_CMD_DRAW_INDICES<<24) | ((i0)<<16) | ((i1)<<8) | ((i2)<<0))

void test_mg_draw_indexed_one_tri(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        0, 1, 2
    };

    const uint32_t expected_commands[] = {
        VTX(3, 0, 0),
        TRI(0, 1, 2),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_two_tris(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        0, 1, 2, 3, 2, 1
    };

    const uint32_t expected_commands[] = {
        VTX(4, 0, 0),
        TRI(0, 1, 2),
        TRI(3, 2, 1),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_full_cache(TestContext *ctx)
{
    MG_INIT();

    const size_t count = 36;
    uint16_t indices[count];
    for (size_t i = 0; i < count; i++) indices[i] = i;

    const uint32_t expected_commands[] = {
        VTX(30, 0, 0),
        TRI(0, 1, 2),
        TRI(3, 4, 5),
        TRI(6, 7, 8),
        TRI(9, 10, 11),
        TRI(12, 13, 14),
        TRI(15, 16, 17),
        TRI(18, 19, 20),
        TRI(21, 22, 23),
        TRI(24, 25, 26),
        TRI(27, 28, 29),
        VTX(6, 0, 0),
        TRI(0, 1, 2),
        TRI(3, 4, 5),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_fragmented_batch(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        0, 1, 2, 41, 42, 43
    };

    const uint32_t expected_commands[] = {
        VTX(3, 0, 0),
        VTX(3, 3, 41),
        TRI(0, 1, 2),
        TRI(3, 4, 5),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_frag_backwards(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        41, 42, 43, 0, 1, 2
    };

    const uint32_t expected_commands[] = {
        VTX(3, 0, 41),
        VTX(3, 3, 0),
        TRI(0, 1, 2),
        TRI(3, 4, 5),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_holes(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        0, 4, 15
    };

    const uint32_t expected_commands[] = {
        VTX(3, 0, 0),
        VTX(3, 1, 4),
        VTX(3, 2, 15),
        TRI(0, 1, 2),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_out_of_order(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        2, 0, 1
    };

    const uint32_t expected_commands[] = {
        VTX(3, 0, 0),
        TRI(2, 0, 1),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}

void test_mg_draw_indexed_coalescing(TestContext *ctx)
{
    MG_INIT();

    const uint16_t indices[] = {
        5, 0, 3, 1, 4, 2
    };

    const uint32_t expected_commands[] = {
        VTX(6, 0, 0),
        TRI(5, 0, 3),
        TRI(1, 4, 2),
    };

    assert_draw_indexed(expected_commands, sizeof(expected_commands)/sizeof(expected_commands[0]), indices, sizeof(indices)/sizeof(indices[0]), 0, ctx);
}
