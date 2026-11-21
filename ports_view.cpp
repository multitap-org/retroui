#include "ports_view.h"

#include <cmath>
#include <cstdio>

#include "imgui.h"
#include "pixel_font_imgui.h"
#include "port_indicators.h"
#include "retro_ui.h"

// ---------------------------------------------------------------------------
// Layout, in retro pixels
// ---------------------------------------------------------------------------

// The tabs.
constexpr float TAB_WIDTH    = 96.0f;   // every tab, whatever it shows
constexpr float TAB_OVERLAP  = 24.0f;   // more than 4 tabs: each covers at least this much of the one before
constexpr float TABS_LEFT    = 4.0f;
constexpr float CARD_PADDING = 5.0f;
constexpr float LINE         = 7.0f;    // a line of the pixel font (glyphs 1..5 of it)
constexpr float HEADER       = 9.0f;    // the first line: LEDs, P1, ping
constexpr float CHIP_LIP     = 2.0f;    // a chip's bottom rows, darker (piccoletto's buttons)
constexpr float CHIP_PADDING = 4.0f;
constexpr float YOURS_BAR    = 2.0f;    // marks the viewer's own chips
constexpr float GAP          = 3.0f;
constexpr float PEEK         = CARD_PADDING + HEADER;   // risen this far, only the first line shows
constexpr float STATUS_GAP   = 6.0f;    // between the tabs and what follows (piccolo's status line)

// The board.
constexpr float BOARD_MARGIN = 8.0f;
constexpr float BOARD_HEADER = 15.0f;
constexpr float BOARD_HINT   = 11.0f;
constexpr float BOARD_GAP    = 4.0f;
constexpr float COLUMN_MIN   = 90.0f;   // narrower than this, the columns scroll instead (4 fit a phone)
constexpr float DONE_WIDTH   = 40.0f;
constexpr float ARROW_WIDTH  = 12.0f;
constexpr float BOARD_SHEET     = 0.4f;   // the share of the screen the board takes from the bottom...
constexpr float BOARD_SHEET_MAX = 0.7f;   // ...more only if a column needs it, up to this

// Over the game: cards the game shows through (piccoletto's). The frontend
// dims the game behind the board itself (piccolo's BACKDROP), not this.
constexpr uint32_t CARD_ALPHA = 0xB0000000u;
constexpr uint32_t LIT_ALPHA  = 0xD8000000u;   // the tab of a port that changed
constexpr uint32_t SHEET_ALPHA = 0x80000000u;  // the board's sheet, behind its cards

// Timing.
constexpr uint64_t POP_MS     = 3000;   // the tabs stay up this long after a change
constexpr uint64_t STAGGER_MS = 35;     // each tab starts moving this much after the one before
constexpr float    EASE_RATE  = 14.0f;  // per second: how fast a tab closes in on where it's going

// ---------------------------------------------------------------------------
// The model
// ---------------------------------------------------------------------------

void ports_view_reset(PortsView* view)
{
    view->port_count = 0;
    view->popped_ms = 0;
    view->has_pick = false;
    view->scroll = -1.0f;
    for (uint32_t p = 0; p < PORTS_VIEW_MAX_PORTS; p++)
    {
        view->ports[p].chips.clear();
        view->changed[p] = false;
        view->height[p] = 0.0f;
        view->target[p] = 0.0f;
    }
    view->out.chips.clear();
}

void ports_view_pop(PortsView* view, const bool* changed, uint64_t now_ms)
{
    if (view->popped_ms == 0)
    {
        for (uint32_t p = 0; p < PORTS_VIEW_MAX_PORTS; p++)
        {
            view->changed[p] = false;
        }
    }
    uint32_t count = view->port_count < PORTS_VIEW_MAX_PORTS ? view->port_count : PORTS_VIEW_MAX_PORTS;
    for (uint32_t p = 0; p < count; p++)
    {
        view->changed[p] = view->changed[p] || changed[p];
    }
    view->popped_ms = now_ms == 0 ? 1 : now_ms;   // (0 means "not up")
}

bool ports_view_tabs_visible(const PortsView* view)
{
    for (uint32_t p = 0; p < view->port_count && p < PORTS_VIEW_MAX_PORTS; p++)
    {
        if (view->height[p] > 0.0f || view->target[p] > 0.0f)
        {
            return true;
        }
    }
    return false;
}

static bool has_live(const PortsViewColumn* column)
{
    for (size_t i = 0; i < column->chips.size(); i++)
    {
        if (!column->chips[i].gone)
        {
            return true;
        }
    }
    return false;
}

static void drop_gone(PortsViewColumn* column)
{
    for (size_t i = column->chips.size(); i > 0; i--)
    {
        if (column->chips[i - 1].gone)
        {
            column->chips.erase(column->chips.begin() + static_cast<std::ptrdiff_t>(i - 1));
        }
    }
}

// The board's columns in order: OUT (when shown), then the ports.
static uint32_t board_columns(const PortsView* view)
{
    return view->port_count + (view->show_out ? 1u : 0u);
}

static const PortsViewColumn* board_column(const PortsView* view, uint32_t index, int32_t* port)
{
    if (view->show_out)
    {
        *port = static_cast<int32_t>(index) - 1;
        return index == 0 ? &view->out : &view->ports[index - 1];
    }
    *port = static_cast<int32_t>(index);
    return &view->ports[index];
}

bool ports_view_find(const PortsView* view, uint32_t key, int32_t* column)
{
    for (uint32_t c = 0; c < board_columns(view); c++)
    {
        int32_t port = 0;
        const PortsViewColumn* list = board_column(view, c, &port);
        for (size_t i = 0; i < list->chips.size(); i++)
        {
            if (list->chips[i].key == key && !list->chips[i].gone)
            {
                *column = port;
                return true;
            }
        }
    }
    return false;
}

void ports_view_pick_next(PortsView* view, int32_t direction)
{
    std::vector<uint32_t> order;   // (a few entries, only on a press)
    for (uint32_t c = 0; c < board_columns(view); c++)
    {
        int32_t port = 0;
        const PortsViewColumn* list = board_column(view, c, &port);
        for (size_t i = 0; i < list->chips.size(); i++)
        {
            if (!list->chips[i].gone)
            {
                order.push_back(list->chips[i].key);
            }
        }
    }
    // DONE is a stop too, before the first chip (UP from the first one, DOWN
    // from the last one, reach it).
    if (view->done_focus || order.empty())
    {
        view->done_focus = order.empty();
        if (!order.empty())
        {
            view->picked = direction > 0 ? order.front() : order.back();
            view->has_pick = true;
        }
        return;
    }
    int32_t at = -1;
    for (size_t i = 0; i < order.size(); i++)
    {
        at = view->has_pick && order[i] == view->picked ? static_cast<int32_t>(i) : at;
    }
    int32_t size = static_cast<int32_t>(order.size());
    int32_t next = at < 0 ? 0 : at + direction;
    if (next < 0 || next >= size)
    {
        view->done_focus = true;
        return;
    }
    view->picked = order[static_cast<size_t>(next)];
    view->has_pick = true;
}

// ---------------------------------------------------------------------------
// Drawing pieces (window pixels; `block` = one retro pixel)
// ---------------------------------------------------------------------------

// One LED per port, the port's own lit when someone is on it. Returns the width.
static float draw_leds(float x, float y, float block, uint32_t count, uint32_t port, bool lit)
{
    int32_t lit_index = lit ? static_cast<int32_t>(port) : -1;
    return port_indicators_draw_leds(x, y + block, block, count, lit_index, retro_ui_palette()->panel_dark);
}

// Three rising bars, bottoms level with the letters', ending at `right`.
static void draw_ping_bars(float right, float y, float block, bool known, uint32_t ping_ms)
{
    port_indicators_draw_ping(right - port_indicators_ping_width(block), y + 6.0f * block, block, known, ping_ms,
                              retro_ui_palette()->panel_dark);
}

// The bars and "12 MS" ending at `right`: the first linked chip's.
static void draw_ping(const PortsViewColumn* column, const PortsViewText* text, float right, float y, float block)
{
    for (size_t i = 0; i < column->chips.size(); i++)
    {
        const PortsViewChip* chip = &column->chips[i];
        if (!chip->has_ping || chip->gone)
        {
            continue;
        }
        float x = right;
        if (chip->ping_known)
        {
            char number[24];
            std::snprintf(number, sizeof(number), text->ping, static_cast<unsigned>(chip->ping_ms));
            x -= retro_ui_layer_text_width(number);
            retro_ui_layer_text(x, y, number, ping_color(chip->ping_ms));
            x -= 3.0f * block;
        }
        draw_ping_bars(x, y, block, chip->ping_known, chip->ping_ms);
        return;
    }
}

// A chip as a flat button with a lip (the picked one in the highlight).
static void draw_chip(const PortsView* view, const PortsViewChip* chip, const PortsViewText* text, float x, float y,
                      float width, float block, bool picked)
{
    const RetroUiPalette* palette = retro_ui_palette();
    uint32_t face = chip->gone ? palette->field_face : (picked ? palette->highlight : palette->button_face);
    uint32_t color = chip->gone ? palette->text_disabled : (picked ? palette->text_highlight : palette->button_text);
    float height = view->chip_height * block;
    float lip = CHIP_LIP * block;
    retro_ui_layer_rect(x, y, width, height - lip, face);
    retro_ui_layer_rect(x, y + height - lip, width, lip, retro_ui_color_darker(face));
    if (chip->yours && !chip->gone)
    {
        retro_ui_layer_rect(x, y, YOURS_BAR * block, height, palette->text_highlight);
    }
    float text_y = y + std::floor((view->chip_height - CHIP_LIP - LINE) * 0.5f) * block;
    float right = x + width - CHIP_PADDING * block;
    if (chip->gone)
    {
        right -= retro_ui_layer_text_width(text->off);
        retro_ui_layer_text(right, text_y, text->off, palette->text_disabled);
        right -= 3.0f * block;
    }
    else if (chip->has_ping)
    {
        draw_ping_bars(right, text_y, block, chip->ping_known, chip->ping_ms);
        right -= port_indicators_ping_width(block) + 3.0f * block;
    }
    float left = x + CHIP_PADDING * block;
    retro_ui_layer_clip(left, y, right - left, height);
    retro_ui_layer_text(left, text_y, chip->label.c_str(), color);
    retro_ui_layer_unclip();
}

// How tall a column's card is: padding, its first line, its chips (or
// NOBODY), padding.
static float card_height(const PortsView* view, const PortsViewColumn* column)
{
    size_t rows = column->chips.size();
    float content = rows == 0 ? LINE : static_cast<float>(rows) * (view->chip_height + GAP) - GAP;
    return CARD_PADDING + HEADER + GAP + content + CARD_PADDING;
}

// ---------------------------------------------------------------------------
// Tabs
// ---------------------------------------------------------------------------

// From one tab to the next: a whole tab; more than 4 overlap; and they
// overlap more when they still don't fit the screen (a phone held upright).
static float tab_step(const PortsView* view)
{
    uint32_t count = view->port_count;
    float step = count > 4 ? TAB_WIDTH - TAB_OVERLAP : TAB_WIDTH;
    if (count > 1 && view->screen_width > 0.0f)
    {
        float fits = (view->screen_width - 2.0f * TABS_LEFT - TAB_WIDTH) / static_cast<float>(count - 1);
        step = fits < step ? std::floor(fits) : step;
    }
    return step < 1.0f ? 1.0f : step;
}

float ports_view_tabs_width(const PortsView* view)
{
    if (view->port_count == 0)
    {
        return 0.0f;
    }
    return TABS_LEFT + static_cast<float>(view->port_count - 1) * tab_step(view) + TAB_WIDTH + STATUS_GAP;
}

static void draw_tab(const PortsView* view, uint32_t port, const PortsViewText* text, float block, float screen_height)
{
    const RetroUiPalette* palette = retro_ui_palette();
    const PortsViewColumn* column = &view->ports[port];
    bool lit = view->changed[port] && view->popped_ms != 0;
    float x = (TABS_LEFT + static_cast<float>(port) * tab_step(view)) * block;
    float y = screen_height - view->height[port] * block;
    float width = TAB_WIDTH * block;
    // The whole card; what hasn't risen yet is below the window's edge.
    retro_ui_layer_rect(x, y, width, card_height(view, column) * block,
                        retro_ui_color_set_alpha(palette->panel_face, lit ? LIT_ALPHA : CARD_ALPHA));

    // First line: the LEDs, P1, a link's ping.
    float line_x = x + CARD_PADDING * block;
    float line_y = y + CARD_PADDING * block;
    float leds = draw_leds(line_x, line_y, block, view->port_count, port, has_live(column));
    char name[16];
    std::snprintf(name, sizeof(name), text->tab, static_cast<unsigned>(port + 1));
    retro_ui_layer_text(line_x + leds + 4.0f * block, line_y, name, lit ? palette->text_highlight : palette->text_disabled);
    draw_ping(column, text, x + width - CARD_PADDING * block, line_y, block);

    // Who is on it.
    float chip_y = line_y + (HEADER + GAP) * block;
    float chip_width = width - 2.0f * CARD_PADDING * block;
    if (column->chips.empty())
    {
        retro_ui_layer_text(line_x, chip_y, text->nobody, palette->text_disabled);
    }
    for (size_t i = 0; i < column->chips.size(); i++)
    {
        draw_chip(view, &column->chips[i], text, line_x, chip_y, chip_width, block, false);
        chip_y += (view->chip_height + GAP) * block;
    }
}

void ports_view_draw_tabs(PortsView* view, ImFont* pixel_font, int32_t scale, const PortsViewText* text, bool peek,
                          bool hidden, bool front, uint64_t now_ms)
{
    uint32_t count = view->port_count < PORTS_VIEW_MAX_PORTS ? view->port_count : PORTS_VIEW_MAX_PORTS;
    float dt = view->last_draw_ms == 0 || now_ms < view->last_draw_ms
                   ? 0.0f
                   : static_cast<float>(now_ms - view->last_draw_ms) / 1000.0f;
    dt = dt > 0.1f ? 0.1f : dt;   // (a stall doesn't jump them)
    view->last_draw_ms = now_ms;
    view->screen_width = ImGui::GetIO().DisplaySize.x / static_cast<float>(scale > 0 ? scale : 1);
    if (view->popped_ms != 0 && now_ms - view->popped_ms >= POP_MS)
    {
        view->popped_ms = 0;   // going down; the greyed chips go once they're down
    }
    // Where each tab goes: hidden, its first line, or whole.
    bool any_up = false;
    for (uint32_t p = 0; p < count; p++)
    {
        float target = 0.0f;
        if (!hidden && view->popped_ms != 0)
        {
            target = view->changed[p] ? card_height(view, &view->ports[p]) : PEEK;
        }
        else if (!hidden && peek)
        {
            target = PEEK;
        }
        if (target != view->target[p])
        {
            view->target[p] = target;
            view->target_ms[p] = now_ms + static_cast<uint64_t>(p) * STAGGER_MS;
        }
        if (now_ms >= view->target_ms[p])
        {
            float distance = view->target[p] - view->height[p];
            view->height[p] += distance * (1.0f - std::exp(-EASE_RATE * dt));
            if (std::fabs(view->target[p] - view->height[p]) < 0.25f)
            {
                view->height[p] = view->target[p];
            }
        }
        any_up = any_up || view->height[p] > 0.0f;
    }
    // Down to their first line (or gone): the greyed chips go now.
    bool settled = view->popped_ms == 0;
    for (uint32_t p = 0; p < count && settled; p++)
    {
        settled = view->height[p] <= PEEK;
    }
    if (settled)
    {
        for (uint32_t p = 0; p < PORTS_VIEW_MAX_PORTS; p++)
        {
            drop_gone(&view->ports[p]);
        }
    }
    if (!any_up)
    {
        return;
    }
    float block = static_cast<float>(scale);
    float screen_height = ImGui::GetIO().DisplaySize.y;
    retro_ui_begin_layer("##ports_tabs", pixel_font, scale, front);
    // The ones that changed last, so they're on top where tabs overlap.
    for (uint32_t pass = 0; pass < 2; pass++)
    {
        for (uint32_t p = 0; p < count; p++)
        {
            bool lit = view->changed[p] && view->popped_ms != 0;
            if ((pass == 1) == lit && view->height[p] > 0.0f)
            {
                draw_tab(view, p, text, block, screen_height);
            }
        }
    }
    retro_ui_end_layer();
}

// ---------------------------------------------------------------------------
// Board
// ---------------------------------------------------------------------------

static bool clicked_in(float x, float y, float width, float height)
{
    const ImGuiIO& io = ImGui::GetIO();
    return io.MouseClicked[0] && io.MousePos.x >= x && io.MousePos.x < x + width && io.MousePos.y >= y &&
           io.MousePos.y < y + height;
}

void ports_view_open_board(PortsView* view)
{
    view->board_rise = 0.0f;
    view->board_last_ms = 0;
    view->scroll = -1.0f;
    view->done_focus = false;
}

void ports_view_draw_board(PortsView* view, ImFont* pixel_font, int32_t scale, const PortsViewText* text, uint64_t now_ms,
                           PortsViewBoardInput* input)
{
    *input = PortsViewBoardInput{};
    if (view->port_count == 0)
    {
        return;
    }
    const RetroUiPalette* palette = retro_ui_palette();
    const ImGuiIO& io = ImGui::GetIO();
    float block = static_cast<float>(scale);
    float screen_width = io.DisplaySize.x;
    float screen_height = io.DisplaySize.y;

    // A sheet from the bottom (where the tabs are): 40% of the screen, taller
    // (up to 70%) only if the tallest column needs it; it slides up.
    float tallest = 0.0f;
    for (uint32_t c = 0; c < board_columns(view); c++)
    {
        int32_t port = 0;
        float height = card_height(view, board_column(view, c, &port));
        tallest = height > tallest ? height : tallest;
    }
    float needed = (2.0f * BOARD_MARGIN + BOARD_HEADER + BOARD_HINT + 2.0f * BOARD_GAP + tallest) * block;
    float sheet_height = needed < BOARD_SHEET * screen_height ? BOARD_SHEET * screen_height : needed;
    sheet_height = sheet_height > BOARD_SHEET_MAX * screen_height ? BOARD_SHEET_MAX * screen_height : sheet_height;
    float dt = view->board_last_ms == 0 || now_ms < view->board_last_ms
                   ? 0.0f
                   : static_cast<float>(now_ms - view->board_last_ms) / 1000.0f;
    view->board_last_ms = now_ms;
    dt = dt > 0.1f ? 0.1f : dt;
    view->board_rise += (1.0f - view->board_rise) * (1.0f - std::exp(-EASE_RATE * dt));
    view->board_rise = view->board_rise > 0.995f ? 1.0f : view->board_rise;
    float sheet_top = std::floor(screen_height - sheet_height * view->board_rise);
    retro_ui_begin_layer("##ports_board", pixel_font, scale, true);
    retro_ui_layer_rect(0.0f, sheet_top, screen_width, sheet_height, retro_ui_color_set_alpha(palette->panel_dark, SHEET_ALPHA));
    retro_ui_layer_rect(0.0f, sheet_top, screen_width, block, palette->panel_light);   // its top edge

    // The title bar with DONE, and the hint under it (piccoletto's).
    float margin = BOARD_MARGIN * block;
    float gap = BOARD_GAP * block;
    float inner_width = screen_width - 2.0f * margin;
    float header_y = sheet_top + margin;
    float header_height = BOARD_HEADER * block;
    float done_width = DONE_WIDTH * block;
    float text_offset = std::floor((BOARD_HEADER - LINE) * 0.5f) * block;
    retro_ui_layer_rect(margin, header_y, inner_width - done_width - gap, header_height, retro_ui_color_set_alpha(palette->panel_dark, CARD_ALPHA));
    retro_ui_layer_text(margin + 6.0f * block, header_y + text_offset, text->title, palette->text_highlight);
    float done_x = margin + inner_width - done_width;
    retro_ui_layer_rect(done_x, header_y, done_width, header_height - CHIP_LIP * block, palette->highlight);
    retro_ui_layer_rect(done_x, header_y + header_height - CHIP_LIP * block, done_width, CHIP_LIP * block, retro_ui_color_darker(palette->highlight));
    if (view->done_focus)
    {
        // Reached with UP / DOWN: an outline in the text color (A takes it).
        uint32_t edge = palette->text_highlight;
        retro_ui_layer_rect(done_x - block, header_y - block, done_width + 2.0f * block, block, edge);
        retro_ui_layer_rect(done_x - block, header_y + header_height, done_width + 2.0f * block, block, edge);
        retro_ui_layer_rect(done_x - block, header_y, block, header_height, edge);
        retro_ui_layer_rect(done_x + done_width, header_y, block, header_height, edge);
    }
    retro_ui_layer_text(done_x + std::floor((done_width - retro_ui_layer_text_width(text->done)) * 0.5f),
                        header_y + text_offset - block, text->done, palette->text_highlight);
    input->done = clicked_in(done_x, header_y, done_width, header_height);
    float hint_y = header_y + header_height + gap;
    float hint_height = BOARD_HINT * block;
    retro_ui_layer_rect(margin, hint_y, inner_width, hint_height, retro_ui_color_set_alpha(palette->panel_dark, CARD_ALPHA));
    retro_ui_layer_clip(margin, hint_y, inner_width, hint_height);
    retro_ui_layer_text(margin + 6.0f * block, hint_y + std::floor((BOARD_HINT - LINE) * 0.5f) * block, text->help,
                        palette->text_disabled);
    retro_ui_layer_unclip();

    // The columns side by side; scrolled when they don't fit.
    float content_y = hint_y + hint_height + gap;
    float content_height = sheet_top + sheet_height - content_y - margin;   // (down to the sheet's bottom)
    content_height = content_height < 0.0f ? 0.0f : content_height;
    uint32_t columns = board_columns(view);
    float column_width = (inner_width - static_cast<float>(columns - 1) * gap) / static_cast<float>(columns);
    float minimum = COLUMN_MIN * block;
    column_width = column_width < minimum ? minimum : std::floor(column_width);
    float total = static_cast<float>(columns) * column_width + static_cast<float>(columns - 1) * gap;
    float max_scroll = total > inner_width ? total - inner_width : 0.0f;
    int32_t picked_port = 0;
    bool has_pick = view->has_pick && ports_view_find(view, view->picked, &picked_port);
    if (has_pick)
    {
        // The picked chip's column in the middle (as far as it goes).
        uint32_t picked_column = static_cast<uint32_t>(picked_port + (view->show_out ? 1 : 0));
        float center = static_cast<float>(picked_column) * (column_width + gap) + column_width * 0.5f;
        view->scroll_target = center - inner_width * 0.5f;
    }
    view->scroll_target = view->scroll_target < 0.0f ? 0.0f : (view->scroll_target > max_scroll ? max_scroll : view->scroll_target);
    if (view->scroll < 0.0f)
    {
        view->scroll = view->scroll_target;
    }
    view->scroll += (view->scroll_target - view->scroll) * 0.25f;
    if (std::fabs(view->scroll_target - view->scroll) < 0.5f)
    {
        view->scroll = view->scroll_target;
    }
    // Cards as tall as the tallest (measured above), so they line up.
    float card = tallest * block;
    card = card > content_height ? content_height : card;

    retro_ui_layer_clip(margin, content_y, inner_width, content_height);
    for (uint32_t c = 0; c < columns; c++)
    {
        int32_t port = 0;
        const PortsViewColumn* column = board_column(view, c, &port);
        float x = margin + static_cast<float>(c) * (column_width + gap) - std::floor(view->scroll);
        if (x + column_width < margin || x > margin + inner_width)
        {
            continue;
        }
        retro_ui_layer_rect(x, content_y, column_width, card, retro_ui_color_set_alpha(palette->panel_face, CARD_ALPHA));
        float line_x = x + CARD_PADDING * block;
        float line_y = content_y + CARD_PADDING * block;
        if (port < 0)
        {
            retro_ui_layer_text(line_x, line_y, text->out, palette->text_disabled);
        }
        else
        {
            char title[32];
            float leds = draw_leds(line_x, line_y, block, view->port_count, static_cast<uint32_t>(port), has_live(column));
            std::snprintf(title, sizeof(title), text->port, static_cast<unsigned>(port + 1));
            retro_ui_layer_text(line_x + leds + 4.0f * block, line_y, title, palette->text_disabled);
            draw_ping(column, text, x + column_width - CARD_PADDING * block, line_y, block);
        }
        float chip_y = line_y + (HEADER + GAP) * block;
        float chip_width = column_width - 2.0f * CARD_PADDING * block;
        bool chip_clicked = false;
        if (column->chips.empty())
        {
            retro_ui_layer_text(line_x, chip_y, text->nobody, palette->text_disabled);
        }
        for (size_t i = 0; i < column->chips.size(); i++)
        {
            const PortsViewChip* chip = &column->chips[i];
            draw_chip(view, chip, text, line_x, chip_y, chip_width, block, has_pick && !view->done_focus && chip->key == view->picked);
            if (!chip->gone && clicked_in(line_x, chip_y, chip_width, view->chip_height * block))
            {
                // A click picks it; another click on it puts it down.
                bool same = has_pick && chip->key == view->picked;
                view->has_pick = !same;
                view->done_focus = false;
                view->picked = chip->key;
                chip_clicked = true;
            }
            chip_y += (view->chip_height + GAP) * block;
        }
        if (!chip_clicked && has_pick && port != picked_port && clicked_in(x, content_y, column_width, content_height))
        {
            input->move = true;
            input->key = view->picked;
            input->port = port;
        }
    }
    retro_ui_layer_unclip();

    // Arrows where there are more columns (the pixel font's triangles).
    float arrow_width = ARROW_WIDTH * block;
    float arrow_y = content_y + std::floor((card - LINE * block) * 0.5f);
    if (view->scroll > 0.5f)
    {
        retro_ui_layer_rect(margin, content_y, arrow_width, card, retro_ui_color_set_alpha(palette->panel_dark, LIT_ALPHA));
        retro_ui_layer_text(margin + 3.0f * block, arrow_y, PIXEL_FONT_UTF8_ARROW_LEFT, palette->text_highlight);
    }
    if (view->scroll < max_scroll - 0.5f)
    {
        float arrow_x = margin + inner_width - arrow_width;
        retro_ui_layer_rect(arrow_x, content_y, arrow_width, card, retro_ui_color_set_alpha(palette->panel_dark, LIT_ALPHA));
        retro_ui_layer_text(arrow_x + 3.0f * block, arrow_y, PIXEL_FONT_UTF8_ARROW_RIGHT, palette->text_highlight);
    }
    retro_ui_end_layer();
}
