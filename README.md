# maxlife

Conway's Game of Life for the terminal, colored by cell age. Pure C99,
no third-party dependencies.

Spiritually adjacent to [haikala](https://github.com/SpaceTrucker2196/haikala)
and [HaikalaC](https://github.com/SpaceTrucker2196/HaikalaC), this one
strips out the haiku, breathing, ripples, spin, sound, weather, and
global hue cycling. What's left: pure life, with the only color motion
coming from each cell's own lifecycle.

```
              newborn = warmest
         aging = cooling
    just died = coolest (brief decay fade)
```

## Build

```sh
make
./maxlife
```

Tested on macOS (clang) and Linux (gcc). Any POSIX system with a C99
compiler, libc, `termios.h`, and `sys/ioctl.h` should work.

## Run

```sh
./maxlife                                  # thermal palette, no background
./maxlife -f                               # with Julia fractal background
./maxlife -P phoenix -f --fractal-palette twilight
./maxlife -d 0.10                          # very sparse seed → rapid reseed cycles
./maxlife -d 0.35                          # busy
./maxlife --no-decay                       # dying cells vanish without the fade
./maxlife -r 60                            # smoother (if your terminal can keep up)
./maxlife --seed 42                        # deterministic pattern
```

Press `q`, `Esc`, or `Ctrl-C` to exit.

## Color semantics

Each life cell carries an **age** (frames alive). The renderer indexes
into the chosen warm→cool life palette by age:

| age           | palette stop | feel        |
|---------------|--------------|-------------|
| 1 (newborn)   | 7            | white/hot   |
| 2             | 6            | very warm   |
| 3             | 5            | warm        |
| 4–5           | 4            | mature      |
| 6–8           | 3            | cooling     |
| 9–15          | 2            | cool        |
| 16+           | 1            | aged        |
| (decay)       | 0            | coolest     |

When a cell dies, it transitions into a 6-frame **decay** state: the
position keeps rendering at palette stop 0 (the coolest color) with a
tiny `·` / `˙` glyph that fades and then disappears. Disable with
`--no-decay` if you want hard transitions.

### Fractal nourishment (`-f`)

When the Julia fractal background is on, life cells that overlap with
the brighter fractal cells (deep inside the set) gain **+3 generations
of grace** — a small token counter that lets them survive 3 extra
would-die ticks beyond what B3/S23 alone allows. Patterns naturally
cluster and persist in the bright fractal regions, while cells out in
the dim Julia escape areas live and die under pure Conway rules.

The bright threshold (luminance ≥ 80/255 on the fractal palette
stop) catches the upper half of every fractal palette — so any
palette gives a usable nourishment field.

**Fed cells adopt the color they ate.** Once a life cell feeds on a
fractal cell, it absorbs that fractal color and renders in it for
the rest of its lifecycle — no more warm→cool age fade for that
cell. Cells that continue to overlap bright fractal regions update
their color as the Julia parameter drifts and the palette shifts
beneath them. When a fed cell finally dies, the fed state is
cleared; a new birth at that position starts fresh on the age-based
ramp until it eats something itself.

Visually: most of the field follows the warm→cool age fade, but
clusters of cells that found bright fractal regions stand out as
patches of fractal-palette color — sapphire for ocean, ember-orange
for ember, etc.

## Palettes

Five built-in life palettes, all going **warm → cool**:

- `thermal` — full spectrum: white → gold → orange → red → magenta → indigo (default)
- `embers` — coal: cream → orange → red → dark red → black
- `phoenix` — saturated fire: white → orange → red → magenta → plum
- `aurora` — green → cyan → blue → violet
- `frost` — pure white → ice cyan → deep blue → indigo

For the optional fractal background, 8 named palettes (Julia-set style,
dark→bright): `aurora`, `ember`, `ocean`, `forest`, `sakura`,
`twilight`, `lava`, `coral`.

## Flags

| short | long                       | what it does |
|---|----|----|
| `-h` | `--help`                       | show help |
| `-d` | `--density N`                  | initial alive density 0.01..0.60 (default 0.18) |
| `-P` | `--palette NAME`               | life palette (warm→cool) |
| `-f` | `--fractal`                    | Julia field background |
|      | `--fractal-palette NAME`       | which fractal palette to use |
|      | `--no-decay`                   | don't fade dying cells |
| `-r` | `--fps N`                      | frames per second (default 24) |
|      | `--seed N`                     | RNG seed for initial pattern |

## Layout

```
maxlife/
├── Makefile
├── include/
│   └── maxlife.h
├── src/
│   ├── main.c        # argv parsing
│   ├── animate.c     # tick + render loop
│   ├── terminal.c    # termios + ANSI control
│   ├── render.c      # Cell grid → ANSI emitter
│   ├── palette.c     # 5 life palettes + 8 fractal palettes
│   ├── fractal.c     # drifting Julia background
│   └── life.c        # Conway B3/S23 with age + decay
└── tests/
    └── test_basic.c
```

## License

MIT.
