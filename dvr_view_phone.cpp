// The DVR on a phone (dvr_view.h, "The phone"): touch_ui's look at touch
// size. Draw only, like the desktop's strip: the frontend moves through its
// history and fills the view in.

#include "dvr_view.h"

#include <cmath>
#include <cstdio>

#include "imgui.h"
#include "retro_ui.h"
#include "touch_ui.h"

// Retro pixels (times `scale` window pixels).
constexpr float MARGIN     = 8.0f;    // from the screen's sides
constexpr float GAP        = 6.0f;    // between the panel's rows
constexpr float HEADER     = 16.0f;
constexpr float CHIP_PAD   = 5.0f;
constexpr float LANES      = 18.0f;   // the RECORDED / KEPT tabs
constexpr float THUMB_GAP  = 4.0f;
constexpr float BIG        = 1.35f;   // the selected picture, this much bigger
constexpr float BORDER     = 1.0f;
constexpr float LABEL      = 10.0f;   // a picture's time under it
constexpr float TRACK      = 10.0f;
constexpr float STEP       = 26.0f;   // the row of steps
constexpr float MAIN       = 32.0f;   // PLAY FROM HERE
constexpr float EXIT       = 24.0f;   // BACK TO NOW
constexpr float SIDE       = 150.0f;  // sideways: the buttons' column
constexpr float TAP_SLOP   = 0.3f;    // a finger that moved less than this (in pictures) tapped
constexpr float SIDE_STRIP = 0.27f;   // sideways: the selected picture's height, at most, of the screen's

constexpr uint32_t PANEL       = 0xFF000000u;   // behind the panel: black, like the sides of a game
constexpr uint32_t AFTER_SHADE = 0x99000000u;   // a picture after the cursor, darker
constexpr uint32_t KEPT_LINE   = 0xFFE8B840u;   // the kept lane (yellow, as on the desktop)
constexpr uint32_t BUMP_RED    = 0xFF4F5AFFu;

// The pictures' sizes for a strip `width` wide showing `shown` of them
// (one of them the bigger selected one), the selected one at most
// `most_height` tall (then they're smaller and more of them fit).
struct StripSize
{
    float width = 0.0f;       // a picture
    float height = 0.0f;
    float big_width = 0.0f;   // the selected one
    float big_height = 0.0f;
    float step = 0.0f;        // from one picture's middle to the next's
};

static StripSize strip_size(const DvrView* view, float width, int32_t shown, float most_height)
{
    StripSize size;
    float aspect = view->aspect > 0.1f ? view->aspect : 8.0f / 7.0f;
    float count = static_cast<float>(shown);
    size.width = std::floor((width - (count - 1.0f) * THUMB_GAP) / (count - 1.0f + BIG));
    if (size.width * BIG / aspect > most_height)
    {
        size.width = std::floor(most_height * aspect / BIG);
    }
    size.height = std::floor(size.width / aspect);
    size.big_width = std::floor(size.width * BIG);
    size.big_height = std::floor(size.big_width / aspect);
    size.step = size.width + THUMB_GAP;
    return size;
}

// The tallest the selected picture may be: upright anything (the width
// decides); sideways a share of the screen, so the game keeps its room.
static float most_picture_height(bool upright, float screen_height)
{
    return upright ? 10000.0f : std::floor(screen_height * SIDE_STRIP);
}

// With room above the pictures for the selected one's edge (it would be
// cut off by the strip's clip otherwise).
static float strip_height(const StripSize* size)
{
    return BORDER + size->big_height + LABEL + TRACK;
}

static int32_t shown_pictures(bool upright)
{
    return upright ? 5 : 4;
}

static float side_column_x(float width)
{
    return width - MARGIN - SIDE;
}

// Upright: what the panel takes under the game.
static float panel_height(const DvrView* view, float width)
{
    StripSize size = strip_size(view, width - 2.0f * MARGIN, shown_pictures(true), most_picture_height(true, 0.0f));
    float height = GAP + HEADER + GAP + strip_height(&size) + GAP + STEP + GAP + MAIN + GAP + EXIT + MARGIN;
    if (view->lanes)
    {
        height += LANES + GAP;
    }
    return height;
}

void dvr_view_phone_areas(const DvrView* view, float width, float height, float game_aspect, DvrPhoneAreas* areas)
{
    *areas = DvrPhoneAreas{};
    areas->upright = height > width;
    float aspect = game_aspect > 0.1f ? game_aspect : 4.0f / 3.0f;
    if (areas->upright)
    {
        // The game keeps its full width at the top (Andrés: full size); the
        // panel under it.
        float room = height - panel_height(view, width);
        float game = std::floor(width / aspect);
        areas->game_width = width;
        areas->game_height = game < room ? game : room;
        return;
    }
    // Sideways: the game at the left, the strip under it.
    float left = side_column_x(width) - MARGIN;
    StripSize size = strip_size(view, left - 2.0f * MARGIN, shown_pictures(false), most_picture_height(false, height));
    areas->game_width = left;
    areas->game_height = height - MARGIN - strip_height(&size) - GAP;
}

// A text in a chip (the DVR's title): its face, the text centered.
static float draw_chip(const TouchUi* ui, float x, float y, float height, const char* label, uint32_t face, uint32_t color)
{
    float width = touch_ui_text_width(ui, label, TouchText::normal) + 2.0f * CHIP_PAD;
    touch_ui_rect(ui, x, y, width, height, face);
    touch_ui_text(ui, x + CHIP_PAD, touch_ui_centered_text_y(ui, y, height, TouchText::normal), label, color, TouchText::normal);
    return width;
}

// The title chip, the mode and (`readout`) where the cursor is, at the right.
static void draw_header(const TouchUi* ui, const DvrViewText* text, float x, float y, float width, bool readout)
{
    const RetroUiPalette* palette = &ui->palette;
    float chip = draw_chip(ui, x, y, HEADER, text->title, palette->highlight, palette->text_highlight);
    float text_y = touch_ui_centered_text_y(ui, y, HEADER, TouchText::normal);
    touch_ui_text(ui, x + chip + 6.0f, text_y, text->mode, palette->text, TouchText::normal);
    if (readout)
    {
        float readout_width = touch_ui_text_width(ui, text->readout, TouchText::normal);
        touch_ui_text(ui, x + width - readout_width, text_y, text->readout, palette->text_highlight, TouchText::normal);
    }
}

// The lanes' tabs, once a kept lane exists: a tap on the other one switches.
static float draw_lanes(const TouchUi* ui, const DvrView* view, const DvrViewText* text, float x, float y, float width,
                        DvrViewInput* input)
{
    if (!view->lanes)
    {
        return y;
    }
    const char* names[2] = {text->lane_recorded, text->lane_kept};
    int32_t current = view->on_upper ? 1 : 0;
    int32_t tapped = touch_ui_choices(ui, "##dvr_lanes", names, 2, current, x, y, width, LANES);
    if (tapped >= 0 && tapped != current)
    {
        input->action = DvrViewAction::other_lane;
    }
    return y + LANES + GAP;
}

// The pictures along `width`, the selected one bigger in the middle, the
// track under them with the cursor; dragging it moves, a tap on a picture
// goes there.
static void draw_strip(const TouchUi* ui, const DvrView* view, float x, float y, float width, int32_t shown,
                       float most_height, DvrViewInput* input)
{
    const RetroUiPalette* palette = &ui->palette;
    StripSize size = strip_size(view, width, shown, most_height);
    float block = static_cast<float>(ui->scale);
    float middle = std::floor(x + width * 0.5f);
    float bottom = y + BORDER + size.big_height;   // the pictures stand on one line, the selected edge inside
    touch_ui_push_clip(ui, x, y, width, strip_height(&size));
    for (uint32_t i = 0; i < view->count; i++)
    {
        float away = static_cast<float>(static_cast<int32_t>(i) - static_cast<int32_t>(view->selected));
        bool selected = i == view->selected;
        float w = selected ? size.big_width : size.width;
        float h = selected ? size.big_height : size.height;
        // Beside the selected one, each a step further (the bigger one pushes them out).
        float center = middle + away * size.step + (away > 0.0f ? 1.0f : (away < 0.0f ? -1.0f : 0.0f)) *
                                                       (size.big_width - size.width) * 0.5f;
        float left = std::floor(center - w * 0.5f);
        if (left + w < x || left > x + width)
        {
            continue;
        }
        float top = bottom - h;
        uint32_t edge = selected ? 0xFFFFFFFFu : palette->panel_dark;
        if (selected && view->bump_direction != 0)
        {
            edge = BUMP_RED;
        }
        touch_ui_rect(ui, left - BORDER, top - BORDER, w + 2.0f * BORDER, h + 2.0f * BORDER, edge);
        const DvrViewThumb* thumb = &view->thumbs[i];
        if (thumb->texture != 0)
        {
            retro_ui_layer_image(thumb->texture, std::floor(left * block), std::floor(top * block), std::floor(w * block),
                                 std::floor(h * block), thumb->u_max, thumb->v_max, 0xFFFFFFFFu);
        }
        else
        {
            touch_ui_rect(ui, left, top, w, h, palette->panel_face);
        }
        if (thumb->after)
        {
            touch_ui_rect(ui, left, top, w, h, AFTER_SHADE);
        }
        float label_width = touch_ui_text_width(ui, thumb->label, TouchText::normal);
        touch_ui_text(ui, std::floor(center - label_width * 0.5f), bottom + 3.0f, thumb->label,
                      selected ? palette->text_highlight : palette->text_disabled, TouchText::normal);
    }

    // The track: the recorded lane's line, the kept lane's above it from
    // where they part, the cursor on its lane between two pictures.
    float track_y = bottom + LABEL;
    float cursor = static_cast<float>(view->selected) + view->between;
    float lower_end = middle + (view->lower_end - cursor) * size.step;
    touch_ui_rect(ui, x, track_y + 6.0f, (lower_end < x + width ? lower_end : x + width) - x, 2.0f, palette->highlight);
    if (view->lanes)
    {
        float fork = middle + (view->fork_at - cursor) * size.step;
        float upper_end = middle + (view->upper_end - cursor) * size.step;
        fork = fork < x ? x : fork;
        upper_end = upper_end > x + width ? x + width : upper_end;
        if (upper_end > fork)
        {
            touch_ui_rect(ui, fork, track_y + 2.0f, upper_end - fork, 2.0f, KEPT_LINE);
        }
    }
    float cursor_x = middle + view->between * size.step;
    float cursor_top = view->lanes && view->on_upper ? track_y : track_y + 3.0f;
    touch_ui_rect(ui, std::floor(cursor_x) - 1.0f, cursor_top, 2.0f, 7.0f, 0xFFFFFFFFu);
    touch_ui_pop_clip();

    // Fingers: the whole strip is one area (a picture under a finger isn't
    // a button: the finger may drag across them).
    bool held = false;
    bool tapped = touch_ui_area(ui, "##dvr_strip", x, y, width, strip_height(&size), &held, TouchNav::taps_only);
    const ImGuiIO& io = ImGui::GetIO();
    float step_pixels = size.step * block;
    float from = (io.MousePos.x - io.MouseClickedPos[0].x) / step_pixels;
    if (held)
    {
        input->dragging = true;
        input->drag_from = from;
        input->drag_moved = io.MouseDelta.x / step_pixels;
    }
    if (tapped && std::fabs(from) < TAP_SLOP)
    {
        float down = (io.MouseClickedPos[0].x / block - middle) / size.step;
        int32_t index = static_cast<int32_t>(view->selected) + static_cast<int32_t>(std::lround(down));
        if (index >= 0 && index < static_cast<int32_t>(view->count) && index != static_cast<int32_t>(view->selected))
        {
            input->clicked = index;
        }
    }
}

// A row of buttons sharing `width`; a tap sets its action.
static void draw_buttons(const TouchUi* ui, const char* id, const char* const* labels, const DvrViewAction* actions,
                         const bool* lit, int32_t count, float x, float y, float width, float height, DvrViewInput* input)
{
    float gap = TOUCH_UI_GAP;
    float each = std::floor((width - gap * static_cast<float>(count - 1)) / static_cast<float>(count));
    for (int32_t i = 0; i < count; i++)
    {
        char button_id[48];
        std::snprintf(button_id, sizeof(button_id), "%s%d", id, static_cast<int>(i));
        float button_x = x + static_cast<float>(i) * (each + gap);
        float button_width = i == count - 1 ? x + width - button_x : each;
        if (touch_ui_button(ui, button_id, button_x, y, button_width, height, labels[i],
                            lit[i] ? TouchButton::primary : TouchButton::normal, TouchText::normal))
        {
            input->action = actions[i];
        }
    }
}

static void draw_main_buttons(const TouchUi* ui, const DvrViewText* text, float x, float bottom, float width,
                              DvrViewInput* input)
{
    float exit_y = bottom - EXIT;
    float play_y = exit_y - GAP - MAIN;
    if (touch_ui_button(ui, "##dvr_play_here", x, play_y, width, MAIN, text->play_here, TouchButton::primary, TouchText::big))
    {
        input->action = DvrViewAction::play_here;
    }
    if (touch_ui_button(ui, "##dvr_back_to_now", x, exit_y, width, EXIT, text->exit, TouchButton::normal, TouchText::normal))
    {
        input->action = DvrViewAction::back_to_now;
    }
}

void dvr_view_draw_phone(const DvrView* view, RetroUiFont* pixel_font, int32_t scale, const DvrViewText* text,
                         const DvrPhoneAreas* areas, DvrViewInput* input)
{
    *input = DvrViewInput{};
    TouchUi ui;
    ui.font = pixel_font;
    ui.scale = scale;
    ui.palette = *retro_ui_palette();
    float width = touch_ui_width(&ui);
    float height = touch_ui_height(&ui);
    const char* back_arrow = "\xE2\x97\x80";      // ◀
    const char* forward_arrow = "\xE2\x96\xB6";   // ▶

    touch_ui_begin_overlay(&ui, "##dvr_phone");
    if (areas->upright)
    {
        float top = areas->game_height;
        touch_ui_rect(&ui, 0.0f, top, width, height - top, PANEL);
        float x = MARGIN;
        float inner = width - 2.0f * MARGIN;
        float y = top + GAP;
        draw_header(&ui, text, x, y, inner, true);
        y += HEADER + GAP;
        y = draw_lanes(&ui, view, text, x, y, inner, input);
        float most = most_picture_height(true, height);
        draw_strip(&ui, view, x, y, inner, shown_pictures(true), most, input);
        StripSize size = strip_size(view, inner, shown_pictures(true), most);
        y += strip_height(&size) + GAP;
        const char* labels[5] = {text->jump_back, back_arrow, text->watch, forward_arrow, text->jump_forward};
        const DvrViewAction actions[5] = {DvrViewAction::jump_back, DvrViewAction::back, DvrViewAction::watch,
                                          DvrViewAction::forward, DvrViewAction::jump_forward};
        const bool lit[5] = {false, false, view->playing, false, false};
        draw_buttons(&ui, "##dvr_step", labels, actions, lit, 5, x, y, inner, STEP, input);
        draw_main_buttons(&ui, text, x, height - MARGIN, inner, input);
    }
    else
    {
        // The strip under the game, the buttons' column at the right.
        float left = areas->game_width;
        float strip_y = areas->game_height + GAP;
        touch_ui_rect(&ui, 0.0f, areas->game_height, left, height - areas->game_height, PANEL);
        draw_strip(&ui, view, MARGIN, strip_y, left - 2.0f * MARGIN, shown_pictures(false), most_picture_height(false, height),
                   input);
        float column = side_column_x(width);
        touch_ui_rect(&ui, left, 0.0f, width - left, height, PANEL);
        float y = MARGIN;
        draw_header(&ui, text, column, y, SIDE, false);
        y += HEADER + 2.0f;
        touch_ui_text(&ui, column, y, text->readout, ui.palette.text_highlight, TouchText::normal);
        y += touch_ui_text_height(&ui, TouchText::normal) + GAP;
        y = draw_lanes(&ui, view, text, column, y, SIDE, input);
        const char* moves[3] = {back_arrow, text->watch, forward_arrow};
        const DvrViewAction move_actions[3] = {DvrViewAction::back, DvrViewAction::watch, DvrViewAction::forward};
        const bool move_lit[3] = {false, view->playing, false};
        draw_buttons(&ui, "##dvr_move", moves, move_actions, move_lit, 3, column, y, SIDE, STEP, input);
        y += STEP + TOUCH_UI_GAP;
        const char* jumps[2] = {text->jump_back, text->jump_forward};
        const DvrViewAction jump_actions[2] = {DvrViewAction::jump_back, DvrViewAction::jump_forward};
        const bool jump_lit[2] = {false, false};
        draw_buttons(&ui, "##dvr_jump", jumps, jump_actions, jump_lit, 2, column, y, SIDE, STEP, input);
        draw_main_buttons(&ui, text, column, height - MARGIN, SIDE, input);
    }
    touch_ui_end_screen();
}
