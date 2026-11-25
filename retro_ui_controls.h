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
    bool        clip_label    = false;     // a label wider than its column is cut there, not
                                           // drawn under the control (give the whole one in `help`)
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

// A button where the cursor is, its id kept while its label changes (the
// language): focus and keyboard navigation stay on it.
bool retro_ui_id_button(const char* id, const char* label);

// A square button as tall as a control with a triangle pointing up (or
// down): moves an item along a list.
bool retro_ui_move_button(const char* id, bool up);

// The width of `text` in the current font.
float retro_ui_text_width(const char* text);

// The label column's width for labels up to `widest` wide (text and spacing).
float retro_ui_label_column_width(float widest);

// Greyed-out text with a tooltip (`help` may be nullptr).
void retro_ui_dim_text(const char* text, const char* help);
// The same, lined up with the controls beside it (as retro_ui_label).
void retro_ui_dim_label(const char* text, const char* help);

// Controls. Each fills the width left on its line (less the end button's
// room inside a row, and a reserved button's). True when the value changed.

// The next control is `width` wide instead (a field sized for what it holds).
void retro_ui_next_width(float width);

bool retro_ui_check(const char* id, const char* label, bool* value);
// The width of a check box with `label` (its box and the label).
float retro_ui_check_width(const char* label);
bool retro_ui_number_field(const char* id, int32_t* value, int32_t step);
// The same, `fast_step` at a time while Ctrl is held on its - / + buttons
// (retro_ui_number_field's is 8).
bool retro_ui_number_field_steps(const char* id, int32_t* value, int32_t step, int32_t fast_step);
// The width of a number field that shows up to `widest` (e.g. "9999"), its
// - and + buttons included: a field sized for what it holds (retro_ui_next_width).
float retro_ui_number_field_width(const char* widest);
bool retro_ui_slider(const char* id, float* value, float min, float max, const char* format);
// The same for whole numbers (`format` e.g. "%d%%").
bool retro_ui_slider_int(const char* id, int32_t* value, int32_t min, int32_t max, const char* format);

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
    bool     decimal        = false;   // 0-9 and . + - * / can be typed (a number, the way the
                                       // pairing code fields always took it)
    uint32_t max_characters = 0;       // 0: as many as `buffer` holds
    bool     focus          = false;   // the keyboard focus goes to it now (typing starts at once)
    bool     read_only      = false;   // shown (selected, scrolled) but not edited: a link to copy
    bool     uppercase      = false;   // letters typed come out in capitals (a name the pixel font shows so)
    const char* hint        = nullptr; // shown dimmed while it's empty; nullptr: none
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
// The same, also true when the item already selected is picked again (it
// does something then: a shared folder picked again goes back to its top).
bool retro_ui_choice_item_repick(uint32_t index, const char* label, bool selected);
void retro_ui_end_choice();
// Inside a choice: its list opened this frame (read what it lists afresh).
bool retro_ui_choice_opened();

// An item of a list (retro_ui_begin_list): `label`, highlighted while
// `selected`, and `note` (nullptr: none) dimmed at the list's right end (e.g.
// PAIRED). True when it was clicked or activated, selected or not.
bool retro_ui_list_item(uint32_t index, const char* label, bool selected, const char* note);

// The same (no note), telling a double click apart: `twice` on its second
// click (it opens or loads the item; the first one was `once`).
enum class RetroUiClick
{
    none,
    once,   // clicked or activated
    twice,  // double-clicked
};
RetroUiClick retro_ui_list_item_click(uint32_t index, const char* label, bool selected);

// An item with `detail` against the right edge of the list's contents (a
// file's size or date; it moves left of a scroll bar), in the item's color.
// `dim`: both greyed out (the item can still be picked). True when clicked
// or activated, selected or not.
bool retro_ui_list_item_detail(uint32_t index, const char* label, bool selected, const char* detail, bool dim);

// The height of a list (retro_ui_begin_list) for `rows` rows of controls,
// its padding included.
float retro_ui_list_height(uint32_t rows);

// A long list (thousands of files) where only the items in view are drawn;
// keyboard / gamepad navigation still reaches all of them:
//   retro_ui_begin_clipped(count);
//   while (retro_ui_clipped_step(&first, &end)) { draw items first..end-1 }
// One at a time (not nested); the steps end it.
void retro_ui_begin_clipped(uint32_t count);
bool retro_ui_clipped_step(uint32_t* first, uint32_t* end);

// Buttons at the right end of a line, in order. `same_line`: on the line of
// the last item (e.g. after a check box); otherwise on a line of their own.
// Returns the index of the one pressed, or -1.
struct RetroUiEndButton
{
    const char* id      = nullptr;
    const char* label   = nullptr;
    bool        shown   = true;    // false: not drawn, and no room kept
    bool        enabled = true;    // false: greyed out
};
int32_t retro_ui_end_buttons(const RetroUiEndButton* buttons, uint32_t count, bool same_line);

// Buttons sharing the width of a line equally, whatever their labels: a row
// of `count` columns `width` wide (0: the width left on the line), the
// spacing between them included, so rows of different counts over the same
// width line up at both ends (QUICK MENU's actions, four a row, and SAVE /
// LOAD / GAME DVR, three). Returns the index of the one pressed, or -1.
struct RetroUiRowButton
{
    const char* id      = nullptr;   // stable while the label changes (the language)
    const char* label   = nullptr;
    bool        enabled = true;      // false: greyed out
    const char* help    = nullptr;   // its tooltip, shown also while greyed out (nullptr: none)
};
int32_t retro_ui_equal_buttons(const RetroUiRowButton* buttons, uint32_t count, float width);
// The width a row of `columns` equal buttons needs so the widest of these
// `count` labels fits in a column (the spacing between columns included).
float retro_ui_equal_buttons_width(const RetroUiRowButton* buttons, uint32_t count, uint32_t columns);

// A button of a set where one is chosen (QUICK MENU's save slots): `label`
// on a `width` x `height` button. True when pressed, and while it has the
// keyboard focus: the arrows move the choice along the set.
//
// With a picture (`picture_height` > 0): a black area that tall across the
// top, the label under it. `texture` (the toolkit's texture id; 0: none,
// `empty` dimmed in the middle instead) is drawn `image_width` x
// `image_height` in the middle of that area, [0, u_max] x [0, v_max] of it,
// sampled bilinear (a thumbnail smaller than its picture).
struct RetroUiTile
{
    float    width  = 0.0f;
    float    height = 0.0f;
    uint32_t face   = 0;       // 0xAARRGGBB; 0: the theme's (another color sets it apart, e.g. a slot in use)
    bool     chosen = false;   // edged, `edge` pixels wide
    float    edge   = 0.0f;
    bool     focus  = false;   // the keyboard focus goes to it now (one chosen otherwise: 1-9, opening)

    float       picture_height = 0.0f;
    uint64_t    texture        = 0;
    float       image_width    = 0.0f;
    float       image_height   = 0.0f;
    float       u_max          = 1.0f;
    float       v_max          = 1.0f;
    const char* empty          = nullptr;   // shown without a texture (nullptr: nothing)
};
bool retro_ui_tile(const char* id, const char* label, const RetroUiTile* tile);

// A table with a row of headers, e.g. in a list (retro_ui_begin_list):
//   if (retro_ui_begin_table("id", columns, count, padding)) {
//       for each row: if (retro_ui_table_row(i, first, i == selected)) selected = i;
//                     retro_ui_table_cell(second); ...
//       retro_ui_end_table();
//   }
// `cell_padding`: the room on each side of a cell's text, across (a gap
// between columns, so a long text doesn't run into the next one).
struct RetroUiColumn
{
    const char* label = nullptr;   // its header
    float       width = 0.0f;      // 0: what the other columns leave
};
bool retro_ui_begin_table(const char* id, const RetroUiColumn* columns, uint32_t count, float cell_padding);
// A row starts: its first cell is `label`, selectable across the whole row.
// True when clicked or activated, selected or not.
bool retro_ui_table_row(uint32_t index, const char* label, bool selected);
// The row's next cell.
void retro_ui_table_cell(const char* text);
void retro_ui_end_table();

// A question that must be answered: a modal window in the middle of the
// screen over everything, `text` wrapped, then the YES and NO buttons. Call
// it every frame; it opens while `asking` and closes once answered. Escape
// (B on a gamepad) answers NO.
enum class RetroUiAnswer
{
    none,   // not answered (yet)
    yes,
    no,
};
struct RetroUiQuestion
{
    const char* id    = nullptr;   // stable while the title changes
    const char* title = nullptr;
    const char* text  = nullptr;
    const char* yes   = nullptr;   // the buttons' labels
    const char* no    = nullptr;
    float       width = 0.0f;      // set when it opens; 0: as wide as its contents
};
RetroUiAnswer retro_ui_question(const RetroUiQuestion* question, bool asking);

// A modal window like the question's with contents of its own (more than
// two answers). Call it every frame; it opens while `open`, and stays open
// until retro_ui_close_modal (the caller decides when it's answered):
//   if (retro_ui_begin_modal(&modal, open)) {
//       ...; if (answered) retro_ui_close_modal();
//       retro_ui_end_modal();
//   }
// Escape isn't handled: check retro_ui_key_pressed (B on a gamepad sends it).
struct RetroUiModal
{
    const char* id    = nullptr;   // stable while the title changes
    const char* title = nullptr;
    float       width = 0.0f;      // set when it opens; 0: as wide as its contents
    float       height = 0.0f;     // set when it opens; 0: as tall as its contents (one size for
                                   // contents that come and go keeps it from jumping)
};
bool retro_ui_begin_modal(const RetroUiModal* modal, bool open);
void retro_ui_close_modal();   // inside it: it closes (gone next frame)
void retro_ui_end_modal();     // only when retro_ui_begin_modal was true

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
    bool        focus_on_open = false; // given the focus when it opens (keys reach it at once)
    bool        scrolls   = true;      // false: never scrolls itself, no scroll bar (its parts
                                       // that need to scroll do it in their own areas)
};

// Draws the window's frame. True while its contents show; call
// retro_ui_window_end() either way. *closed is set when the X was pressed
// (may be nullptr).
bool retro_ui_window_begin(const RetroUiWindow* window, int32_t scale, bool* closed);
void retro_ui_window_end();

// The window being drawn opened this frame (or reopened after being closed).
bool retro_ui_window_appearing();

// The window being drawn, or a part of it, has the keyboard focus: keys it
// reacts to on its own (retro_ui_key_pressed) are meant for it.
bool retro_ui_window_focused();

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
// The same, open or closed as `open` says every frame (the caller decides:
// one section of several open at a time). True while it's open: false once
// its header was clicked to close it.
bool retro_ui_section_header_open(const char* id, const char* label, bool open);

// Tabs: a bar of tabs, one shown at a time, its contents below:
//   if (retro_ui_begin_tabs("id")) {
//       if (retro_ui_begin_tab("a", label, select_a)) { ...; retro_ui_end_tab(); }
//       ...
//       retro_ui_end_tabs();
//   }
// `select`: the tab is chosen now (PAGE LEFT / RIGHT, retro_ui_tab_switch, or
// a window opened on it); otherwise the user's click chooses. Bars can nest
// (a tab with tabs of its own). Ids stay while the labels change (the language).
bool retro_ui_begin_tabs(const char* id);
void retro_ui_end_tabs();        // only when retro_ui_begin_tabs was true
bool retro_ui_begin_tab(const char* id, const char* label, bool select);   // true while it's the one shown
void retro_ui_end_tab();         // only when retro_ui_begin_tab was true

// A part of the window, `width` x `height` (0: what's left), that scrolls on
// its own when its contents are taller; no background or edge of its own (the
// window's shows through). True while its contents show; call
// retro_ui_end_scroll_area() either way.
bool retro_ui_begin_scroll_area(const char* id, float width, float height);
void retro_ui_end_scroll_area();
// The same with the field color behind it (no edge, no padding): a block of
// rows set apart from the rest of the window (the running core's options).
// Ended by retro_ui_end_scroll_area() too.
bool retro_ui_begin_scroll_field(const char* id, float width, float height);

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
void retro_ui_label_colored(uint32_t argb, const char* text);   // the same in a color of its own
// Text cut short with "…" (whole characters) when it's wider than `width`,
// all of it in the tooltip then. _label: lined up with the controls beside it.
void retro_ui_text_fit(const char* text, float width);
void retro_ui_label_fit(const char* text, float width);

void retro_ui_same_line();              // the next item goes on this line
void retro_ui_same_line_at(float x);    // ...starting `x` from the window's left edge (a column)
void retro_ui_same_line_gap(float gap); // ...`gap` after the last item (set apart from it)
void retro_ui_set_cursor_x(float x);    // the next item starts `x` from the window's left edge
                                        // (on the line it's on: centered in a column, say)
void retro_ui_down_to(float y);         // the next item starts at least `y` down from the window's top
                                        // (no higher: something kept in one place, whatever is above it)
// Empty room `width` x `height` at the cursor, as an item that draws nothing
// (a line kept as tall as a control, a column kept as wide as its arrows).
void retro_ui_space(float width, float height);
void retro_ui_spacing();
void retro_ui_separator();
void retro_ui_indent();
void retro_ui_unindent();
void retro_ui_indent_by(float width);     // ...`width` instead of the theme's step (pair with the same width)
void retro_ui_unindent_by(float width);

// The scroll area (or window) scrolls so the last line is in its middle: a
// row waiting for a key stays in view while BIND ALL goes down the list.
void retro_ui_scroll_here();

// Greys out (and disables) what comes until the matching end, when `disabled`.
void retro_ui_begin_disabled(bool disabled);
void retro_ui_end_disabled();
// The last item's tooltip, shown also while it's greyed out (it says why).
void retro_ui_disabled_item_tooltip(const char* text);
// The last item is under the mouse, or has the focus while navigating with
// a gamepad or keys (a click focuses too, but that focus doesn't count once
// the mouse has left it): what a picture beside a list follows.
bool retro_ui_item_pointed();

// Ids for repeated controls (one per item of a list).
void retro_ui_push_id(const char* id);
void retro_ui_push_index(uint32_t index);
void retro_ui_pop_id();

float retro_ui_width_left();                 // the width left on this line
float retro_ui_height_left();                // the height left below the cursor (in a window or an area)
float retro_ui_cursor_y();                   // where the next item goes, down from the window's top
                                             // (what's between two of these is that tall)
float retro_ui_cursor_x();                   // ...and right from the window's left edge
float retro_ui_right_edge();                 // the right edge of the window's (or area's) contents,
                                             // from its left edge: retro_ui_same_line_at(edge - width)
                                             // ends an item `width` wide against it
float retro_ui_line_height();                // a control's height
float retro_ui_line_step();                  // ...and the spacing below it: rows of controls are this far apart
float retro_ui_text_line_step();             // a line of text and the spacing below it
float retro_ui_item_spacing();               // between two items on a line
float retro_ui_button_width(const char* label);
float retro_ui_font_size();                  // the current font's height (its glyphs are drawn this tall)
float retro_ui_bottom_line_y();              // where a control on the window's last line goes, down from
                                             // its top (buttons kept at the bottom: retro_ui_down_to it)
void  retro_ui_window_size(float* width, float* height);   // the window being drawn, its frame included

// The toolkit's clock: seconds since it started (an animation, a COPIED shown
// for two seconds).
double retro_ui_seconds();

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
// VideoTexture::ui_texture) stretched over the rectangle; [0, u_max] x
// [0, v_max] of it is drawn, tinted by `argb` (0xFFFFFFFF: as it is).
void retro_ui_canvas_image(uint64_t texture, float x, float y, float width, float height, float u_max, float v_max,
                           uint32_t argb);
// The same, sampled bilinear instead of nearest (the rest of the window stays
// nearest). With a texture already scaled up by a whole number with nearest,
// this is sharp bilinear: sharp pixels, soft only where a pixel falls
// between two screen pixels.
void retro_ui_canvas_image_smooth(uint64_t texture, float x, float y, float width, float height, float u_max,
                                  float v_max, uint32_t argb);
// Text in the current font, its top left corner at (x, y).
void retro_ui_canvas_text(float x, float y, const char* text, uint32_t argb);
// A rectangle anywhere on the window being drawn, in pixels from the window's
// top left corner (its frame's), taking no room: a mark kept in one place
// whatever the contents (a page indicator above the buttons).
void retro_ui_window_rect(float x, float y, float width, float height, uint32_t argb);

// Port LEDs and ping bars (port_indicators.h) as items on the line; `block`:
// window pixels per retro pixel. Off LEDs and unlit bars are the palette's
// panel_dark.
// `count` LEDs, the one at `lit` lit (-1: none), centered on a line of text
// as tall as the font.
void retro_ui_port_leds(uint32_t count, int32_t lit, float block);
// The ping bars, on an item as tall as a control, their bottoms two retro
// pixels above its bottom; all off while not `known`.
void retro_ui_ping_bars(bool known, uint32_t ping_ms, float block);

// The ring that closes while a button is held to bind it (`progress` 0 .. 1),
// as text in `argb` after the last item on its line; nothing at 0.
void retro_ui_hold_ring(float progress, uint32_t argb);

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
