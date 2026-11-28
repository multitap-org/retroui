#pragma once

// The CONTINUE card: the last game played, its picture and RESUME (piccolo,
// Andrés 2026-11-26, mockup v2), in both looks, from the same words:
//
//   panel (the desktop): part of the empty screen, centered under the menu
//   bar, behind every window and menu; no title bar, no X, never focused.
//   Side by side in a square or wide area, stacked (the picture on top, the
//   same width) in a tall one; the shelf of recent games under it, never
//   more than four tiles at once (the rest on pages); in a short window,
//   a text list of them beside the card instead (continue_view_panel_layout).
//   +--------------------------------------------------------------+
//   | +----------------------+  SUPER MARIO WORLD                   |
//   | |  the picture, at the |  SNES9X                              |
//   | |  aspect it was       |  PLAYED TODAY AT 21:40               |
//   | |  played at, in a     |  (why it says RESTART, if it does)   |
//   | |  square              |  [ RESUME ] [ RESTART ]              |
//   | +----------------------+  A / ENTER: RESUME · DOWN: OTHER ... |
//   | RECENT                                                   < 1/3 > |
//   | ------------------------------------------------------------ |
//   | [tile] [tile] [tile] [tile]   (name, then "core · when")     |
//   +--------------------------------------------------------------+
//   A tile doesn't start its game: it goes into the card (select, then
//   RESUME or RESTART).
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

// The card's picture room on the panel, in retro pixels: a square, so a
// vertical game (3:4) is 120 x 160 and a 4:3 one 160 x 120.
constexpr float CONTINUE_VIEW_PICTURE_WIDTH  = 160.0f;
constexpr float CONTINUE_VIEW_PICTURE_HEIGHT = 160.0f;
// The text column beside it.
constexpr float CONTINUE_VIEW_INFO_WIDTH = 150.0f;
// The panel's width (the same side by side and stacked), and the shelf's
// tiles: 4 across its inside, a 4:3 box each.
constexpr float CONTINUE_VIEW_PANEL_WIDTH = 329.0f;
constexpr float CONTINUE_VIEW_STACKED_PICTURE_HEIGHT = 240.0f;   // the stacked card's picture, at most this tall
constexpr uint32_t CONTINUE_VIEW_TILES_MAX = 4;                  // shown at once, in every window shape
constexpr float CONTINUE_VIEW_TILE_WIDTH  = 77.0f;
constexpr float CONTINUE_VIEW_TILE_HEIGHT = 58.0f;

// One game of the shelf.
struct ContinueViewTile
{
    const char* game = "";   // its name (clipped to the tile)
    const char* line = "";   // "SNES9X · TODAY" (clipped)
    const char* core = "";   // the core's name: on the dark box of a game without a picture
    uint64_t    texture = 0; // 0: no picture
    float       u_max = 1.0f;
    float       v_max = 1.0f;
    uint32_t    aspect_width = 0;   // the shape its picture is drawn at (0: 4:3)
    uint32_t    aspect_height = 0;
};

// The recent games under the card: one page of at most four tiles, and where
// the keyboard / controller is.
struct ContinueViewShelf
{
    const char*      header = "";   // "RECENT"
    ContinueViewTile tiles[CONTINUE_VIEW_TILES_MAX];
    uint32_t         tile_count = 0;   // 0: no shelf
    uint32_t         page = 0;         // shown, from 0
    uint32_t         page_count = 0;   // more than 1: the page indicator and its arrows show
};

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

    // Nothing covers it (desktop): its focused button is highlighted and takes
    // clicks. Covered by a menu or a window it shows, but takes nothing.
    bool active = true;

    // The desktop panel's shelf and where the keys are: on a button (the
    // second when `focus_second`) or on a tile of the page (`focus_tile` >= 0,
    // white border, no button highlighted).
    ContinueViewShelf shelf;
    int32_t           focus_tile = -1;
    bool              focus_second = false;

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

// How the panel is laid out in a window (the one decision, tested):
//   side_shelf:    the card side by side, the shelf of pictures under it
//   side_list:     a window under 720 px tall (the tiles would be tiny at 1
//                  px per retro pixel) that is wide enough for the card and
//                  a column beside it: the other games as a text list at
//                  the card's right instead, 4 rows, paged
//   stacked_shelf: a window taller than wide: the picture on top, the shelf
//                  under it
enum class ContinueViewLayout
{
    side_shelf,
    side_list,
    stacked_shelf,
};
// The list's column, and the window width (retro pixels) the card plus the
// list need.
constexpr float CONTINUE_VIEW_LIST_WIDTH     = 150.0f;
constexpr float CONTINUE_VIEW_LIST_MIN_WIDTH = 490.0f;
constexpr float CONTINUE_VIEW_LIST_MAX_HEIGHT = 720.0f;   // window pixels: the list is for smaller windows
ContinueViewLayout continue_view_panel_layout(float screen_width, float screen_height, int32_t scale);

// The card's picture size on the panel at `scale`, in window pixels: what the
// frontend scales the picture up for (drawn smaller only when the stacked
// panel doesn't fit the height).
void continue_view_panel_picture_size(const ContinueViewCard* card, int32_t scale, bool stacked, float* width,
                                      float* height);

// A tile's picture at `scale`, in window pixels: fitted in the tile's 4:3 box.
void continue_view_tile_picture_size(uint32_t aspect_width, uint32_t aspect_height, int32_t scale, float* width,
                                     float* height);

// What the panel returned this frame (only while active).
struct ContinueViewAction
{
    ContinueViewButton pressed = ContinueViewButton::none;
    int32_t            tile_chosen = -1;   // a click on this tile of the page
    int32_t            page_delta = 0;     // a click on the page indicator's arrows: -1 / +1
};

// The desktop panel in the window (`screen_width` x `screen_height`, window
// pixels) below `top` (the menu bar's height), laid out as
// continue_view_panel_layout says, centered in what is left of the screen.
ContinueViewAction continue_view_draw_panel(RetroUiFont* pixel_font, int32_t scale, const ContinueViewCard* card,
                                            float top, float screen_width, float screen_height);

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
