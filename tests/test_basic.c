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

static void test_life_boost_extends_survival(void)
{
    /* Construct a 5x5 with two isolated alive cells. Without boost
     * Conway kills them in one tick (underpopulation). With boost,
     * they survive that tick. */
    ml_life *L = ml_life_new(5, 5);
    ASSERT(L != NULL);
    /* Use a high density seed then immediately overwrite to a known
     * sparse state via the seed; for our purposes any cell with no
     * 2..3 neighbors will die. The seed at 0.0 produces no cells. */
    ml_life_seed_random(L, 0.0, 1u);
    /* We can't directly poke cells through the public API, so instead
     * use the boost-from-grid path: any alive cell gets a boost when
     * the src grid has bright fg at that position. */
    ml_life_seed_random(L, 1.0, 1u);  /* every disc cell alive */
    int seeded = ml_life_alive_count(L);
    ASSERT(seeded == 5 * 5);

    /* Tick once: most cells die from overcrowding (n=8 → not in S23). */
    ml_life_tick(L);
    int after = ml_life_alive_count(L);
    ASSERT(after < seeded);

    /* Reseed full + boost everywhere. After tick, more cells should
     * survive than without boost. */
    ml_life_seed_random(L, 1.0, 1u);
    ml_grid *src = ml_grid_new(5, 5);
    ASSERT(src != NULL);
    /* Paint a bright fg on every src cell so the boost setter fires. */
    for (int i = 0; i < 25; ++i) {
        snprintf(src->cells[i].glyph, ML_MAX_GLYPH_BYTES, "%s", "*");
        src->cells[i].fg = (ml_rgb){0xff, 0xff, 0xff};
        src->cells[i].has_fg = true;
    }
    ml_life_boost_from_grid(L, src, 80.0, 3);
    ml_life_tick(L);
    int boosted = ml_life_alive_count(L);
    /* With +3 boost, no cell can die from rule yet — all 25 should
     * have survived this single tick. */
    ASSERT(boosted == seeded);

    ml_grid_free(src);
    ml_life_free(L);
}

static void test_life_fed_cells_use_absorbed_color(void)
{
    /* Build a 5×5 with all cells alive, paint a known fractal color
     * everywhere, feed, stamp into a different palette — confirm the
     * stamped fg matches the absorbed color, NOT the life palette. */
    ml_life *L = ml_life_new(5, 5);
    ASSERT(L != NULL);
    ml_life_seed_random(L, 1.0, 1u);

    ml_grid *src = ml_grid_new(5, 5);
    ASSERT(src != NULL);
    ml_rgb absorbed = {0x12, 0xab, 0xef};
    for (int i = 0; i < 25; ++i) {
        snprintf(src->cells[i].glyph, ML_MAX_GLYPH_BYTES, "%s", "*");
        src->cells[i].fg = absorbed;
        src->cells[i].has_fg = true;
    }
    ml_life_boost_from_grid(L, src, 80.0, 3);

    ml_grid *out = ml_grid_new(5, 5);
    ASSERT(out != NULL);
    /* Use a palette whose stops are all-zero so a "palette" rendering
     * would produce black (clearly distinguishable from absorbed). */
    ml_rgb black_pal[ML_PALETTE_STOPS] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},
                                          {0,0,0},{0,0,0},{0,0,0},{0,0,0}};
    ml_life_stamp(L, out, black_pal, true);

    /* Every alive cell in `out` must have fg == absorbed, not the
     * black palette. */
    int matched = 0;
    for (int i = 0; i < 25; ++i) {
        if (out->cells[i].glyph[0] == '\0') continue;
        ASSERT(out->cells[i].fg.r == absorbed.r);
        ASSERT(out->cells[i].fg.g == absorbed.g);
        ASSERT(out->cells[i].fg.b == absorbed.b);
        ++matched;
    }
    ASSERT(matched > 0);

    ml_grid_free(out);
    ml_grid_free(src);
    ml_life_free(L);
}

static void test_life_inheritance_through_birth(void)
{
    /* Seed densely, feed every alive cell with one absorbed color,
     * tick once, then confirm every alive cell — including newborns
     * — carries that color (newborns inherit by averaging parents). */
    ml_life *L = ml_life_new(8, 8);
    ASSERT(L != NULL);
    ml_life_seed_random(L, 0.50, 99u);

    ml_grid *src = ml_grid_new(8, 8);
    ASSERT(src != NULL);
    ml_rgb absorbed = {0x40, 0xc8, 0x70};
    for (int i = 0; i < 64; ++i) {
        snprintf(src->cells[i].glyph, ML_MAX_GLYPH_BYTES, "%s", "*");
        src->cells[i].fg = absorbed;
        src->cells[i].has_fg = true;
    }
    ml_life_boost_from_grid(L, src, 80.0, 3);

    int alive_before = ml_life_alive_count(L);
    ASSERT(alive_before > 0);

    ml_life_tick(L);
    int alive_after = ml_life_alive_count(L);
    ASSERT(alive_after > 0);

    /* Stamp into a black-palette grid; any alive cell that doesn't
     * carry the absorbed color (or some inherited average of it)
     * would render in black. Since all parents started with the
     * identical color, all newborns inherit exactly that color too. */
    ml_grid *out = ml_grid_new(8, 8);
    ASSERT(out != NULL);
    ml_rgb black_pal[ML_PALETTE_STOPS] = {{0,0,0},{0,0,0},{0,0,0},{0,0,0},
                                          {0,0,0},{0,0,0},{0,0,0},{0,0,0}};
    ml_life_stamp(L, out, black_pal, false);

    int unfed = 0;
    int matched = 0;
    for (int i = 0; i < 64; ++i) {
        if (out->cells[i].glyph[0] == '\0') continue;
        if (out->cells[i].fg.r == absorbed.r &&
            out->cells[i].fg.g == absorbed.g &&
            out->cells[i].fg.b == absorbed.b) {
            ++matched;
        } else {
            ++unfed;
        }
    }
    /* All survivors and newborns should carry the inherited color. */
    ASSERT(matched > 0);
    ASSERT(unfed == 0);

    ml_grid_free(out);
    ml_grid_free(src);
    ml_life_free(L);
}

static void test_fractal_types_all_render(void)
{
    /* Every fractal type must render some non-empty cells into a
     * small grid without crashing. Also exercises the type name
     * lookup. */
    ml_grid *g = ml_grid_new(40, 20);
    ASSERT(g != NULL);
    const ml_rgb *pal = ml_fractal_palette(ML_FP_OCEAN);
    for (int i = 0; i < ML_FRACTAL_TYPE_COUNT; ++i) {
        ml_grid_clear(g);
        ml_fractal_paint(g, 1.0, pal, (ml_fractal_type)i);
        int painted = 0;
        for (int j = 0; j < g->width * g->height; ++j) {
            if (g->cells[j].glyph[0] != '\0') ++painted;
        }
        ASSERT(painted > 0);
        /* Round-trip the type name. */
        const char *name = ml_fractal_type_name((ml_fractal_type)i);
        ASSERT(ml_fractal_type_from_name(name) == (ml_fractal_type)i);
    }
    /* Common aliases. */
    ASSERT(ml_fractal_type_from_name("burning_ship") == ML_FRACTAL_BURNING_SHIP);
    ASSERT(ml_fractal_type_from_name("mandelbar")    == ML_FRACTAL_TRICORN);
    ASSERT((int)ml_fractal_type_from_name("xyzzy")   < 0);
    ml_grid_free(g);
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
    test_life_boost_extends_survival();
    test_life_fed_cells_use_absorbed_color();
    test_life_inheritance_through_birth();
    test_fractal_types_all_render();
    if (failures) {
        printf("FAILED: %d failures\n", failures);
        return 1;
    }
    printf("OK: all tests passed\n");
    return 0;
}
