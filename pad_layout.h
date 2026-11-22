#pragma once

// The touch pad's parts (GAMEPAD, REMOTE PLAY): blocks like piccolo's pad
// picture (retroui's pad_picture), lit yellow while held; a held stick's
// nub follows the finger.
//
// A layout is a table of parts in the mockups' dp (a 390 x 844 portrait
// phone, 844 x 390 landscape...), fitted to the real screen (PadFrame), plus
// what the user changed in EDIT LAYOUT: each group of parts (the d-pad, the
// face buttons, a stick...) moved and resized, and the whole layout's alpha
// (for overlays over the game). Saved per layout in settings/layouts.cfg.
//
// Multi-touch (touch_fingers.h): a button is held while any free finger is
// on it (sliding from one to the next works, like across the d-pad); a stick
// belongs to the finger that went down on it until that finger is lifted,
// and that finger presses nothing else.

#include <cstdint>

#include "touch_ui.h"

struct SettingsFile;

enum class PadPartKind
{
    button,   // a block with its label
    stick,    // a block with a nub in the middle
    action,   // the frontend's own button (piccoletto: HOTKEY, with STREAM access;
              // piccolo: QUICK MENU), labelled by it; shown only when it gives a label
    menu,     // a second one of the frontend's (piccolo: MAIN MENU), the same way
};

// What moves together in EDIT LAYOUT.
enum class PadGroup
{
    left_shoulders,
    right_shoulders,
    dpad,
    face,
    center,        // SELECT, START, the piccolo menu
    left_stick,
    right_stick,
    count,
};
constexpr int32_t PAD_GROUP_COUNT = static_cast<int32_t>(PadGroup::count);

// "D-PAD" etc. (shown while editing) and "dpad" etc. (the settings keys).
const char* pad_group_label(PadGroup group);
const char* pad_group_key(PadGroup group);

struct PadPart
{
    const char* id;      // unique on the screen ("##a")
    const char* label;   // "" = none
    float       x, y, width, height;   // dp in the reference layout
    PadPartKind kind;
    TouchText   size;
    PadGroup    group;
};

// A group's change from the default: moved by (dx, dy) dp, resized around
// its middle.
struct PadGroupEdit
{
    float dx    = 0.0f;
    float dy    = 0.0f;
    float scale = 1.0f;
};

struct PadLayout
{
    const char*    name          = "";        // settings section, e.g. "gamepad_portrait"
    const PadPart* parts         = nullptr;   // the defaults
    int32_t        count         = 0;
    float          width         = 0.0f;      // reference size, dp
    float          height        = 0.0f;
    float          default_alpha = 1.0f;
    bool           overlay       = false;     // over the game's picture: parts in the button color, to stand out
    PadGroupEdit   groups[PAD_GROUP_COUNT] = {};
    float          alpha         = 1.0f;
    bool           alpha_set     = false;     // set in EDIT LAYOUT; else the default follows SETTINGS
    uint32_t       held          = 0;         // the parts a finger held last frame, a bit each (part index < 32)
    const char*    action_label   = nullptr;  // the action button's label, set by the frontend (nullptr: not shown)
    bool           action_held    = false;    // a finger is on the action button (this frame)
    bool           action_pressed = false;    // ...and went down on it this frame
    const char*    menu_label     = nullptr;  // the menu button's label, the same way
    bool           menu_held      = false;
    bool           menu_pressed   = false;
};

constexpr int32_t PAD_MAX_PARTS = 32;   // `held`'s bits

constexpr float PAD_SCALE_MIN = 0.5f;
constexpr float PAD_SCALE_MAX = 2.0f;
constexpr float PAD_ALPHA_MIN = 0.1f;

// Back to the defaults (every group in place, the default alpha).
void pad_layout_reset(PadLayout* layout);

// The user's changes from / to the layouts file (its own section).
void pad_layout_load(PadLayout* layout, const SettingsFile* file);
void pad_layout_save(const PadLayout* layout, SettingsFile* file);

// The opacity from the frontend's setting: the default, and the alpha too
// unless EDIT LAYOUT set one for this layout.
void pad_layout_set_default_alpha(PadLayout* layout, float alpha);

// Whether any part belongs to `group` (a layout without sticks has none).
bool pad_layout_has_group(const PadLayout* layout, PadGroup group);

// A part's rectangle with its group's change applied, in reference dp.
void pad_layout_part_rect(const PadLayout* layout, int32_t part, float* x, float* y, float* width, float* height);

// Part `part` as it's shown, in `shown` (the action / menu button with the
// frontend's label); false = not shown (an action / menu button without one).
bool pad_layout_shown_part(const PadLayout* layout, int32_t part, PadPart* shown);

// A group's bounding box with its change applied, in reference dp.
void pad_layout_group_rect(const PadLayout* layout, PadGroup group, float* x, float* y, float* width, float* height);

// Reference dp -> the screen's retro pixels: x_retro = x + dp * dp_scale.
struct PadFrame
{
    float x;
    float y;
    float dp_scale;   // retro pixels per dp
};

// The frame that fits a `width` x `height` dp layout into (x, y, room_width,
// room_height) retro pixels, centered.
PadFrame pad_frame_fit_into(float width, float height, float x, float y, float room_width, float room_height);

// The same over the whole screen.
PadFrame pad_frame_fit(const TouchUi* ui, float width, float height);

// As big as it fits on the screen, centered across and at the bottom (the
// landscape pads: what's left over goes under the title bar's row).
PadFrame pad_frame_fit_bottom(const TouchUi* ui, float width, float height);

// Reference dp to the screen's retro pixels.
float pad_x(const PadFrame* frame, float dp);
float pad_y(const PadFrame* frame, float dp);
float pad_size(const PadFrame* frame, float dp);

// Draws the layout's parts (at its alpha, the held ones lit: from ui->fingers
// and ui->lit); the action button only with layout->action_label set,
// labelled with it (layout->action_held / _pressed say when). Parts newly
// held by a finger this frame are added to ui->fingers->presses.
void pad_layout_draw(const TouchUi* ui, const PadFrame* frame, PadLayout* layout);

// One part at screen position (retro pixels), at `alpha`; `held` lit;
// `overlay`: in the button color (over a picture) instead of the field color
// (on the pad's body). A stick's nub is drawn at (stick_x, stick_y), -1..1
// from the middle.
void pad_part_draw(const TouchUi* ui, const PadPart* part, float x, float y, float width, float height, float alpha, bool held,
                   bool overlay, float stick_x, float stick_y);

// A flat block (the pad's body, a bezel) in reference dp.
void pad_block(const TouchUi* ui, const PadFrame* frame, float x, float y, float width, float height, uint32_t argb);

// A button in reference dp (touch_ui_button with the frame applied).
bool pad_button(const TouchUi* ui, const PadFrame* frame, const char* id, float x, float y, float width, float height,
                const char* label, TouchButton style, TouchText size);

// Text at reference dp (x, y); `centered`: x is the middle.
void pad_text(const TouchUi* ui, const PadFrame* frame, float x, float y, const char* text, uint32_t argb, TouchText size,
              bool centered);

