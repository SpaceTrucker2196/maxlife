/* Simple drifting Julia-set background.
 *
 * Unlike haikala's fractal pass, there's no dihedral folding or radial
 * symmetry — maxlife is rectangular and life cells provide the visual
 * focus, so the background is just a flowing organic field.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "maxlife.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const char *FRACTAL_GLYPHS[] = {
    " ", " ", "·", "˙", "░", "░", "▒", "▓",
};
#define FRACTAL_MAX_ITER 7

void ml_fractal_paint(ml_grid *g, double t,
                      const ml_rgb palette[ML_PALETTE_STOPS])
{
    if (!g || !palette) return;
    int width = g->width, height = g->height;
    int cx = width / 2, cy = height / 2;
    /* Map screen coordinates so the disc-ish view fits the largest
     * dimension. x is scaled 2× because terminal cells are roughly
     * 2:1 (height:width). */
    double scale_x = 3.0 / (double)width;
    double scale_y = 3.0 / (double)height;
    double drift = 0.05 * t;
    double cre = 0.7885 * cos(drift);
    double cim = 0.7885 * sin(drift);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double zr = (x - cx) * scale_x;
            double zi = (y - cy) * scale_y * 2.0; /* aspect correct */
            int i = 0;
            while (i < FRACTAL_MAX_ITER && (zr*zr + zi*zi) < 4.0) {
                double nzr = zr*zr - zi*zi + cre;
                double nzi = 2.0 * zr * zi + cim;
                zr = nzr; zi = nzi;
                ++i;
            }
            const char *ch = FRACTAL_GLYPHS[i];
            if (ch[0] == ' ' && ch[1] == '\0') {
                ml_cell *c = &g->cells[y * width + x];
                c->glyph[0] = '\0';
                c->has_fg = false;
                c->has_bg = false;
                c->style = 0;
                continue;
            }
            ml_cell *c = &g->cells[y * width + x];
            snprintf(c->glyph, ML_MAX_GLYPH_BYTES, "%s", ch);
            c->fg = palette[i];
            c->has_fg = true;
            c->has_bg = false;
            c->style = 0;
        }
    }
}
