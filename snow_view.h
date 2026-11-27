#pragma once

// Snow behind the menu: an easter egg after ZSNES's GUI snow (Andrés,
// 2026-11-25; piccolo-agents design-notes "Menu snow, after ZSNES's"). ZSNES
// is GPL: only the behaviour is carried over, nothing of its code, tables or
// bitmaps.
//
//   - One retro pixel per flake, drawn through ImGui's background draw list:
//     behind every window and the menu bar, over the game's picture.
//   - About one flake per 290 retro pixels of the area (800 on 640 x 360).
//   - A fixed 36 Hz step, caught up before each draw: the stepping is part of
//     the look. Per step a flake falls 1 + vy retro pixels and drifts vx to
//     the right (vx, vy in 0..1, fixed per flake), never left, and wraps
//     around both edges.
//   - A flake shows only once it has wrapped through the top, so the snow
//     always starts from the sky (when it's turned on, or shown again).
//   - Four shades per flake, from the theme's text color toward its panel
//     color (0, 20, 40 and 60 %), so every theme gets snow that fits it.
//   - The December 25 sleigh: our own drawing (two reindeer, a red nose, the
//     sleigh) crossing right to left at a fixed row, 1 retro pixel per step,
//     once a minute once every flake is falling.
//
// The frontend decides when it shows (snow_view_in_season() is the window
// both frontends use). Positions are in retro pixels, 8.8 fixed point; the
// logic needs no ImGui (tests/snow_view_test.cpp). Nothing is allocated: the
// flakes are a fixed array.

#include <cstdint>

constexpr uint32_t SNOW_VIEW_MAX_FLAKES = 4096;
constexpr uint32_t SNOW_VIEW_PIXELS_PER_FLAKE = 290;   // ZSNES: 200 flakes on 256 x 224
constexpr uint32_t SNOW_VIEW_STEP_HZ = 36;
constexpr uint32_t SNOW_VIEW_MAX_CATCH_UP = 9;         // steps at most per update at 36 Hz (a quarter second)
constexpr uint32_t SNOW_VIEW_SHADES = 4;
constexpr int32_t  SNOW_VIEW_SLEIGH_ROW = 60;          // retro pixels from the area's top
constexpr int32_t  SNOW_VIEW_SLEIGH_WIDTH = 32;
constexpr int32_t  SNOW_VIEW_SLEIGH_HEIGHT = 10;
constexpr uint32_t SNOW_VIEW_SLEIGH_EVERY = 60 * SNOW_VIEW_STEP_HZ;   // a crossing starts once a minute

// The knobs (MENU > DEBUG tunes them live; frontends keep the defaults, the
// look described above). Speeds in retro pixels per step.
struct SnowParams
{
    int32_t pixels_per_flake = static_cast<int32_t>(SNOW_VIEW_PIXELS_PER_FLAKE);   // density: area / this
    int32_t step_hz = static_cast<int32_t>(SNOW_VIEW_STEP_HZ);
    float   fall = 1.0f;          // every flake falls this much a step...
    float   fall_extra = 1.0f;    // ...plus 0..this (its own share)
    float   drift = 1.0f;         // right 0..this a step (its own share); never left
    int32_t shade_spread = 60;    // the palest shade, % from the text color toward the panel's
};

// One flake. Positions in 1/256 of a retro pixel; vx, vy are the flake's own
// share (0..255) of SnowParams' drift and fall_extra.
struct SnowFlake
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t vx = 0;        // 0..255 of drift
    int32_t vy = 0;        // 0..255 of fall_extra
    uint8_t shade = 0;     // 0..3: 0 is the text color
    bool    shown = false; // wrapped through the top once
};

struct SnowView
{
    SnowFlake flakes[SNOW_VIEW_MAX_FLAKES];
    SnowParams params;
    uint32_t  count = 0;            // flakes in use for this area
    int32_t   width = 0;            // the area, retro pixels (0: not set yet)
    int32_t   height = 0;
    uint32_t  random = 0;           // the PRNG's state (xorshift32, never 0)
    uint64_t  steps = 0;            // steps taken since snow_view_start
    uint64_t  start_ms = 0;
    bool      started = false;      // the clock runs (false: the next update starts it)

    // The sleigh.
    bool      sleigh_day = false;   // it crosses once a minute (December 25)
    bool      sleigh_crossing = false;
    int32_t   sleigh_x = 0;         // its left edge, retro pixels
    uint32_t  sleigh_wait = 0;      // steps until the next crossing; 0: as soon as every flake shows
};

// Fresh snow: every flake's position, speed and shade from the PRNG, all
// hidden until they wrap through the top. `seed` 0 picks a fixed one.
void snow_view_start(SnowView* snow, uint32_t seed);

// Snow shown again after a while away (the menu closed, the area hidden):
// the flakes hide until they come through the top again, and the clock
// restarts at the next update. A sleigh crossing ends; the next one waits
// until every flake shows again.
void snow_view_restart(SnowView* snow);

// The area (retro pixels). A new size sets how many flakes are used and
// moves the flakes in proportion, so a resize doesn't bunch them up.
void snow_view_set_area(SnowView* snow, int32_t width, int32_t height);

// New knobs (values out of range are clamped). The density applies at once,
// the speeds from the next step; the clock restarts if the rate changed.
void snow_view_set_params(SnowView* snow, const SnowParams* params);

// Steps the snow up to `now_ms` (step_hz a second, at most a quarter second's
// worth at once: a long stall skips time rather than racing). Returns the
// steps taken.
uint32_t snow_view_update(SnowView* snow, uint64_t now_ms);

// One 36 Hz step (snow_view_update calls it; tests call it directly).
void snow_view_step(SnowView* snow);

// December 25: the sleigh crosses once a minute once every flake shows.
void snow_view_set_sleigh_day(SnowView* snow, bool sleigh_day);
// Sends one across now, whatever the day (MENU > DEBUG > SLEIGH NOW).
void snow_view_sleigh_now(SnowView* snow);
// Every flake in use has come through the top.
bool snow_view_all_shown(const SnowView* snow);

// The season the snow shows by itself: December 24 to 28, local date (Andrés,
// 2026-11-25). `month` 1..12.
bool snow_view_in_season(int32_t month, int32_t day);
bool snow_view_is_sleigh_day(int32_t month, int32_t day);

// Draws the flakes (and the sleigh) on ImGui's background draw list, the
// area's top left at (x, y) in screen pixels, `scale` screen pixels per retro
// pixel, in the palette's colors (retro_ui_palette()).
void snow_view_draw(const SnowView* snow, float x, float y, int32_t scale);
