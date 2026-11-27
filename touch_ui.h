#pragma once

// The touch UI (piccoletto's screens, the touch pads of piccoletto and
// piccolo): the "retro but modern" look (Andrés, 2026-11-19): flat cards on
// the background, big touch targets, the pixel font and palettes, none of
// piccolo's desktop chrome (menu bar, title bars, bevels).
//
// Everything is laid out in retro pixels (one pixel of the 5x5 font) times
// `scale` window pixels, so edges stay on whole pixels and the font stays
// crisp. Text comes in two sizes: normal (1 retro pixel per font pixel) and
// big (1.5, rounded down to whole window pixels).

#include <cstdint>

#include "retro_ui.h"

#include "retro_ui_font.h"
struct TouchFingers;

// Where the gamepad's focus is this frame (a button reports itself while
// it has it), so a scrolled list can bring it into view. Reset each frame.
struct TouchFocus
{
    bool  has    = false;
    float y      = 0.0f;   // retro pixels
    float height = 0.0f;
};

struct RetroPad;

struct TouchUi
{
    RetroUiFont*        font    = nullptr;
    int32_t        scale   = 1;          // window pixels per retro pixel
    RetroUiPalette palette = {};
    // Every finger, for the pads (touch_fingers.h); nullptr while something
    // covers the screen (the ☰ sheet, HOME's sidebar): the pads take none then.
    TouchFingers*  fingers = nullptr;
    // What is held elsewhere (a real gamepad, as a RetroPad): those parts
    // light too; `action_lit` the action button. nullptr: none.
    const RetroPad* lit     = nullptr;
    bool           action_lit = false;
    TouchFocus*    focus   = nullptr;     // nullptr: not tracked
    // The touch pad's state this frame (pad_layout_draw adds the parts its
    // fingers hold); nullptr: not collected.
    RetroPad*      pad     = nullptr;
};

// The scale for a window: the short side holds at least 195 retro pixels
// (a phone's 390 dp at 2 dp per retro pixel). At least 1.
int32_t touch_ui_scale_for(float width, float height);

// A screen: one undecorated window over the whole display, in the
// palette's background color. Draw with the functions below in between.
// Sizes and positions below are in retro pixels from the screen's top left.
void touch_ui_begin_screen(const TouchUi* ui, const char* id);
void touch_ui_end_screen();

// An overlay (a sheet over a screen): a transparent window over the whole
// display, above the screen, so the screen behind takes no taps while it's
// up. Close with touch_ui_end_screen().
void touch_ui_begin_overlay(const TouchUi* ui, const char* id);

// The screen's size in retro pixels.
float touch_ui_width(const TouchUi* ui);
float touch_ui_height(const TouchUi* ui);

enum class TouchText
{
    normal,
    big,
};

// Text with its top left at (x, y). Returns its width in retro pixels.
float touch_ui_text(const TouchUi* ui, float x, float y, const char* text, uint32_t argb, TouchText size);
float touch_ui_text_width(const TouchUi* ui, const char* text, TouchText size);
// The line's height in retro pixels.
float touch_ui_text_height(const TouchUi* ui, TouchText size);

// The size to give ImGui::PushFont for an ImGui widget (a text field) so its
// text matches touch_ui_text's: ImGui scales pushed fonts by the style's
// font scales (frontends-common sets the DPI one to the display's scale).
float touch_ui_imgui_font_size(const TouchUi* ui, TouchText size);

// Text broken at spaces into lines no wider than `width`, its top left at
// (x, y). Returns its height in retro pixels; _height measures it alone.
float touch_ui_text_wrapped(const TouchUi* ui, float x, float y, float width, const char* text, uint32_t argb,
                            TouchText size);
float touch_ui_text_wrapped_height(const TouchUi* ui, float width, const char* text, TouchText size);

// A filled rectangle.
void touch_ui_rect(const TouchUi* ui, float x, float y, float width, float height, uint32_t argb);

// A texture (the toolkit's id, e.g. VideoTexture::ui_texture) stretched over
// the rectangle, [0, u_max] x [0, v_max] of it, sampled bilinear: with a
// texture already scaled up by a whole number with nearest, sharp bilinear.
void touch_ui_image_smooth(const TouchUi* ui, uint64_t texture, float x, float y, float width, float height,
                           float u_max, float v_max);

enum class TouchButton
{
    normal,    // the button face
    primary,   // the highlight color: the screen's main action
    ghost,     // only an outline
    card,      // a card you can tap (a row in a list): no lip
    plain,     // only the label (an icon in a header)
    field,     // a pad part's colors (on a pad's body): no lip
};

// A button you can tap: the label centered (or, for `card`, at the left).
// Normal and primary buttons have a lip at the bottom that goes away while
// pressed. True when tapped. `id` must be unique on the screen.
bool touch_ui_button(const TouchUi* ui, const char* id, float x, float y, float width, float height,
                     const char* label, TouchButton style, TouchText size);

// Whether gamepad navigation can reach a touch area (buttons always can):
// a backdrop that only catches taps (outside a sheet, a scroll area) can't.
enum class TouchNav
{
    reachable,
    taps_only,
};

// A touch area with nothing drawn (the caller draws it): true when tapped,
// *held while a finger (the mouse) is on it.
bool touch_ui_area(const TouchUi* ui, const char* id, float x, float y, float width, float height, bool* held,
                   TouchNav nav);

// A light the height of a letter at (x, y + 2): online green, or `argb`.
constexpr float TOUCH_UI_LAMP = 5.0f;
constexpr uint32_t TOUCH_UI_LAMP_ONLINE = 0xFF3AD15Au;
void touch_ui_lamp(const TouchUi* ui, float x, float y, uint32_t argb);

// Where text of `size` goes to look centered in a box `height` tall at `y`.
float touch_ui_centered_text_y(const TouchUi* ui, float y, float height, TouchText size);

// The gap between the buttons of touch_ui_choices and touch_ui_stepper.
constexpr float TOUCH_UI_GAP = 4.0f;

// A row of buttons, one per choice, filling `width`; the current one is
// primary (highlighted). Returns the tapped choice, or -1. The buttons' ids
// are `id` plus their index.
int32_t touch_ui_choices(const TouchUi* ui, const char* id, const char* const* names, int32_t count, int32_t current,
                         float x, float y, float width, float height);

// [-] value [+]: square buttons `height` wide at both ends, the value
// centered in a field between them. Returns -1, +1 or 0.
int32_t touch_ui_stepper(const TouchUi* ui, const char* id, const char* value, float x, float y, float width,
                         float height);

// A rectangle's edge, `thickness` retro pixels inside it (a picked row, a
// group in EDIT LAYOUT).
void touch_ui_outline(const TouchUi* ui, float x, float y, float width, float height, float thickness, uint32_t argb);

// Only what's inside (x, y, width, height) is drawn and takes taps until the
// pop (a scrolling list under a fixed header).
void touch_ui_push_clip(const TouchUi* ui, float x, float y, float width, float height);
void touch_ui_pop_clip();
