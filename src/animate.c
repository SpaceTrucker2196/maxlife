/* Animation loop. Each frame: tick life, render fractal (optional),
 * stamp life on top, emit ANSI. No breathing, no hue cycling — only
 * the cell age drives color, via the warm→cool life palette. */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "maxlife.h"

void ml_options_default(ml_options *opt)
{
    if (!opt) return;
    opt->width = 0;
    opt->height = 0;
    opt->density = 0.18;
    opt->fps = 24.0;
    opt->fractal = false;
    opt->life_palette = ML_PAL_THERMAL;
    opt->fractal_palette = ML_FP_OCEAN;
    opt->decay = true;
    opt->seed = 0;
}

static double monotonic_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void sleep_for(double seconds)
{
    if (seconds <= 0) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
}

int ml_run(const ml_options *opt)
{
    if (!opt) return 1;

    int w = opt->width;
    int h = opt->height;
    if (w <= 0 || h <= 0) {
        ml_term_size_get(&w, &h);
    }
    /* Reserve one bottom line for the cursor/exit prompt. */
    int gw = w;
    int gh = h - 1;
    if (gw < 4) gw = 4;
    if (gh < 4) gh = 4;

    if (!ml_term_init()) {
        fprintf(stderr, "maxlife: requires a tty\n");
        return 1;
    }
    ml_term_enter_alt_screen();
    ml_term_hide_cursor();
    ml_term_clear_screen();

    ml_grid *canvas = ml_grid_new(gw, gh);
    ml_life *life   = ml_life_new(gw, gh);
    if (!canvas || !life) {
        ml_grid_free(canvas);
        ml_life_free(life);
        ml_term_show_cursor();
        ml_term_exit_alt_screen();
        ml_term_restore();
        return 1;
    }

    uint32_t seed = opt->seed ? opt->seed : (uint32_t)time(NULL);
    double density = opt->density;
    if (density < 0.01) density = 0.01;
    if (density > 0.60) density = 0.60;
    ml_life_seed_random(life, density, seed);

    const ml_rgb *life_pal = ml_palette_named(opt->life_palette);
    const ml_rgb *frac_pal = ml_fractal_palette(opt->fractal_palette);

    size_t bufsize = (size_t)gw * (size_t)gh * 64 + 4096;
    char *buf = (char *)malloc(bufsize);
    if (!buf) {
        ml_grid_free(canvas); ml_life_free(life);
        ml_term_show_cursor(); ml_term_exit_alt_screen(); ml_term_restore();
        return 1;
    }

    double frame_interval = 1.0 / (opt->fps > 0 ? opt->fps : 24.0);
    double start = monotonic_seconds();

    while (1) {
        if (ml_term_quit_pressed()) break;
        double t = monotonic_seconds() - start;

        ml_life_tick(life);

        /* Reseed when population collapses below 5% of initial. */
        int alive = ml_life_alive_count(life);
        int init  = ml_life_initial_count(life);
        if (alive == 0 || (init > 0 && alive * 20 < init)) {
            seed = seed * 1664525u + 1013904223u;
            ml_life_seed_random(life, density, seed);
        }

        /* Background: fractal or clear. */
        if (opt->fractal) {
            ml_fractal_paint(canvas, t, frac_pal);
        } else {
            ml_grid_clear(canvas);
        }
        /* Stamp life cells on top. */
        ml_life_stamp(life, canvas, life_pal, opt->decay);

        ml_grid_to_ansi(canvas, buf, bufsize);
        ml_term_home();
        fputs(buf, stdout);
        fflush(stdout);

        sleep_for(frame_interval);
    }

    free(buf);
    ml_grid_free(canvas);
    ml_life_free(life);
    ml_term_show_cursor();
    ml_term_exit_alt_screen();
    ml_term_restore();
    fputs("life finds a way.\n", stdout);
    return 0;
}
