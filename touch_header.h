#pragma once

// The header of a touch screen (piccoletto's title bar, Andrés 2026-11-19,
// shared with piccolo's phone UI): the screen's name or a back arrow at the
// left, icons at the right (rotate, ☰, ...), always in the same place.
// Retro pixels, like touch_ui.

#include <cstdint>

#include "touch_ui.h"

constexpr float TOUCH_HEADER_MARGIN = 8.0f;    // from the screen's edges
constexpr float TOUCH_HEADER_ICON   = 24.0f;   // each icon's square

// Its top: 6 retro pixels in landscape (phones have little height then), 8 in portrait.
float touch_header_y(const TouchUi* ui);
// Where the screen's content can start under the header.
float touch_header_bottom(const TouchUi* ui);
// The left edge of `icon_count` icons at the right (a title must end before it).
float touch_header_icons_x(const TouchUi* ui, int32_t icon_count);

// The screen's name at the left, big.
void touch_header_title(const TouchUi* ui, const char* title);

// The icon in slot `slot` of `icon_count` (0 = the leftmost of them), its
// glyph big: true when tapped.
bool touch_header_icon(const TouchUi* ui, const char* id, int32_t slot, int32_t icon_count, const char* glyph);

// The back arrow at the left: true when tapped.
bool touch_header_back(const TouchUi* ui);
