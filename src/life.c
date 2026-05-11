/* Conway-style life automaton with age tracking + decay fade.
 *
 * Each cell is a uint8_t age:
 *   0   = dead (no decay)
 *   1   = newborn this frame (B3: born from exactly 3 alive neighbors)
 *   2.. = surviving (S23: 2 or 3 alive neighbors), age++ per frame, cap 255
 *
 * A parallel uint8_t decay grid tracks recently-dead cells:
 *   0   = no decay
 *   >0  = decay ticks remaining (counts down each frame to 0)
 *
 * When a cell dies (alive > 0 → next = 0), decay is set to DECAY_MAX so
 * the renderer can show it as a brief cool-colored ghost before it
 * disappears entirely.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maxlife.h"

#define DECAY_MAX 6   /* frames a dead cell stays visible during fade */

struct ml_life {
    int       width;
    int       height;
    uint8_t  *cells;
    uint8_t  *next;
    uint8_t  *decay;
    uint8_t  *boost;     /* extra would-die ticks a cell can survive */
    uint8_t  *fed;       /* 1 if cell has ever eaten from the fractal */
    ml_rgb   *fed_color; /* color absorbed at the most recent feeding */
    int       initial_alive;
};

ml_life *ml_life_new(int width, int height)
{
    if (width <= 0 || height <= 0) return NULL;
    ml_life *L = (ml_life *)calloc(1, sizeof(*L));
    if (!L) return NULL;
    L->width = width;
    L->height = height;
    size_t n = (size_t)width * (size_t)height;
    L->cells     = (uint8_t *)calloc(n, 1);
    L->next      = (uint8_t *)calloc(n, 1);
    L->decay     = (uint8_t *)calloc(n, 1);
    L->boost     = (uint8_t *)calloc(n, 1);
    L->fed       = (uint8_t *)calloc(n, 1);
    L->fed_color = (ml_rgb  *)calloc(n, sizeof(ml_rgb));
    if (!L->cells || !L->next || !L->decay || !L->boost
        || !L->fed || !L->fed_color) {
        ml_life_free(L);
        return NULL;
    }
    return L;
}

void ml_life_free(ml_life *L)
{
    if (!L) return;
    free(L->cells);
    free(L->next);
    free(L->decay);
    free(L->boost);
    free(L->fed);
    free(L->fed_color);
    free(L);
}

void ml_life_seed_random(ml_life *L, double density, uint32_t seed)
{
    if (!L) return;
    if (density < 0.0) density = 0.0;
    if (density > 1.0) density = 1.0;
    int total = L->width * L->height;
    int alive = 0;
    for (int i = 0; i < total; ++i) {
        uint32_t h = seed ^ ((uint32_t)i * 2654435761u);
        h = (h ^ (h >> 16)) * 0x85ebca6bu;
        h = (h ^ (h >> 13)) * 0xc2b2ae35u;
        h =  h ^ (h >> 16);
        double r = (h & 0xFFFFFFu) / (double)0xFFFFFFu;
        if (r < density) {
            L->cells[i] = 1;
            ++alive;
        } else {
            L->cells[i] = 0;
        }
        L->decay[i] = 0;
    }
    L->initial_alive = alive;
}

int ml_life_tick(ml_life *L)
{
    if (!L) return 0;
    int w = L->width, h = L->height;
    int alive_next = 0;
    /* Decay countdown: applied before the next-state derivation so
     * decay slots from PREVIOUS frame fade by one. Cells that die
     * this tick reset decay to DECAY_MAX below. */
    for (int i = 0; i < w * h; ++i) {
        if (L->decay[i] > 0) --L->decay[i];
    }
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int n = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                int ny = y + dy;
                if (ny < 0 || ny >= h) continue;
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    if (nx < 0 || nx >= w) continue;
                    if (L->cells[ny * w + nx] > 0) ++n;
                }
            }
            int idx = y * w + x;
            uint8_t cur = L->cells[idx];
            uint8_t nv  = 0;
            if (cur > 0) {
                if (n == 2 || n == 3) {
                    /* Normal survival. */
                    nv = (cur < 255) ? (uint8_t)(cur + 1) : 255;
                } else if (L->boost[idx] > 0) {
                    /* Would-die override: spend one boost token,
                     * survive anyway, age++. */
                    --L->boost[idx];
                    nv = (cur < 255) ? (uint8_t)(cur + 1) : 255;
                } else {
                    /* Died — start decay fade, forget what it ate. */
                    L->decay[idx] = DECAY_MAX;
                    L->fed[idx]   = 0;
                }
            } else {
                if (n == 3) {
                    nv = 1;
                    L->decay[idx] = 0;
                    L->boost[idx] = 0;  /* newborn starts unboosted */
                    L->fed[idx]   = 0;  /* and unfed */
                }
            }
            L->next[idx] = nv;
            if (nv > 0) ++alive_next;
        }
    }
    uint8_t *tmp = L->cells;
    L->cells = L->next;
    L->next  = tmp;
    return alive_next;
}

void ml_life_boost_from_grid(ml_life *L, const ml_grid *src,
                             double min_lum, uint8_t amount)
{
    if (!L || !src) return;
    if (src->width != L->width || src->height != L->height) return;
    int total = L->width * L->height;
    for (int i = 0; i < total; ++i) {
        if (L->cells[i] == 0) continue;          /* dead — no boost */
        const ml_cell *c = &src->cells[i];
        if (!c->has_fg) continue;                /* untouched bg cell */
        double lum = 0.299 * c->fg.r + 0.587 * c->fg.g + 0.114 * c->fg.b;
        if (lum < min_lum) continue;
        if (L->boost[i] < amount) L->boost[i] = amount;
        /* Cell adopts the color it just ate; it'll render in this
         * color for the rest of its lifecycle (or until it eats
         * something else). */
        L->fed[i]       = 1;
        L->fed_color[i] = c->fg;
    }
}

int ml_life_alive_count(const ml_life *L)
{
    if (!L) return 0;
    int n = 0;
    int total = L->width * L->height;
    for (int i = 0; i < total; ++i) if (L->cells[i] > 0) ++n;
    return n;
}

int ml_life_initial_count(const ml_life *L)
{
    return L ? L->initial_alive : 0;
}

/* Glyph for life cells by age. Newborns are dense + bold; oldsters
 * fade to small marks. Decay cells use even smaller marks. */
static const char *LIFE_GLYPHS[] = {
    "●", "●", "◉", "◯", "○", "◌", "·", "˙",
};
#define LIFE_GLYPH_N (sizeof(LIFE_GLYPHS) / sizeof(LIFE_GLYPHS[0]))

static const char *DECAY_GLYPHS[] = { "˙", "·" };
#define DECAY_GLYPH_N (sizeof(DECAY_GLYPHS) / sizeof(DECAY_GLYPHS[0]))

/* Map life age → palette index. Palette is warm→cool: stop 7 is the
 * warmest (newborn), stop 1 is the coolest aging color. Stop 0 is
 * reserved for decay cells.
 *   age 1  → stop 7
 *   age 2  → stop 6
 *   age 3  → stop 5
 *   age 4-5 → stop 4
 *   age 6-8 → stop 3
 *   age 9-15 → stop 2
 *   age 16+ → stop 1
 */
static int age_to_palette(uint8_t age)
{
    if (age <= 1) return 7;
    if (age <= 2) return 6;
    if (age <= 3) return 5;
    if (age <= 5) return 4;
    if (age <= 8) return 3;
    if (age <= 15) return 2;
    return 1;
}

void ml_life_stamp(const ml_life *L, ml_grid *dst,
                   const ml_rgb palette[ML_PALETTE_STOPS],
                   bool with_decay)
{
    if (!L || !dst || !palette) return;
    if (dst->width != L->width || dst->height != L->height) return;
    int total = L->width * L->height;
    for (int i = 0; i < total; ++i) {
        uint8_t age = L->cells[i];
        if (age > 0) {
            ml_cell *c = &dst->cells[i];
            int gi = (age - 1 < (int)LIFE_GLYPH_N)
                ? (int)(age - 1)
                : (int)(LIFE_GLYPH_N - 1);
            snprintf(c->glyph, ML_MAX_GLYPH_BYTES, "%s", LIFE_GLYPHS[gi]);
            /* Fed cells render in the color they absorbed from the
             * fractal; unfed cells use the age-based warm→cool ramp. */
            if (L->fed[i]) {
                c->fg = L->fed_color[i];
            } else {
                int pi = age_to_palette(age);
                c->fg = palette[pi];
            }
            c->has_fg = true;
            c->has_bg = false;
            c->style = (age <= 2) ? ML_STYLE_BOLD : ML_STYLE_NONE;
            continue;
        }
        if (with_decay && L->decay[i] > 0) {
            ml_cell *c = &dst->cells[i];
            int gi = (L->decay[i] >= 4) ? 0 : 1;
            snprintf(c->glyph, ML_MAX_GLYPH_BYTES, "%s", DECAY_GLYPHS[gi]);
            c->fg = palette[0]; /* coolest stop */
            c->has_fg = true;
            c->has_bg = false;
            c->style = ML_STYLE_DIM;
        }
    }
}
