#pragma once

// How a frontend shows a port and a connection (retroui: ports_view draws
// them; piccoletto's title bar too), so piccolo and piccoletto look the same.
// Sizes in retro pixels, colors ARGB. The drawing functions at the end draw
// both in one place, into the current ImGui window (a layer or a touch screen).
//
// Port LEDs: one small square per port, side by side, like a gamepad's
// player lights; the port's own is lit.
// Ping: three rising bars (3, 5 and 7 pixels high, bottoms level), as many
// lit as the connection deserves, in its color; the rest dark.

#include <cstdint>

constexpr float    PORT_LED_SIZE = 5.0f;   // a letter's height in the 5x5 pixel font
constexpr float    PORT_LED_GAP  = 3.0f;
constexpr uint32_t PORT_LED_ON   = 0xFF3AD15Au;   // green; off is the palette's panel_dark

constexpr uint32_t PING_GOOD    = PORT_LED_ON;    // green
constexpr uint32_t PING_FAIR    = 0xFFFFC83Cu;    // yellow
constexpr uint32_t PING_BAD     = 0xFFE84848u;    // red
constexpr uint32_t PING_GOOD_MS = 16;             // round trip below this: good
constexpr uint32_t PING_FAIR_MS = 48;             // below this: fair; otherwise bad
constexpr uint32_t PING_BARS    = 3;
constexpr float    PING_BAR_WIDTH = 3.0f;
constexpr float    PING_BAR_GAP   = 1.0f;

// The bars lit for a round trip (1..3).
constexpr uint32_t ping_bars_lit(uint32_t round_trip_ms)
{
    return round_trip_ms < PING_GOOD_MS ? 3u : (round_trip_ms < PING_FAIR_MS ? 2u : 1u);
}

// The color of those bars (and of the number after them).
constexpr uint32_t ping_color(uint32_t round_trip_ms)
{
    return round_trip_ms < PING_GOOD_MS ? PING_GOOD : (round_trip_ms < PING_FAIR_MS ? PING_FAIR : PING_BAD);
}

// A bar's height (bar 0 is the shortest).
constexpr float ping_bar_height(uint32_t bar)
{
    return 3.0f + 2.0f * static_cast<float>(bar);
}

// Drawing them into the current ImGui window's draw list, in window pixels;
// `block` = window pixels per retro pixel. Off LEDs and unlit bars are
// `off_argb` (the palette's panel_dark).
//
// LEDs: `count` squares from (x, top), the one at `lit_index` lit (-1: none).
// Returns their width.
float port_indicators_draw_leds(float x, float top, float block, uint32_t count, int32_t lit_index, uint32_t off_argb);
// The ping bars' width.
float port_indicators_ping_width(float block);
// The ping bars from `x`, bottoms level at `bottom`: as many lit as the round
// trip deserves, in its color; all off while not `known`.
void port_indicators_draw_ping(float x, float bottom, float block, bool known, uint32_t ping_ms, uint32_t off_argb);
