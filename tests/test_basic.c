/* Smoke tests for maxlife — no terminal interaction. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maxlife.h"

#define ASSERT(cond) do {                               \
    if (!(cond)) {                                      \
        fprintf(stderr, "FAIL %s:%d: %s\n",             \
                __FILE__, __LINE__, #cond);             \
        ++failures;                                     \
    }                                                   \
} while (0)

static int failures = 0;

static void test_palette_lookup(void)
{
    ASSERT(ml_palette_id_from_name("thermal") == ML_PAL_THERMAL);
    ASSERT(ml_palette_id_from_name("aurora")  == ML_PAL_AURORA);
    ASSERT((int)ml_palette_id_from_name("xyzzy") < 0);
    /* Thermal goes warm→cool: stop 7 brighter than stop 0. */
    const ml_rgb *p = ml_palette_named(ML_PAL_THERMAL);
    ASSERT(p[7].r > p[0].r);
}

static void test_fractal_palette_lookup(void)
{
    ASSERT(ml_fractal_palette_from_name("aurora") == ML_FP_AURORA);
    ASSERT(ml_fractal_palette_from_name("ocean")  == ML_FP_OCEAN);
    ASSERT((int)ml_fractal_palette_from_name("xyzzy") < 0);
}

static void test_life_seed_and_tick(void)
{
    ml_life *L = ml_life_new(40, 20);
    ASSERT(L != NULL);
    ml_life_seed_random(L, 0.20, 12345u);
    int seeded = ml_life_alive_count(L);
    ASSERT(seeded > 50);
    ASSERT(seeded < 250);

    /* Tick a few times — count must be non-negative and ≤ total cells. */
    int alive = 0;
    for (int i = 0; i < 5; ++i) {
        alive = ml_life_tick(L);
        ASSERT(alive >= 0);
        ASSERT(alive <= 40 * 20);
    }
    (void)alive;
    ml_life_free(L);
}

static void test_life_decay_after_death(void)
{
    /* Build a 3-cell row that becomes a blinker — but enclose it so
     * cells die. Easier: a single lone cell dies immediately. After
     * tick, decay should be set at that position. */
    ml_life *L = ml_life_new(10, 10);
    ASSERT(L != NULL);
    /* Manually mark one cell alive via seed_random with extreme seed
     * — instead, use a dense seed and tick once; many cells will die
     * and have decay slots. */
    ml_life_seed_random(L, 0.50, 7u);  /* dense — many overcrowding deaths */
    ml_life_tick(L);
    /* Stamp into a grid; with_decay=true must produce SOME cells
     * even if the population is small (decay glyphs from dying cells). */
    ml_grid *g = ml_grid_new(10, 10);
    ASSERT(g != NULL);
    const ml_rgb *pal = ml_palette_named(ML_PAL_THERMAL);
    ml_life_stamp(L, g, pal, true);
    int painted = 0;
    for (int i = 0; i < 100; ++i) {
        if (g->cells[i].glyph[0] != '\0') ++painted;
    }
    ASSERT(painted > 0);
    ml_grid_free(g);
    ml_life_free(L);
}

static void test_grid_to_ansi_runs(void)
{
    ml_grid *g = ml_grid_new(10, 3);
    ASSERT(g != NULL);
    /* Put one colored cell. */
    snprintf(g->cells[5].glyph, ML_MAX_GLYPH_BYTES, "%s", "X");
    g->cells[5].fg = (ml_rgb){0xff, 0xc0, 0x40};
    g->cells[5].has_fg = true;
    char buf[2048];
    size_t n = ml_grid_to_ansi(g, buf, sizeof(buf));
    ASSERT(n > 0);
    ASSERT(strstr(buf, "X") != NULL);
    ASSERT(strstr(buf, "38;2;255;192;64") != NULL);
    ml_grid_free(g);
}

int main(void)
{
    test_palette_lookup();
    test_fractal_palette_lookup();
    test_life_seed_and_tick();
    test_life_decay_after_death();
    test_grid_to_ansi_runs();
    if (failures) {
        printf("FAILED: %d failures\n", failures);
        return 1;
    }
    printf("OK: all tests passed\n");
    return 0;
}
