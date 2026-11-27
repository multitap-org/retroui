#pragma once

// The CONTINUE card: the last game played, its picture and RESUME (piccolo,
// Andrés 2026-11-26, mockup v2), in both looks, from the same words:
//
//   panel (the desktop): part of the empty screen, centered under the menu
//   bar, behind every window and menu; no title bar, no X, never focused.
//   +--------------------------------------------------------------+
//   | +----------------------+  SUPER MARIO WORLD                   |
//   | |                      |  SNES9X · SUPER NINTENDO             |
//   | |  the picture, at the |  PLAYED TODAY AT 21:40               |
//   | |  aspect it was       |  (why it says START, if it does)     |
//   | |  played at           |                                      |
//   | |                      |  [            RESUME              ]  |
//   | +----------------------+  A / ENTER: RESUME · OTHER GAMES ... |
//   +--------------------------------------------------------------+
//
//   card (the phone's HOME upright): full width, the picture across the top
//   edge to edge (any size, not whole pixels; never taller than the card is
//   wide), then the name, the core, when, the
//   line, the buttons.
//
//   cover (the phone's HOME on its side, Andrés's layout C): the picture is
//   the card, as tall as the room, with a dark band over its bottom: the
//   name (two lines at most), the core, when, the line, the buttons.
//
// The picture is drawn bilinear at the recorded aspect: the frontend hands
// a texture already scaled up by a whole number with nearest (sharp
// bilinear), sized for continue_view_picture_size.
//
// Two buttons when the game's save file is newer than the automatic state:
// START first (highlighted), RESUME second.

#include <cstdint>

#include "retro_ui_font.h"
struct TouchUi;

enum class ContinueViewButton
{
    none,
    start,
    resume,
};

// The picture's room on the panel, in retro pixels.
constexpr float CONTINUE_VIEW_PICTURE_WIDTH  = 160.0f;
constexpr float CONTINUE_VIEW_PICTURE_HEIGHT = 108.0f;
// The text column beside it.
constexpr float CONTINUE_VIEW_INFO_WIDTH = 150.0f;

struct ContinueViewCard
{
    const char* game   = "";    // the game's name
    const char* core   = "";    // "SNES9X" (no system: a core for several
                                // can't tell which one the game was for)
    const char* when   = "";    // "PLAYED TODAY AT 21:40"
    const char* reason = "";    // why it says START ("": nothing)
    const char* hint   = "";    // the panel's last line ("": none; the phone shows none)

    const char*        first_label = "";   // the highlighted button
    ContinueViewButton first = ContinueViewButton::resume;
    const char*        second_label = "";  // a second button ("": none)
    ContinueViewButton second = ContinueViewButton::none;

    uint64_t texture = 0;       // the toolkit's texture id (0: a black picture)
    float    u_max = 1.0f;
    float    v_max = 1.0f;
    uint32_t aspect_width = 0;  // the shape to draw it at (0: 4:3)
    uint32_t aspect_height = 0;

    // Nothing covers it (desktop): its first button is highlighted and takes
    // clicks. Covered by a menu or a window it shows, but takes nothing.
    bool active = true;

    // The phone's card: its picture no taller than this (retro pixels; 0:
    // as tall as the card's width makes it, at most square), so the card
    // leaves room for the list.
    float picture_max_height = 0.0f;
};

// The biggest `aspect_width` : `aspect_height` rectangle inside
// `max_width` x `max_height`, on whole units (4:3 when the aspect is
// unknown). Retro pixels on the panel, window pixels too: any unit.
void continue_view_picture_size(uint32_t aspect_width, uint32_t aspect_height, float max_width, float max_height,
                                float* width, float* height);

// The picture's size on the panel at `scale`, in window pixels: what the
// frontend scales the picture up for.
void continue_view_panel_picture_size(const ContinueViewCard* card, int32_t scale, float* width, float* height);

// The desktop panel, centered in the screen below `top` (window pixels: the
// menu bar's height). Returns the button pressed (only while active).
ContinueViewButton continue_view_draw_panel(RetroUiFont* pixel_font, int32_t scale, const ContinueViewCard* card,
                                            float top);

// The phone's card at (x, y), `width` wide (retro pixels of `ui`), on a
// touch screen. Its height goes to `height`. Returns the button tapped.
ContinueViewButton continue_view_draw_card(const TouchUi* ui, const ContinueViewCard* card, float x, float y,
                                           float width, float* height);
// The card's height at `width` without drawing it (retro pixels of `ui`).
float continue_view_card_height(const TouchUi* ui, const ContinueViewCard* card, float width);
// Its picture's size there (retro pixels, fractional): the card's width at
// the aspect, no taller than that width nor picture_max_height. The frontend
// scales for it.
void continue_view_card_picture_size(const ContinueViewCard* card, float width, float* picture_width,
                                     float* picture_height);

// The phone on its side: the picture as big as fits in `max_width` x
// `max_height` at its aspect (retro pixels, fractional). The frontend scales
// for it.
void continue_view_cover_picture_size(const ContinueViewCard* card, float max_width, float max_height,
                                      float* picture_width, float* picture_height);
// Draws it at (x, y) with the band over its bottom; its width goes to
// `width`. Returns the button tapped.
ContinueViewButton continue_view_draw_cover(const TouchUi* ui, const ContinueViewCard* card, float x, float y,
                                            float max_width, float max_height, float* width);
