/* maxlife — Conway's Life with warm/cool age coloring for the terminal.
 *
 * Pure C99 + libc + POSIX (termios.h, sys/ioctl.h). No third-party deps,
 * no images, no audio — just cells that are born, age, and die, rendered
 * with truecolor ANSI on an optional fractal background.
 */

#ifndef MAXLIFE_H
#define MAXLIFE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ML_MAX_GLYPH_BYTES 8
#define ML_PALETTE_STOPS   8

/* ---------- color ---------------------------------------------------- */

typedef struct { uint8_t r, g, b; } ml_rgb;

typedef enum {
    ML_STYLE_NONE = 0,
    ML_STYLE_DIM  = 1u << 0,
    ML_STYLE_BOLD = 1u << 1,
} ml_style_bits;

typedef struct {
    char    glyph[ML_MAX_GLYPH_BYTES];
    ml_rgb  fg;
    ml_rgb  bg;
    bool    has_fg;
    bool    has_bg;
    uint8_t style;
} ml_cell;

typedef struct {
    int      width;
    int      height;
    ml_cell *cells;
} ml_grid;

ml_grid *ml_grid_new(int width, int height);
void     ml_grid_free(ml_grid *g);
void     ml_grid_clear(ml_grid *g);

/* ---------- palettes ----------------------------------------------- */

/* Life palettes go warm (stop 7) → cool (stop 0). Newborn cells use
 * the warm end, aging cells slide toward cool, decay cells use the
 * coolest stops. */
typedef enum {
    ML_PAL_THERMAL = 0,   /* white → yellow → orange → red → magenta → indigo */
    ML_PAL_EMBERS,        /* coal: gold → orange → red → dark red → black */
    ML_PAL_PHOENIX,       /* saturated fire palette */
    ML_PAL_AURORA,        /* green → cyan → blue → violet */
    ML_PAL_FROST,         /* white → cyan → blue → indigo */
    ML_PAL_COUNT,
} ml_palette_id;

const ml_rgb *ml_palette_named(ml_palette_id id);
ml_palette_id ml_palette_id_from_name(const char *name);
const char   *ml_palette_name(ml_palette_id id);

/* Fractal background palettes — separate set, go dark → bright (the
 * usual Julia-set convention). */
typedef enum {
    ML_FP_AURORA = 0, ML_FP_EMBER, ML_FP_OCEAN, ML_FP_FOREST,
    ML_FP_SAKURA, ML_FP_TWILIGHT, ML_FP_LAVA, ML_FP_CORAL,
    ML_FP_COUNT,
} ml_fractal_palette_id;

const ml_rgb *ml_fractal_palette(ml_fractal_palette_id id);
ml_fractal_palette_id ml_fractal_palette_from_name(const char *name);
const char   *ml_fractal_palette_name(ml_fractal_palette_id id);

/* ---------- life automaton ----------------------------------------- */

typedef struct ml_life ml_life;

ml_life *ml_life_new(int width, int height);
void     ml_life_free(ml_life *L);

/* Seed alive-state with a deterministic random pattern at the given
 * density (0..1). Clears any existing state. */
void ml_life_seed_random(ml_life *L, double density, uint32_t seed);

/* Advance one Conway B3/S23 generation. Survivors age; cells that just
 * died are marked for decay (rendered cool for a few frames). Returns
 * the new alive count. */
int  ml_life_tick(ml_life *L);

int  ml_life_alive_count(const ml_life *L);
int  ml_life_initial_count(const ml_life *L);

/* Stamp life cells onto `dst` (same dimensions as the life grid).
 * Newborns use palette stop 7 (warmest), aging cells slide toward
 * stop 1, decay cells use stop 0 (coolest). When `with_decay` is
 * false, dying cells vanish immediately. */
void ml_life_stamp(const ml_life *L, ml_grid *dst,
                   const ml_rgb palette[ML_PALETTE_STOPS],
                   bool with_decay);

/* ---------- fractal background ------------------------------------- */

/* Paint a slowly-drifting Julia field into `g` using the given 8-stop
 * palette. `t` advances the c parameter. No symmetry folding — this
 * is just an organic backdrop. */
void ml_fractal_paint(ml_grid *g, double t,
                      const ml_rgb palette[ML_PALETTE_STOPS]);

/* ---------- render ------------------------------------------------- */

size_t ml_grid_to_ansi(const ml_grid *g, char *buf, size_t bufsize);

/* ---------- terminal ----------------------------------------------- */

bool ml_term_init(void);
void ml_term_restore(void);
bool ml_term_size_get(int *w, int *h);
bool ml_term_quit_pressed(void);
void ml_term_enter_alt_screen(void);
void ml_term_exit_alt_screen(void);
void ml_term_hide_cursor(void);
void ml_term_show_cursor(void);
void ml_term_home(void);
void ml_term_clear_screen(void);

/* ---------- options + runner --------------------------------------- */

typedef struct {
    int    width;              /* 0 = derive from terminal */
    int    height;             /* 0 = derive from terminal */
    double density;            /* initial seed density, default 0.18 */
    double fps;                /* frames per second, default 24 */
    bool   fractal;            /* draw Julia background under life cells */
    ml_palette_id         life_palette;
    ml_fractal_palette_id fractal_palette;
    bool   decay;              /* show recently-dead cells fading */
    uint32_t seed;             /* 0 = derive from time */
} ml_options;

void ml_options_default(ml_options *opt);
int  ml_run(const ml_options *opt);

#ifdef __cplusplus
}
#endif

#endif /* MAXLIFE_H */
