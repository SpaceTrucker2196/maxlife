/* Fractal background painter.
 *
 * All variants share:
 *   - A drifting parameter c = 0.7885 · (cos(0.05t) + i·sin(0.05t))
 *     so the field shifts slowly each frame.
 *   - The same iteration limit and bailout (|z|² > 4).
 *   - The same glyph repertoire indexed by escape count.
 *
 * What differs is the per-step recurrence:
 *   JULIA           : z = z² + c
 *   BURNING_SHIP    : z = (|Re(z)| + i|Im(z)|)² + c
 *   TRICORN         : z = conj(z)² + c
 *   MULTIBROT3      : z = z³ + c
 *   PHOENIX         : z = z² + c + p·z_{n-1}  (p = 0.5667, classic)
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

static const char *TYPE_NAMES[ML_FRACTAL_TYPE_COUNT] = {
    "julia", "ship", "tricorn", "multibrot3", "phoenix",
};

const char *ml_fractal_type_name(ml_fractal_type t)
{
    if (t < 0 || t >= ML_FRACTAL_TYPE_COUNT) return "?";
    return TYPE_NAMES[t];
}

ml_fractal_type ml_fractal_type_from_name(const char *name)
{
    if (!name) return (ml_fractal_type)-1;
    for (int i = 0; i < ML_FRACTAL_TYPE_COUNT; ++i) {
        if (strcmp(name, TYPE_NAMES[i]) == 0) return (ml_fractal_type)i;
    }
    /* Common alias. */
    if (strcmp(name, "burning_ship") == 0) return ML_FRACTAL_BURNING_SHIP;
    if (strcmp(name, "mandelbar") == 0)    return ML_FRACTAL_TRICORN;
    if (strcmp(name, "z3") == 0)            return ML_FRACTAL_MULTIBROT3;
    return (ml_fractal_type)-1;
}

void ml_fractal_paint(ml_grid *g, double t,
                      const ml_rgb palette[ML_PALETTE_STOPS],
                      ml_fractal_type type)
{
    if (!g || !palette) return;
    int width = g->width, height = g->height;
    int cx = width / 2, cy = height / 2;
    double scale_x = 3.0 / (double)width;
    double scale_y = 3.0 / (double)height;
    double drift = 0.05 * t;
    double cre = 0.7885 * cos(drift);
    double cim = 0.7885 * sin(drift);
    /* Phoenix companion parameter — controls the strength of the
     * z_{n-1} memory term. 0.5667 is the classic value. */
    const double phoenix_p = 0.5667;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double zr = (x - cx) * scale_x;
            double zi = (y - cy) * scale_y * 2.0; /* aspect correct */
            double pr = 0.0, pi = 0.0;            /* previous z, phoenix only */
            int i = 0;
            while (i < FRACTAL_MAX_ITER && (zr*zr + zi*zi) < 4.0) {
                double nzr, nzi;
                switch (type) {
                case ML_FRACTAL_BURNING_SHIP: {
                    double ar = fabs(zr);
                    double ai = fabs(zi);
                    nzr = ar*ar - ai*ai + cre;
                    nzi = 2.0 * ar * ai + cim;
                    break;
                }
                case ML_FRACTAL_TRICORN: {
                    /* conj(z)² + c: imaginary part flips sign in the
                     * cross term, so we just negate the standard
                     * Julia imaginary part. */
                    nzr = zr*zr - zi*zi + cre;
                    nzi = -2.0 * zr * zi + cim;
                    break;
                }
                case ML_FRACTAL_MULTIBROT3: {
                    /* (a + bi)³ = (a³ − 3ab²) + (3a²b − b³)i */
                    nzr = zr*zr*zr - 3.0*zr*zi*zi + cre;
                    nzi = 3.0*zr*zr*zi - zi*zi*zi + cim;
                    break;
                }
                case ML_FRACTAL_PHOENIX: {
                    double tr = zr*zr - zi*zi + cre + phoenix_p * pr;
                    double ti = 2.0*zr*zi          + cim + phoenix_p * pi;
                    pr = zr; pi = zi;
                    nzr = tr; nzi = ti;
                    break;
                }
                case ML_FRACTAL_JULIA:
                default:
                    nzr = zr*zr - zi*zi + cre;
                    nzi = 2.0 * zr * zi + cim;
                    break;
                }
                zr = nzr; zi = nzi;
                ++i;
            }

            const char *ch = FRACTAL_GLYPHS[i];
            ml_cell *c = &g->cells[y * width + x];
            if (ch[0] == ' ' && ch[1] == '\0') {
                c->glyph[0] = '\0';
                c->has_fg = false;
                c->has_bg = false;
                c->style = 0;
                continue;
            }
            snprintf(c->glyph, ML_MAX_GLYPH_BYTES, "%s", ch);
            c->fg = palette[i];
            c->has_fg = true;
            c->has_bg = false;
            c->style = 0;
        }
    }
}
