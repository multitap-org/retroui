#pragma once

// The HUD over the game: what HUD widgets show, drawn the same in every
// frontend (Andrés, 2026-11-21; piccolo-agents design-notes "Widgets: the API
// in code"). HUD widgets take no input and never pause; any number show at
// once. Two stacks, so nothing they show overlaps:
//
//   STATE ICONS     top right: small square chips, one per state (PLAY,
//                   PAUSED, REC, REWINDING, FAST FORWARD with its speed),
//                   right to left in the order they were added.
//   NOTIFICATIONS   bottom right: short-lived cards of one width (a
//                   controller with no remap, why the DVR didn't open),
//                   the newest at the bottom, older ones pushed up; cards
//                   that don't fit above the area's top aren't drawn (the
//                   oldest first).
//
//   +--------------------------------------------- [||][>>x4] +
//   |                                                         |
//   |                      (the game)       +---------------+ |
//   |                                       |[PORT 2] DUAL  5| |
//   |                                       |HOLD [START] TO | |
//   |                                       +---------------+ |
//   |                                       |[PORT 1] SN30  2| |
//   |                                       |HOLD [START] TO | |
//   | [P1][P2] status line                  +---------------+ |
//   +---------------------------------------------------------+
//
// Every frame the frontend clears a HudStack, each HUD widget adds what it
// shows, and the frontend draws it once, behind every window, inside a
// HudArea: where the stacks may go this frame (the frontend shrinks it for
// the menu bar, QUICK MENU, the DVR's strip, the on-screen pad...). Layout is
// in retro pixels and needs no ImGui (tests/hud_view_test.cpp); nothing is
// allocated.

#include <cstdint>

#include "retro_ui_font.h"

constexpr uint32_t HUD_VIEW_MAX_CARDS = 8;
constexpr uint32_t HUD_VIEW_MAX_ICONS = 8;
constexpr float    HUD_VIEW_CARD_WIDTH = 132.0f;   // retro pixels, every card
constexpr float    HUD_VIEW_MARGIN = 6.0f;         // from the area's sides
constexpr float    HUD_VIEW_GAP = 3.0f;            // between cards, between icons
constexpr float    HUD_VIEW_ICON = 13.0f;          // an icon chip's height (and width without a label)

enum class HudIcon
{
    play,           // the game plays on (after the DVR's PLAY FROM HERE)
    pause,
    rec,
    rewind,
    fast_forward,
};

// A notification card: a title line (a chip, a title, a corner note such as a
// countdown) and an optional second line "before [key] after" (a key chip in
// the middle when `key` isn't empty). Texts are copied in (UTF-8, cut to fit).
struct HudCard
{
    char     chip[16] = {};      // the title chip ("PORT 1"); empty: no chip
    char     title[48] = {};     // after it ("SN30 PRO")
    char     corner[8] = {};     // right of the title line ("5")
    char     before[48] = {};    // the second line; all three empty: one line only
    char     key[16] = {};
    char     after[24] = {};
    float    key_fill = 0.0f;    // 0..1 of the key chip lit (a button being held)
    float    bar = -1.0f;        // 0..1: a countdown bar along the bottom; < 0: none
    float    alpha = 1.0f;       // the widget fades its card out with this
    uint64_t since_ms = 0;       // when it appeared: it slides in, and orders the stack
};

struct HudIconChip
{
    HudIcon icon = HudIcon::pause;
    char    label[8] = {};       // after the icon ("x4"); empty: a square chip
};

struct HudStack
{
    HudCard     cards[HUD_VIEW_MAX_CARDS];
    uint32_t    card_count = 0;
    HudIconChip icons[HUD_VIEW_MAX_ICONS];
    uint32_t    icon_count = 0;
};

// Where the stacks may go this frame, in retro pixels from the screen's top
// left: icons hang from (right, top), cards stand on (right, bottom).
struct HudArea
{
    float right = 0.0f;    // usually the screen's width; QUICK MENU's left edge while it's up
    float top = 0.0f;      // below the menu bar while it shows
    float bottom = 0.0f;   // above the status line, the DVR's strip, the on-screen pad
};

struct HudRect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    bool  shown = false;   // false: it didn't fit
};

// Every frame, before the widgets add to it.
void hud_view_clear(HudStack* stack);

// What a HUD widget shows this frame. False when the stack is full (it isn't shown).
bool hud_view_add_card(HudStack* stack, const HudCard* card);
bool hud_view_add_icon(HudStack* stack, HudIcon icon, const char* label);

// A plain message as a card's text: on the title line, and wrapped at a
// space onto the second line when it's wider than the card.
void hud_view_card_text(HudCard* card, const char* text);

// A card's height in retro pixels (one or two lines).
float hud_view_card_height(const HudCard* card);

// Where a card / an icon goes in `area` (retro pixels, before any slide-in).
HudRect hud_view_card_rect(const HudStack* stack, const HudArea* area, uint32_t index);
HudRect hud_view_icon_rect(const HudStack* stack, const HudArea* area, uint32_t index);

// Both stacks, behind every window, at `scale` window pixels per retro pixel.
void hud_view_draw(const HudStack* stack, const HudArea* area, RetroUiFont* pixel_font, int32_t scale, uint64_t now_ms);
