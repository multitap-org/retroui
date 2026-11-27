// snow_view: the step (down 1 + vy, right vx, never left), wrapping on both
// axes, flakes hidden until they come through the top, the density, the 36 Hz
// clock with its catch-up limit, the season and the sleigh.

#include <cstdint>
#include <cstdio>

#include "snow_view.h"

static int g_failures = 0;

static void check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        g_failures++;
    }
}

// Big enough for the stack of a test? It's 80 KB: keep it out of main's frame.
static SnowView g_snow;

static void test_density()
{
    snow_view_start(&g_snow, 1);
    snow_view_set_area(&g_snow, 640, 360);
    check(g_snow.count == 794, "640 x 360 uses 640 * 360 / 290 flakes");
    snow_view_set_area(&g_snow, 4000, 4000);
    check(g_snow.count == SNOW_VIEW_MAX_FLAKES, "a huge area is capped");
    snow_view_set_area(&g_snow, 0, 0);
    check(g_snow.width == 1 && g_snow.height == 1 && g_snow.count == 0, "an empty area has no flakes");
}

static void test_step_and_wrap()
{
    snow_view_start(&g_snow, 2);
    snow_view_set_area(&g_snow, 100, 50);
    SnowFlake* flake = &g_snow.flakes[0];
    flake->x = 10 * 256;
    flake->y = 20 * 256;
    flake->vx = 128;   // half a pixel right
    flake->vy = 64;    // a quarter pixel more than the whole one
    flake->shown = false;
    snow_view_step(&g_snow);
    check(flake->x == 10 * 256 + 128, "right by vx");
    check(flake->y == 20 * 256 + 256 + 64, "down by 1 + vy");
    check(!flake->shown, "still hidden mid-air");

    // Every flake moves right or not at all, and down at least 1 px.
    snow_view_start(&g_snow, 3);
    snow_view_set_area(&g_snow, 640, 360);
    bool never_left = true;
    bool always_down = true;
    for (uint32_t i = 0; i < g_snow.count; i++)
    {
        int32_t x = g_snow.flakes[i].x;
        int32_t y = g_snow.flakes[i].y;
        snow_view_step(&g_snow);
        // Unless it wrapped, x grows by 0..255 and y by 256..511.
        int32_t dx = g_snow.flakes[i].x - x;
        int32_t dy = g_snow.flakes[i].y - y;
        if (dx < 0 && dx > -640 * 256 + 256)
        {
            never_left = false;
        }
        if (dy < 256 && dy > -360 * 256 + 512)
        {
            always_down = false;
        }
        if (dx > 255 || dy > 511)
        {
            always_down = false;
        }
    }
    check(never_left, "no flake moves left");
    check(always_down, "every flake falls 1 to 2 px a step");

    // Wrapping: through the right edge to the left, through the bottom to the top (and shown).
    snow_view_set_area(&g_snow, 100, 50);
    flake = &g_snow.flakes[0];
    flake->x = 99 * 256 + 200;
    flake->vx = 100;
    flake->y = 49 * 256 + 200;
    flake->vy = 0;
    flake->shown = false;
    snow_view_step(&g_snow);
    check(flake->x == 44, "wraps through the right edge");
    check(flake->y == 200, "wraps through the bottom");
    check(flake->shown, "shown once it comes through the top");
}

static void test_starts_from_the_sky()
{
    snow_view_start(&g_snow, 4);
    snow_view_set_area(&g_snow, 320, 180);
    bool any_shown = false;
    for (uint32_t i = 0; i < g_snow.count; i++)
    {
        any_shown = any_shown || g_snow.flakes[i].shown;
    }
    check(!any_shown, "nothing shows at the start");
    // 180 px at 1 px a step at the slowest: every flake has wrapped by then.
    for (uint32_t i = 0; i < 181; i++)
    {
        snow_view_step(&g_snow);
    }
    check(snow_view_all_shown(&g_snow), "every flake shows after one fall");
    snow_view_restart(&g_snow);
    check(!snow_view_all_shown(&g_snow) && !g_snow.flakes[0].shown, "a restart hides them again");
}

static void test_clock()
{
    snow_view_start(&g_snow, 5);
    snow_view_set_area(&g_snow, 320, 180);
    check(snow_view_update(&g_snow, 1000) == 0, "the first update only starts the clock");
    check(snow_view_update(&g_snow, 1000 + 1000 / 36 + 1) == 1, "one step after 1/36 s");
    check(snow_view_update(&g_snow, 1000 + 250) == 8, "caught up to 9 steps in a quarter second");
    check(g_snow.steps == 9, "9 steps in all");
    check(snow_view_update(&g_snow, 1000 + 2000) == SNOW_VIEW_MAX_CATCH_UP, "a stall: at most the catch-up limit");
    check(snow_view_update(&g_snow, 1000 + 2000 + 1000 / 36 + 1) == 1, "then on at 36 Hz from there");
}

static void test_params()
{
    snow_view_start(&g_snow, 8);
    snow_view_set_area(&g_snow, 640, 360);
    SnowParams params = {};
    params.pixels_per_flake = 580;
    params.fall = 2.0f;
    params.fall_extra = 0.0f;
    params.drift = 0.0f;
    snow_view_set_params(&g_snow, &params);
    check(g_snow.count == 397, "half the density: twice the pixels per flake");
    SnowFlake* flake = &g_snow.flakes[0];
    flake->x = 10 * 256;
    flake->y = 10 * 256;
    flake->vx = 255;
    flake->vy = 255;
    snow_view_step(&g_snow);
    check(flake->x == 10 * 256 && flake->y == 12 * 256, "no drift, 2 px a step and no extra");
    params.drift = 2.0f;
    snow_view_set_params(&g_snow, &params);
    flake->vx = 128;
    snow_view_step(&g_snow);
    check(flake->x == 11 * 256, "half its share of 2 px of drift");
    params.pixels_per_flake = 0;
    params.step_hz = 1000;
    snow_view_set_params(&g_snow, &params);
    check(g_snow.params.pixels_per_flake == 16 && g_snow.params.step_hz == 240, "out-of-range knobs are clamped");
    check(!g_snow.started, "a new rate restarts the clock");
    SnowParams defaults = {};
    snow_view_set_params(&g_snow, &defaults);
    check(g_snow.count == 794, "back to the defaults");
}

static void test_season()
{
    check(!snow_view_in_season(12, 23), "not on December 23");
    check(snow_view_in_season(12, 24), "December 24");
    check(snow_view_in_season(12, 28), "December 28");
    check(!snow_view_in_season(12, 29), "not on December 29");
    check(!snow_view_in_season(1, 1) && !snow_view_in_season(11, 25), "not in January or November");
    check(snow_view_is_sleigh_day(12, 25) && !snow_view_is_sleigh_day(12, 24), "the sleigh on December 25 only");
}

static void test_sleigh()
{
    snow_view_start(&g_snow, 6);
    snow_view_set_area(&g_snow, 320, 180);
    snow_view_set_sleigh_day(&g_snow, true);
    snow_view_step(&g_snow);
    check(!g_snow.sleigh_crossing, "no sleigh before the snow is all falling");
    uint32_t steps = 1;
    while (!g_snow.sleigh_crossing && steps < 1000)
    {
        snow_view_step(&g_snow);
        steps++;
    }
    check(g_snow.sleigh_crossing && snow_view_all_shown(&g_snow), "the sleigh once every flake shows");
    check(g_snow.sleigh_x == 320, "it starts at the right edge");
    // From the step it started: 1 px left a step until it's past the left edge.
    steps = 0;
    snow_view_step(&g_snow);
    steps++;
    check(g_snow.sleigh_x == 319, "1 px left a step");
    while (g_snow.sleigh_crossing && steps < 1000)
    {
        snow_view_step(&g_snow);
        steps++;
    }
    check(steps == static_cast<uint32_t>(320 + SNOW_VIEW_SLEIGH_WIDTH + 1), "gone past the left edge after the whole width");
    while (!g_snow.sleigh_crossing && steps < 2 * SNOW_VIEW_SLEIGH_EVERY)
    {
        snow_view_step(&g_snow);
        steps++;
    }
    check(g_snow.sleigh_crossing && steps == SNOW_VIEW_SLEIGH_EVERY + 1, "again a minute after the last one started");

    snow_view_start(&g_snow, 7);
    snow_view_set_sleigh_day(&g_snow, false);
    snow_view_sleigh_now(&g_snow);
    check(g_snow.sleigh_crossing && g_snow.sleigh_x == 320, "SLEIGH NOW: from the right edge, any day");
}

int main()
{
    test_density();
    test_step_and_wrap();
    test_starts_from_the_sky();
    test_clock();
    test_params();
    test_season();
    test_sleigh();
    if (g_failures != 0)
    {
        return 1;
    }
    std::printf("snow OK\n");
    return 0;
}
