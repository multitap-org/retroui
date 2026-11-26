#pragma once

// The rewind DVR's strip, drawn the same in every frontend (Andrés,
// 2026-11-21; the look of the ports' tabs: flat cards the game shows
// through, chips with a darker lip, no bevels). Draw only: the frontend
// moves through its history, makes the thumbnails and fills this in.
//
//   HEADER   the title chip (◀◀ DVR, ▶ WATCHING), the mode, which lane
//            (only on the kept one), how much is kept, where the cursor is,
//            and the exit button at the right (BACK TO NOW, its own
//            button: clicked, or the frontend's back key). Every key is a
//            button acting at once; nothing is focused.
//   STRIP    thumbnails along the bottom (the cursor's lane), the selected
//            one bigger in the middle; the ones after the cursor darker. A
//            track under them with the cursor between two. Two lanes
//            (Andrés, 2026-11-22): the recorded lane is the track as ever,
//            the kept lane a second line above it, from where they part to
//            where it ends; the cursor sits on its lane's line (UP / DOWN in
//            the frontend move it between them).
//            A move that hits either end of the history bounces the selected
//            one toward that end, its border blinking red and white
//            (Andrés: "bounce it a bit").
//   HINTS    gamepad: key chips and what they do. Touch: big buttons (the
//            on-screen pad is hidden while the DVR is open).
//
// The game isn't drawn here: the frontend draws it shrunk into the space
// above (dvr_view_height says how much the strip takes).
//
// Its HUD (the PLAY state icon, why it didn't open) is hud_view's.

#include <cstdint>

#include "retro_ui_font.h"

constexpr uint32_t DVR_VIEW_MAX_THUMBS = 15;
constexpr uint32_t DVR_VIEW_MAX_HINTS  = 6;
constexpr uint32_t DVR_VIEW_LABEL_MAX  = 16;

struct DvrViewThumb
{
    uint64_t texture = 0;      // the GUI's handle; 0: not made yet (an empty card)
    float    u_max   = 1.0f;   // the part of the texture with the picture
    float    v_max   = 1.0f;
    char     label[DVR_VIEW_LABEL_MAX] = {};   // its time ("-0:30")
    bool     after   = false;  // after the cursor: kept until PLAY FROM HERE
};

// What a hint does when it's clicked / tapped (the frontend also maps its
// gamepad buttons to these).
enum class DvrViewAction : uint8_t
{
    none,
    back,          // one step back (frame / snapshot)
    forward,
    play_here,     // B: play on from the cursor
    watch,         // Y: play the recording / pause it
    back_to_now,   // A: back to the present
    other_lane,    // UP / DOWN: to the other lane
    jump_back,     // L: 5 s back (SCRUB); the phone's -5 S / ◀◀ (SNAPSHOTS: the oldest)
    jump_forward,  // R: 5 s on; the phone's +5 S / ▶▶ (SNAPSHOTS: the newest)
};

struct DvrViewHint
{
    const char*   key    = "";   // the button's name (gamepad); unused for touch
    const char*   label  = "";
    bool          lit    = false;   // on (WATCH while watching), or the touch screen's main button
    DvrViewAction action = DvrViewAction::none;
};

struct DvrViewText
{
    const char* title   = "";   // the chip: "◀◀ DVR" / "▶ WATCHING"
    const char* mode    = "";   // "SCRUB" / "SNAPSHOTS EVERY 10 S"
    const char* kept    = "";   // "4:12 KEPT" ("" = not shown)
    const char* readout = "";   // where the cursor is: "-0:27.8"
    const char* exit    = "";   // the exit button: "BACK TO NOW"
    const char* lane    = "";   // which lane, after the mode ("KEPT LANE"; "" = not shown)
    // The phone's panel (dvr_view_draw_phone) names its own buttons:
    const char* play_here     = "";   // PLAY FROM HERE
    const char* watch         = "";   // WATCH, or PAUSE while it plays
    const char* jump_back     = "";   // "-5 S" (SCRUB) / "◀◀" (SNAPSHOTS)
    const char* jump_forward  = "";   // "+5 S" / "▶▶"
    const char* lane_recorded = "";   // the lanes' tabs: RECORDED
    const char* lane_kept     = "";   //                  KEPT
};

struct DvrView
{
    DvrViewThumb thumbs[DVR_VIEW_MAX_THUMBS];
    uint32_t     count    = 0;
    uint32_t     selected = 0;
    float        between  = 0.0f;   // the cursor from the selected thumbnail toward the next (0..1)
    bool         playing  = false;  // a ▶ over the cursor
    float        aspect   = 8.0f / 7.0f;   // the pictures' width / height
    DvrViewHint  hints[DVR_VIEW_MAX_HINTS];
    uint32_t     hint_count = 0;
    bool         touch    = false;  // big buttons, no time under every thumbnail
    int32_t      bump_direction = 0;      // a move hit the end: -1 the oldest, 1 the newest (0: none)
    float        bump_seconds   = 0.0f;   // since it did (the bounce lasts DVR_VIEW_BUMP_SECONDS)

    // Two lanes. Places along the track are in thumbnails: the index of the
    // thumbnail at or before it plus the way to the next (0..1); before the
    // first or after the last they go on at the thumbnails' spacing.
    bool  lanes     = false;   // a kept lane (the track's height grows by a line)
    bool  on_upper  = false;   // the cursor is on the upper lane (the kept one)
    float fork_at   = 0.0f;    // where the lanes part
    float upper_end = 0.0f;    // where the upper (kept) lane ends
    float lower_end = 0.0f;    // where the lower (recorded) lane ends
};

constexpr float DVR_VIEW_BUMP_SECONDS = 0.45f;

// What the mouse / a finger did this frame.
struct DvrViewInput
{
    DvrViewAction action  = DvrViewAction::none;
    int32_t       clicked = -1;   // a thumbnail clicked (go there)
    // The phone's strip, dragged (Andrés, 2026-11-26): while a finger holds
    // it, how far it is from where it went down and how far it moved this
    // frame, in thumbnails (positive: the finger went right, toward older).
    bool          dragging   = false;
    float         drag_from  = 0.0f;
    float         drag_moved = 0.0f;
};

// Window pixels the strip takes at the bottom.
float dvr_view_height(const DvrView* view, int32_t scale);

// The strip, above every window (it is one: the DVR's GUI).
void dvr_view_draw(const DvrView* view, RetroUiFont* pixel_font, int32_t scale, const DvrViewText* text,
                   DvrViewInput* input);

// ---------------------------------------------------------------------------
// The phone (piccolo's phone UI; Andrés, 2026-11-26, mockup
// https://claude.ai/artifact/1bqKXHsjJcgiDSmSxey9K3): the same view at touch
// size, in touch_ui's look. Upright the game keeps its place at the top and
// the panel takes the room under it (where the on-screen pad was); sideways
// the game shrinks to the left with the strip under it and the buttons go
// in a column at the right. Five big pictures (the selected one bigger),
// dragged to move; a row of steps (-5 S, ◀, WATCH, ▶, +5 S); PLAY FROM HERE
// and BACK TO NOW across; RECORDED / KEPT tabs once there is a kept lane.
// Retro pixels at the touch scale (touch_ui_scale_for).

struct DvrPhoneAreas
{
    bool  upright = false;
    float game_x = 0.0f;     // the game's room (the frontend fits the picture in it)
    float game_y = 0.0f;
    float game_width = 0.0f;
    float game_height = 0.0f;
};

// Where the game goes on a `width` x `height` screen (retro pixels) for a
// picture `game_aspect` wide per unit of height, the panel taking the rest.
void dvr_view_phone_areas(const DvrView* view, float width, float height, float game_aspect, DvrPhoneAreas* areas);

// The panel and the strip, in an overlay window (they take the taps). Its
// areas from dvr_view_phone_areas with the same screen.
void dvr_view_draw_phone(const DvrView* view, RetroUiFont* pixel_font, int32_t scale, const DvrViewText* text,
                         const DvrPhoneAreas* areas, DvrViewInput* input);
