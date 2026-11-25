#pragma once

// Controls drawn inside a window, and the row they sit in, with no ImGui in
// their signatures: plain ids, labels, values and flags. A frontend's windows
// draw with these and leave the layout (widths, spacing, alignment, IDs,
// greying out) to retroui. Another GUI toolkit would reimplement this file.
// Use inside the theme (retro_ui_push_theme).
//
// A row:
//
//   M LABEL           [ control                         ] [END]
//
//   M      a marker of one glyph in its own color (e.g. "C" / "G" for a
//          value set per core / per game), with a tooltip. Its space is
//          kept when there's none, so rows line up.
//   LABEL  in a column `label_width` wide, so the controls line up.
//   END    a button at the right edge (e.g. RESET). The controls leave room
//          for it when the row has one, shown or hidden.
//
// Ids are separate from labels: a label can change (the language) and the
// control stays the same one.

#include <cstddef>
#include <cstdint>

#if defined(__GNUC__) || defined(__clang__)
#define RETRO_UI_PRINTF(format_index, first_argument) __attribute__((format(printf, format_index, first_argument)))
#else
#define RETRO_UI_PRINTF(format_index, first_argument)
#endif

struct RetroUiRow
{
    const char* id            = nullptr;   // unique among the window's rows
    bool        marker_column = true;      // false: no marker space at all
    const char* marker        = nullptr;   // nullptr: none
    uint32_t    marker_color  = 0;         // 0xAARRGGBB
    const char* marker_help   = nullptr;
    const char* label         = nullptr;   // nullptr: none (a checkbox carries its own)
    const char* help          = nullptr;   // the label's tooltip; nullptr: none
    float       label_width   = 0.0f;      // the label column; 0: the control follows the label
    bool        enabled       = true;      // false: greyed out, `disabled_help` as the tooltip
    const char* disabled_help = nullptr;
    const char* end_button    = nullptr;   // nullptr: none, and no room kept
    bool        end_button_hidden = false; // not drawn, but its room is kept (the controls
                                           // don't change width when it appears)
    const char* end_button_help = nullptr;
};

// The row starts: marker, label, then the controls follow on the same line.
void retro_ui_row_begin(const RetroUiRow* row);

// The row ends: the end button, when it has one, isn't hidden and the row is
// enabled. True when it was pressed.
bool retro_ui_row_end(const RetroUiRow* row);

// The next control leaves room after it for a button with this label (drawn
// with retro_ui_row_button). Inside or outside a row.
void retro_ui_reserve_button(const char* label);

// A button right after the last control, on the same line.
bool retro_ui_row_button(const char* id, const char* label);

// The width of `text` in the current font.
float retro_ui_text_width(const char* text);

// The label column's width for labels up to `widest` wide (text and spacing).
float retro_ui_label_column_width(float widest);

// Greyed-out text with a tooltip (`help` may be nullptr).
void retro_ui_dim_text(const char* text, const char* help);

// Controls. Each fills the width left on its line (less the end button's
// room inside a row, and a reserved button's). True when the value changed.

bool retro_ui_check(const char* id, const char* label, bool* value);
bool retro_ui_number_field(const char* id, int32_t* value, int32_t step);
bool retro_ui_slider(const char* id, float* value, float min, float max, const char* format);

// A swatch (a click opens a color picker) and the value as #RRGGBB.
bool retro_ui_color_field(const char* id, uint32_t* argb);

// A text field showing `value` (or `hint` while empty). True once, when an
// edit ends; the new text is then in `edited`.
bool retro_ui_text_field(const char* id, const char* hint, const char* value, char* edited, size_t edited_size);

// A text field that edits `buffer` in place (the caller keeps the text while
// it's typed). True when Enter was pressed in it (the caller acts on the text:
// OK). Lengths are counted in UTF-8 characters, not bytes, as a core asking
// for text counts them.
struct RetroUiEditField
{
    bool     digits_only    = false;   // only 0-9 can be typed (a number)
    uint32_t max_characters = 0;       // 0: as many as `buffer` holds
    bool     focus          = false;   // the keyboard focus goes to it now (typing starts at once)
};
bool retro_ui_edit_field(const char* id, char* buffer, size_t buffer_size, const RetroUiEditField* field);

// A read-only field showing `value` (or `hint` while empty) and "..." after
// it. True when either was clicked (the caller opens a picker).
bool retro_ui_picker_field(const char* id, const char* hint, const char* value, const char* help);

// A list to pick one from:
//   if (retro_ui_begin_choice("id", preview)) {
//       for each item: if (retro_ui_choice_item(i, label, i == current)) picked = i;
//       retro_ui_end_choice();
//   }
// retro_ui_choice_item is true when that item was picked.
bool retro_ui_begin_choice(const char* id, const char* preview);
bool retro_ui_choice_item(uint32_t index, const char* label, bool selected);
void retro_ui_end_choice();

// An item of a list (retro_ui_begin_list): `label`, highlighted while
// `selected`, and `note` (nullptr: none) dimmed at the list's right end (e.g.
// PAIRED). True when it was clicked or activated, selected or not.
bool retro_ui_list_item(uint32_t index, const char* label, bool selected, const char* note);

// ---------------------------------------------------------------------------
// Windows, text and layout. The window's frame is the theme's (light edge,
// pixel X); positions and sizes are in window pixels.

struct RetroUiWindow
{
    const char* id        = nullptr;   // stable: the window stays the same one when its title changes
    const char* title     = nullptr;   // shown in the title bar
    float       width     = 0.0f;      // 0: as wide as its contents
    float       height    = 0.0f;      // 0: as tall as its contents
    bool        centered  = false;     // kept in the middle of the screen (not movable)
    bool        centered_on_open = false;   // in the middle when it opens, movable afterwards
    float       x         = 0.0f;      // neither centered: where it first opens
    float       y         = 0.0f;
    bool        closable  = true;      // the X in the title bar
    bool        resizable = false;
    bool        focus     = false;     // given the focus every frame (a question that must be answered)
};

// Draws the window's frame. True while its contents show; call
// retro_ui_window_end() either way. *closed is set when the X was pressed
// (may be nullptr).
bool retro_ui_window_begin(const RetroUiWindow* window, int32_t scale, bool* closed);
void retro_ui_window_end();

// The window being drawn goes above every other one, now (an editor that
// must not hide under the menus). Any window: retroui's or an overlay
// (touch_ui_begin_overlay).
void retro_ui_bring_window_to_front();

// What a window adds around its contents: the padding on both sides
// (`width`), and the padding and the title bar (`height`). A window sized to
// fit something adds these.
void retro_ui_window_chrome(float* width, float* height);

// A section of a window that can be folded away: its header, a bar with an
// arrow and `label`. True while it's open (draw its contents then).
// `open_at_first`: open the first time it's drawn.
bool retro_ui_section_header(const char* id, const char* label, bool open_at_first);

// A part of the window, `width` x `height` (0: what's left), that scrolls on
// its own when its contents are taller; no background or edge of its own (the
// window's shows through). True while its contents show; call
// retro_ui_end_scroll_area() either way.
bool retro_ui_begin_scroll_area(const char* id, float width, float height);
void retro_ui_end_scroll_area();

// What's drawn between these is one block: retro_ui_same_line() after it puts
// the next item right of all of it, at its top (a column beside another).
void retro_ui_begin_group();
void retro_ui_end_group();

void retro_ui_text(const char* text);
void retro_ui_text_format(const char* format, ...) RETRO_UI_PRINTF(1, 2);   // printf-style
void retro_ui_text_wrapped(const char* text);              // wraps at the window's edge
// Wraps `wrap_x` from the window's left edge (0: at the window's edge), in
// the text color or greyed out: a paragraph kept narrower than the window.
void retro_ui_text_wrapped_at(const char* text, float wrap_x);
void retro_ui_dim_text_wrapped_at(const char* text, float wrap_x);
void retro_ui_text_colored(uint32_t argb, const char* text);
void retro_ui_label(const char* text);                     // lined up with the controls beside it

void retro_ui_same_line();              // the next item goes on this line
void retro_ui_same_line_at(float x);    // ...starting `x` from the window's left edge (a column)
void retro_ui_spacing();
void retro_ui_separator();
void retro_ui_indent();
void retro_ui_unindent();

// Greys out (and disables) what comes until the matching end, when `disabled`.
void retro_ui_begin_disabled(bool disabled);
void retro_ui_end_disabled();

// Ids for repeated controls (one per item of a list).
void retro_ui_push_id(const char* id);
void retro_ui_push_index(uint32_t index);
void retro_ui_pop_id();

float retro_ui_width_left();                 // the width left on this line
float retro_ui_line_height();                // a control's height
float retro_ui_line_step();                  // ...and the spacing below it: rows of controls are this far apart
float retro_ui_text_line_step();             // a line of text and the spacing below it
float retro_ui_item_spacing();               // between two items on a line
float retro_ui_button_width(const char* label);
float retro_ui_font_size();                  // the current font's height (its glyphs are drawn this tall)

// ---------------------------------------------------------------------------
// Drawing in a window (a QR code, a picture with marks over it): a canvas
// takes `width` x `height` at the cursor, like a control would, and the
// retro_ui_canvas_* calls after it draw there, in window pixels from its top
// left corner, until the next canvas. Its corner is on a whole pixel. Colors
// are ARGB (alpha used).
void retro_ui_canvas(float width, float height);
void retro_ui_canvas_rect(float x, float y, float width, float height, uint32_t argb);
// A rectangle's outline, `thickness` pixels wide.
void retro_ui_canvas_outline(float x, float y, float width, float height, uint32_t argb, float thickness);
// A texture (as retro_ui_layer_image: the toolkit's texture id, e.g.
// VideoTexture::imgui_id) stretched over the rectangle; [0, u_max] x
// [0, v_max] of it is drawn, tinted by `argb` (0xFFFFFFFF: as it is).
void retro_ui_canvas_image(uint64_t texture, float x, float y, float width, float height, float u_max, float v_max,
                           uint32_t argb);

// ---------------------------------------------------------------------------
// Keys a window reacts to on its own (Escape closes it, a number key picks a
// slot), beyond the interface keys the frame loop drives every window with.

enum class RetroUiKey
{
    escape,
    enter,
    space,
    tab,
    backspace,
    digit_0, digit_1, digit_2, digit_3, digit_4, digit_5, digit_6, digit_7, digit_8, digit_9,
    a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
};

// `key` went down this frame. `repeat`: also true again while it's held, at
// the keyboard's repeat rate (as a text field repeats a held key).
bool retro_ui_key_pressed(RetroUiKey key, bool repeat);
