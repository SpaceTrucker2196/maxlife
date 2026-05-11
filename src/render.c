/* Cell grid → ANSI escape sequences with truecolor (24-bit) output. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maxlife.h"

ml_grid *ml_grid_new(int width, int height)
{
    if (width <= 0 || height <= 0) return NULL;
    ml_grid *g = (ml_grid *)calloc(1, sizeof(*g));
    if (!g) return NULL;
    g->width = width;
    g->height = height;
    g->cells = (ml_cell *)calloc((size_t)width * (size_t)height, sizeof(ml_cell));
    if (!g->cells) { free(g); return NULL; }
    return g;
}

void ml_grid_free(ml_grid *g)
{
    if (!g) return;
    free(g->cells);
    free(g);
}

void ml_grid_clear(ml_grid *g)
{
    if (!g) return;
    memset(g->cells, 0,
           (size_t)g->width * (size_t)g->height * sizeof(ml_cell));
}

static int append(char *buf, size_t bufsize, size_t pos, const char *s)
{
    if (!s) return 0;
    size_t len = strlen(s);
    if (pos + len + 1 > bufsize) return -1;
    memcpy(buf + pos, s, len);
    buf[pos + len] = '\0';
    return (int)len;
}

static size_t emit_sgr(char *buf, size_t bufsize, size_t pos,
                       const ml_cell *cell)
{
    char body[128];
    size_t n = 0;
    body[0] = '\0';
    if (cell->style & ML_STYLE_DIM) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s2", n ? ";" : "");
    }
    if (cell->style & ML_STYLE_BOLD) {
        n += (size_t)snprintf(body + n, sizeof(body) - n, "%s1", n ? ";" : "");
    }
    if (cell->has_fg) {
        n += (size_t)snprintf(body + n, sizeof(body) - n,
                              "%s38;2;%u;%u;%u",
                              n ? ";" : "",
                              (unsigned)cell->fg.r,
                              (unsigned)cell->fg.g,
                              (unsigned)cell->fg.b);
    }
    if (cell->has_bg) {
        n += (size_t)snprintf(body + n, sizeof(body) - n,
                              "%s48;2;%u;%u;%u",
                              n ? ";" : "",
                              (unsigned)cell->bg.r,
                              (unsigned)cell->bg.g,
                              (unsigned)cell->bg.b);
    }
    if (n == 0) {
        int r = append(buf, bufsize, pos, "\x1b[0m");
        return (r > 0) ? (size_t)r : 0;
    }
    char esc[160];
    int len = snprintf(esc, sizeof(esc), "\x1b[0;%sm", body);
    if (len < 0) return 0;
    int r = append(buf, bufsize, pos, esc);
    return (r > 0) ? (size_t)r : 0;
}

size_t ml_grid_to_ansi(const ml_grid *g, char *buf, size_t bufsize)
{
    if (!g || !buf || bufsize == 0) return 0;
    size_t pos = 0;
    buf[0] = '\0';

    for (int y = 0; y < g->height; ++y) {
        bool sgr_active = false;
        for (int x = 0; x < g->width; ++x) {
            const ml_cell *c = &g->cells[y * g->width + x];
            if (c->glyph[0] == '\0') {
                if (sgr_active) {
                    int r = append(buf, bufsize, pos, "\x1b[0m");
                    if (r < 0) return pos;
                    pos += (size_t)r;
                    sgr_active = false;
                }
                int r = append(buf, bufsize, pos, " ");
                if (r < 0) return pos;
                pos += (size_t)r;
                continue;
            }
            pos += emit_sgr(buf, bufsize, pos, c);
            sgr_active = true;
            int r = append(buf, bufsize, pos, c->glyph);
            if (r < 0) return pos;
            pos += (size_t)r;
        }
        if (sgr_active) {
            int r = append(buf, bufsize, pos, "\x1b[0m");
            if (r > 0) pos += (size_t)r;
        }
        if (y < g->height - 1) {
            int r = append(buf, bufsize, pos, "\r\n");
            if (r < 0) return pos;
            pos += (size_t)r;
        }
    }
    return pos;
}
