// Snow behind the menu (snow_view.h).

#include "snow_view.h"

#include "imgui.h"
#include "retro_ui.h"

constexpr int32_t  FIXED_ONE = 256;            // 8.8 fixed point
constexpr int32_t  DEFAULT_WIDTH = 640;        // before the frontend sets the area
constexpr int32_t  DEFAULT_HEIGHT = 360;
constexpr uint32_t DEFAULT_SEED = 20261225u;
constexpr uint32_t NOSE_COLOR = 0xFFE05050u;   // the leading reindeer's nose, in every theme

// Our own sleigh (drawn for piccolo, 2026-11-25), flying left: two reindeer
// with antlers, the harness ('='), the sleigh with its runner. 'r' is the red
// nose, '#' the body, '=' the harness, '.' nothing.
static const char* const SLEIGH_ROWS[SNOW_VIEW_SLEIGH_HEIGHT] = {
    "..#.#.......#.#.................",
    "...#.........#..................",
    "..###.......###..............#..",
    "r####......####.............##..",
    "######.....#####...........###..",
    ".#####======#####=====..#######.",
    ".##..##.....##..##.....########.",
    ".#....#.....#....#......######..",
    "......................#######..#",
    "........................#####.#.",
};

// xorshift32: small, fast, and the same sequence on every platform.
static uint32_t next_random(SnowView* snow)
{
    uint32_t x = snow->random;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    snow->random = x;
    return x;
}

// 0 .. limit - 1 (limit > 0).
static int32_t random_below(SnowView* snow, int32_t limit)
{
    return static_cast<int32_t>(next_random(snow) % static_cast<uint32_t>(limit));
}

static int32_t area_width(const SnowView* snow)
{
    return snow->width > 0 ? snow->width : DEFAULT_WIDTH;
}

static int32_t area_height(const SnowView* snow)
{
    return snow->height > 0 ? snow->height : DEFAULT_HEIGHT;
}

static void update_count(SnowView* snow)
{
    uint64_t count = static_cast<uint64_t>(area_width(snow)) * static_cast<uint64_t>(area_height(snow)) /
                     static_cast<uint64_t>(snow->params.pixels_per_flake);
    snow->count = count < SNOW_VIEW_MAX_FLAKES ? static_cast<uint32_t>(count) : SNOW_VIEW_MAX_FLAKES;
    if (snow->width == 0)
    {
        snow->count = 0;   // no area yet
    }
}

static int32_t clamp_int(int32_t value, int32_t min, int32_t max)
{
    return value < min ? min : (value > max ? max : value);
}

static float clamp_float(float value, float min, float max)
{
    return value < min ? min : (value > max ? max : value);
}

// Retro pixels to 1/256 of one.
static int32_t to_fixed(float pixels)
{
    return static_cast<int32_t>(pixels * static_cast<float>(FIXED_ONE) + 0.5f);
}

void snow_view_set_params(SnowView* snow, const SnowParams* params)
{
    SnowParams clean = *params;
    clean.pixels_per_flake = clamp_int(clean.pixels_per_flake, 16, 100000);
    clean.step_hz = clamp_int(clean.step_hz, 1, 240);
    clean.fall = clamp_float(clean.fall, 0.0f, 16.0f);
    clean.fall_extra = clamp_float(clean.fall_extra, 0.0f, 16.0f);
    clean.drift = clamp_float(clean.drift, 0.0f, 16.0f);
    clean.shade_spread = clamp_int(clean.shade_spread, 0, 100);
    if (clean.step_hz != snow->params.step_hz)
    {
        snow->started = false;
    }
    snow->params = clean;
    update_count(snow);
}

void snow_view_start(SnowView* snow, uint32_t seed)
{
    snow->random = seed != 0 ? seed : DEFAULT_SEED;
    int32_t width = area_width(snow);
    int32_t height = area_height(snow);
    // Every flake, not only those in use: a bigger area later uses more of
    // them, already spread out.
    for (uint32_t i = 0; i < SNOW_VIEW_MAX_FLAKES; i++)
    {
        SnowFlake* flake = &snow->flakes[i];
        flake->x = random_below(snow, width * FIXED_ONE);
        flake->y = random_below(snow, height * FIXED_ONE);
        flake->vx = random_below(snow, FIXED_ONE);
        flake->vy = random_below(snow, FIXED_ONE);
        flake->shade = static_cast<uint8_t>(random_below(snow, static_cast<int32_t>(SNOW_VIEW_SHADES)));
        flake->shown = false;
    }
    snow->steps = 0;
    snow->started = false;
    snow->sleigh_crossing = false;
    snow->sleigh_wait = 0;
}

void snow_view_restart(SnowView* snow)
{
    for (uint32_t i = 0; i < SNOW_VIEW_MAX_FLAKES; i++)
    {
        snow->flakes[i].shown = false;
    }
    snow->started = false;
    snow->sleigh_crossing = false;
    snow->sleigh_wait = 0;
}

void snow_view_set_area(SnowView* snow, int32_t width, int32_t height)
{
    if (width < 1)
    {
        width = 1;
    }
    if (height < 1)
    {
        height = 1;
    }
    if (width == snow->width && height == snow->height)
    {
        return;
    }
    int32_t old_width = area_width(snow);
    int32_t old_height = area_height(snow);
    for (uint32_t i = 0; i < SNOW_VIEW_MAX_FLAKES; i++)
    {
        SnowFlake* flake = &snow->flakes[i];
        flake->x = static_cast<int32_t>(static_cast<int64_t>(flake->x) * width / old_width);
        flake->y = static_cast<int32_t>(static_cast<int64_t>(flake->y) * height / old_height);
    }
    snow->width = width;
    snow->height = height;
    update_count(snow);
}

bool snow_view_all_shown(const SnowView* snow)
{
    for (uint32_t i = 0; i < snow->count; i++)
    {
        if (!snow->flakes[i].shown)
        {
            return false;
        }
    }
    return true;
}

static void step_sleigh(SnowView* snow)
{
    if (snow->sleigh_crossing)
    {
        snow->sleigh_x--;
        if (snow->sleigh_x < -SNOW_VIEW_SLEIGH_WIDTH)
        {
            snow->sleigh_crossing = false;
        }
    }
    if (!snow->sleigh_day || !snow_view_all_shown(snow))
    {
        return;
    }
    // Once a minute from the start of one crossing to the next.
    if (snow->sleigh_wait > 0)
    {
        snow->sleigh_wait--;
        return;
    }
    snow_view_sleigh_now(snow);
    snow->sleigh_wait = SNOW_VIEW_SLEIGH_EVERY;
}

void snow_view_step(SnowView* snow)
{
    int32_t width = area_width(snow) * FIXED_ONE;
    int32_t height = area_height(snow) * FIXED_ONE;
    int32_t fall = to_fixed(snow->params.fall);
    int32_t fall_extra = to_fixed(snow->params.fall_extra);
    int32_t drift = to_fixed(snow->params.drift);
    for (uint32_t i = 0; i < snow->count; i++)
    {
        SnowFlake* flake = &snow->flakes[i];
        flake->x = (flake->x + flake->vx * drift / FIXED_ONE) % width;
        flake->y += fall + flake->vy * fall_extra / FIXED_ONE;
        if (flake->y >= height)
        {
            flake->y %= height;
            flake->shown = true;   // it came through the top: from now on it's seen
        }
    }
    step_sleigh(snow);
    snow->steps++;
}

uint32_t snow_view_update(SnowView* snow, uint64_t now_ms)
{
    if (!snow->started || now_ms < snow->start_ms)
    {
        snow->started = true;
        snow->start_ms = now_ms;
        snow->steps = 0;
        return 0;
    }
    uint64_t hz = static_cast<uint64_t>(snow->params.step_hz);
    uint64_t due = (now_ms - snow->start_ms) * hz / 1000;
    uint32_t catch_up = static_cast<uint32_t>(hz / 4 > 0 ? hz / 4 : 1);
    uint32_t taken = 0;
    while (snow->steps < due && taken < catch_up)
    {
        snow_view_step(snow);
        taken++;
    }
    if (snow->steps < due)
    {
        // A stall (a dragged window, a slow load): skip the time instead of
        // racing to catch up. The clock restarts from this step.
        snow->start_ms = now_ms;
        snow->steps = 0;
    }
    return taken;
}

void snow_view_set_sleigh_day(SnowView* snow, bool sleigh_day)
{
    snow->sleigh_day = sleigh_day;
}

void snow_view_sleigh_now(SnowView* snow)
{
    snow->sleigh_crossing = true;
    snow->sleigh_x = area_width(snow);
}

bool snow_view_in_season(int32_t month, int32_t day)
{
    return month == 12 && day >= 24 && day <= 28;
}

bool snow_view_is_sleigh_day(int32_t month, int32_t day)
{
    return month == 12 && day == 25;
}

// ---------------------------------------------------------------------------
// Drawing

// `a` toward `b` by percent (0..100), each channel; alpha from `a`.
static uint32_t mix(uint32_t a, uint32_t b, uint32_t percent)
{
    uint32_t out = a & 0xFF000000u;
    for (uint32_t shift = 0; shift < 24; shift += 8)
    {
        int32_t from = static_cast<int32_t>((a >> shift) & 0xFFu);
        int32_t to = static_cast<int32_t>((b >> shift) & 0xFFu);
        int32_t value = from + (to - from) * static_cast<int32_t>(percent) / 100;
        out |= static_cast<uint32_t>(value) << shift;
    }
    return out;
}

static void draw_sleigh(ImDrawList* draw_list, const SnowView* snow, float x, float y, float block)
{
    const RetroUiPalette* palette = retro_ui_palette();
    uint32_t body = retro_ui_color_imgui(mix(palette->text, palette->panel_face, 50));
    uint32_t harness = retro_ui_color_imgui(mix(palette->text, palette->panel_face, 30));
    uint32_t nose = retro_ui_color_imgui(NOSE_COLOR);
    for (int32_t row = 0; row < SNOW_VIEW_SLEIGH_HEIGHT; row++)
    {
        const char* line = SLEIGH_ROWS[row];
        for (int32_t column = 0; column < SNOW_VIEW_SLEIGH_WIDTH; column++)
        {
            char c = line[column];
            if (c == '.')
            {
                continue;
            }
            uint32_t color = c == 'r' ? nose : (c == '=' ? harness : body);
            float left = x + static_cast<float>(snow->sleigh_x + column) * block;
            float top = y + static_cast<float>(SNOW_VIEW_SLEIGH_ROW + row) * block;
            draw_list->AddRectFilled(ImVec2(left, top), ImVec2(left + block, top + block), color);
        }
    }
}

void snow_view_draw(const SnowView* snow, float x, float y, int32_t scale)
{
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    float block = static_cast<float>(scale);
    // The sleigh first: the snow falls in front of it.
    if (snow->sleigh_crossing)
    {
        draw_sleigh(draw_list, snow, x, y, block);
    }
    const RetroUiPalette* palette = retro_ui_palette();
    // Evenly from the text color to shade_spread % toward the panel's (0, 20, 40, 60 by default).
    uint32_t spread = static_cast<uint32_t>(snow->params.shade_spread);
    uint32_t colors[SNOW_VIEW_SHADES] = {};
    for (uint32_t i = 0; i < SNOW_VIEW_SHADES; i++)
    {
        uint32_t percent = spread * i / (SNOW_VIEW_SHADES - 1);
        colors[i] = retro_ui_color_imgui(mix(palette->text, palette->panel_face, percent));
    }
    for (uint32_t i = 0; i < snow->count; i++)
    {
        const SnowFlake* flake = &snow->flakes[i];
        if (!flake->shown)
        {
            continue;
        }
        float left = x + static_cast<float>(flake->x / FIXED_ONE) * block;
        float top = y + static_cast<float>(flake->y / FIXED_ONE) * block;
        draw_list->AddRectFilled(ImVec2(left, top), ImVec2(left + block, top + block), colors[flake->shade]);
    }
}
