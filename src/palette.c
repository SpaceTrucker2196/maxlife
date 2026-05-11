/* Color palettes for maxlife.
 *
 * Two distinct palette sets, each 8 stops:
 *
 *  - LIFE palettes go warm/bright (stop 7) → cool/dark (stop 0). The
 *    life renderer indexes by cell age — newborns get stop 7 (hot),
 *    aging cells slide down toward stop 1, decay cells use stop 0.
 *
 *  - FRACTAL palettes go dark (stop 0) → bright (stop 7), the
 *    standard Julia-set convention. Used only for the optional
 *    background painter.
 */

#include <stdint.h>
#include <string.h>

#include "maxlife.h"

#define HEX(rgb) {0x##rgb >> 16, (0x##rgb >> 8) & 0xFF, 0x##rgb & 0xFF}

/* ---- LIFE palettes (warm → cool, index 7..0) ----------------------- */

/* Index meaning:
 *   7 = newborn (warmest, brightest)
 *   6..1 = aging (cooling)
 *   0 = decay (coolest, dimmest) — just-died cells
 */
static const ml_rgb LIFE_PALETTES[ML_PAL_COUNT][ML_PALETTE_STOPS] = {
    /* THERMAL — full spectrum, sunset-like */
    {
        HEX(0a0820),  /* 0: decay — deepest indigo */
        HEX(2c1a5c),  /* 1: oldest live — purple */
        HEX(5a2887),  /* 2: very old — violet */
        HEX(c0286a),  /* 3: old — magenta-red */
        HEX(ff6a3a),  /* 4: mature — orange */
        HEX(ffae28),  /* 5: young — amber */
        HEX(ffe070),  /* 6: very young — pale gold */
        HEX(fff5e0),  /* 7: newborn — white-cream */
    },
    /* EMBERS — coal / firepit */
    {
        HEX(080000),  /* 0: decay — near-black */
        HEX(2a0808),  /* 1: dim ember */
        HEX(551208),  /* 2: dull ember */
        HEX(8f2a0c),  /* 3: glowing coal */
        HEX(d24a14),  /* 4: bright ember */
        HEX(ff8030),  /* 5: flame */
        HEX(ffc060),  /* 6: hot flame */
        HEX(fff0a0),  /* 7: white-hot core */
    },
    /* PHOENIX — saturated fire with pink/magenta highs */
    {
        HEX(180020),  /* 0: deep violet decay */
        HEX(40104a),  /* 1: aged plum */
        HEX(8a1860),  /* 2: aging magenta */
        HEX(d8285a),  /* 3: rose */
        HEX(ff5040),  /* 4: scarlet */
        HEX(ff9020),  /* 5: orange */
        HEX(ffd040),  /* 6: gold */
        HEX(ffffff),  /* 7: white-hot newborn */
    },
    /* AURORA — green → cyan → blue → violet */
    {
        HEX(080a18),  /* 0: deep night decay */
        HEX(2a1850),  /* 1: indigo */
        HEX(2a4090),  /* 2: blue */
        HEX(28a0c8),  /* 3: cyan */
        HEX(38d0a0),  /* 4: teal-green */
        HEX(80e870),  /* 5: bright green */
        HEX(d0f6a0),  /* 6: pale green */
        HEX(fffff0),  /* 7: white newborn */
    },
    /* FROST — white → cyan → blue → indigo, very cold feel */
    {
        HEX(02041a),  /* 0: blackish blue decay */
        HEX(0a1840),  /* 1: deep indigo */
        HEX(1c4080),  /* 2: deep blue */
        HEX(3878c0),  /* 3: medium blue */
        HEX(6cb0e0),  /* 4: light blue */
        HEX(a8d8f0),  /* 5: pale cyan */
        HEX(d8eef8),  /* 6: ice white */
        HEX(ffffff),  /* 7: pure white newborn */
    },
};

static const char *LIFE_NAMES[ML_PAL_COUNT] = {
    "thermal", "embers", "phoenix", "aurora", "frost",
};

const ml_rgb *ml_palette_named(ml_palette_id id)
{
    if (id < 0 || id >= ML_PAL_COUNT) id = ML_PAL_THERMAL;
    return LIFE_PALETTES[id];
}

ml_palette_id ml_palette_id_from_name(const char *name)
{
    if (!name) return (ml_palette_id)-1;
    for (int i = 0; i < ML_PAL_COUNT; ++i) {
        if (strcmp(name, LIFE_NAMES[i]) == 0) return (ml_palette_id)i;
    }
    return (ml_palette_id)-1;
}

const char *ml_palette_name(ml_palette_id id)
{
    if (id < 0 || id >= ML_PAL_COUNT) return "?";
    return LIFE_NAMES[id];
}

/* ---- FRACTAL palettes (dark → bright, index 0..7) ---------------- */

static const ml_rgb FRACTAL_PALETTES[ML_FP_COUNT][ML_PALETTE_STOPS] = {
    /* aurora */ {
        HEX(000a1a), HEX(022040), HEX(0a4060), HEX(19828c),
        HEX(2cd9a0), HEX(7af56a), HEX(c270ff), HEX(ffaff0),
    },
    /* ember */ {
        HEX(0a0000), HEX(2b0500), HEX(660f00), HEX(a82800),
        HEX(e65800), HEX(ff9d20), HEX(ffd76a), HEX(fff8d0),
    },
    /* ocean */ {
        HEX(000814), HEX(001d3d), HEX(003566), HEX(0077b6),
        HEX(00b4d8), HEX(48cae4), HEX(caf0f8), HEX(ffffff),
    },
    /* forest */ {
        HEX(0a1f0a), HEX(15391a), HEX(2c5a2c), HEX(4c7a2c),
        HEX(7ea848), HEX(b8c870), HEX(e8c878), HEX(fff0c0),
    },
    /* sakura */ {
        HEX(1a0e1a), HEX(3a1f30), HEX(6e3a55), HEX(a55a7a),
        HEX(dc88a8), HEX(f5b6cc), HEX(fcdde6), HEX(ffffff),
    },
    /* twilight */ {
        HEX(080216), HEX(1a0a3a), HEX(36195e), HEX(5a2887),
        HEX(8e3ec0), HEX(c266e0), HEX(ec96f0), HEX(fcd9ff),
    },
    /* lava */ {
        HEX(050505), HEX(1a0a05), HEX(3d1a0a), HEX(7a2a14),
        HEX(c75028), HEX(ee8b3a), HEX(f5cf6a), HEX(fff5d0),
    },
    /* coral */ {
        HEX(001a1a), HEX(024040), HEX(157070), HEX(3aa8a0),
        HEX(ffb88a), HEX(ff8466), HEX(ffd6b0), HEX(ffffff),
    },
};

static const char *FRACTAL_NAMES[ML_FP_COUNT] = {
    "aurora", "ember", "ocean", "forest",
    "sakura", "twilight", "lava", "coral",
};

const ml_rgb *ml_fractal_palette(ml_fractal_palette_id id)
{
    if (id < 0 || id >= ML_FP_COUNT) id = ML_FP_AURORA;
    return FRACTAL_PALETTES[id];
}

ml_fractal_palette_id ml_fractal_palette_from_name(const char *name)
{
    if (!name) return (ml_fractal_palette_id)-1;
    for (int i = 0; i < ML_FP_COUNT; ++i) {
        if (strcmp(name, FRACTAL_NAMES[i]) == 0) return (ml_fractal_palette_id)i;
    }
    return (ml_fractal_palette_id)-1;
}

const char *ml_fractal_palette_name(ml_fractal_palette_id id)
{
    if (id < 0 || id >= ML_FP_COUNT) return "?";
    return FRACTAL_NAMES[id];
}
