/* maxlife — pure Conway's Life with warm/cool age coloring. */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "maxlife.h"

static void usage(FILE *out)
{
    fputs(
        "maxlife — Conway's Life for the terminal, colored by cell age\n"
        "\n"
        "Usage: maxlife [options]\n"
        "\n"
        "Life:\n"
        "  -d, --density <n>         initial seed density 0.01..0.60 (default 0.18)\n"
        "  -P, --palette <name>      life palette: thermal | embers | phoenix |\n"
        "                            aurora | frost (default thermal)\n"
        "      --no-decay            dying cells vanish immediately instead of fading\n"
        "\n"
        "Background:\n"
        "  -f, --fractal             slowly-drifting Julia field underneath\n"
        "      --fractal-palette <name>\n"
        "                            aurora | ember | ocean | forest | sakura |\n"
        "                            twilight | lava | coral (default ocean)\n"
        "\n"
        "Timing:\n"
        "  -r, --fps <n>             frames per second (default 24)\n"
        "      --seed <n>            RNG seed for the initial pattern (default: time)\n"
        "\n"
        "  -h, --help                show this help\n"
        "\n"
        "Press q or Ctrl-C to exit.\n",
        out);
}

static bool match(const char *arg, const char *short_, const char *long_)
{
    if (short_ && strcmp(arg, short_) == 0) return true;
    return strcmp(arg, long_) == 0;
}

int main(int argc, char **argv)
{
    ml_options opt;
    ml_options_default(&opt);

    for (int i = 1; i < argc; ++i) {
        const char *a = argv[i];
        if (match(a, "-h", "--help"))     { usage(stdout); return 0; }
        if (match(a, "-f", "--fractal"))  { opt.fractal = true; continue; }
        if (match(a, NULL, "--no-fractal")) { opt.fractal = false; continue; }
        if (match(a, NULL, "--no-decay")) { opt.decay = false; continue; }
        if (match(a, "-d", "--density") && i + 1 < argc) {
            opt.density = atof(argv[++i]);
            continue;
        }
        if (match(a, "-r", "--fps") && i + 1 < argc) {
            opt.fps = atof(argv[++i]);
            continue;
        }
        if (match(a, NULL, "--seed") && i + 1 < argc) {
            opt.seed = (uint32_t)strtoul(argv[++i], NULL, 10);
            continue;
        }
        if (match(a, "-P", "--palette") && i + 1 < argc) {
            const char *v = argv[++i];
            ml_palette_id id = ml_palette_id_from_name(v);
            if ((int)id < 0) {
                fprintf(stderr, "maxlife: unknown life palette %s\n", v);
                return 2;
            }
            opt.life_palette = id;
            continue;
        }
        if (match(a, NULL, "--fractal-palette") && i + 1 < argc) {
            const char *v = argv[++i];
            ml_fractal_palette_id id = ml_fractal_palette_from_name(v);
            if ((int)id < 0) {
                fprintf(stderr, "maxlife: unknown fractal palette %s\n", v);
                return 2;
            }
            opt.fractal_palette = id;
            continue;
        }
        fprintf(stderr, "maxlife: unknown option %s\n", a);
        usage(stderr);
        return 2;
    }

    return ml_run(&opt);
}
