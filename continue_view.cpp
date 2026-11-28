#include "continue_view.h"

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "imgui.h"
#include "imgui_internal.h"   // BringWindowToDisplayBack

#include "pixel_font_imgui.h"
#include "retro_ui.h"
#include "touch_ui.h"

// The panel, in retro pixels (Andrés, 2026-11-28: "fill the screen", mockup
// continue-recents c736fe8). Only margins, gaps and text lines are fixed;
// the picture, the tiles and the rows take what the room leaves.
constexpr float PANEL_SHARE      = 0.9f;    // of the room under the menu bar, both ways
constexpr float PANEL_MAX_ASPECT = 1.85f;   // side by side, never wider than this times its height
constexpr float PANEL_PADDING    = 8.0f;
constexpr float PANEL_GAP        = 10.0f;   // between the picture and the words / the list
constexpr float SECTION_GAP      = 8.0f;    // between the card and the shelf
constexpr float LINE_GAP         = 3.0f;    // between lines of text
constexpr float TEXT_LINE        = 8.0f;    // a line of the pixel font (PIXEL_FONT_IMGUI_UNIT at 1x)
constexpr float NAME_LINE        = 16.0f;   // the game's name, at double size
constexpr float NAME_LINES_SIDE  = 2.0f;    // side by side the name wraps to two lines; under the picture, one
constexpr float BUTTON_GAP       = 4.0f;    // above the buttons, and between two
constexpr float BUTTON_HEIGHT    = 20.0f;
constexpr float TILE_GAP         = 8.0f;    // between two tiles of the shelf
constexpr float WORDS_MIN_WIDTH  = 170.0f;  // side by side: the words' column at least
constexpr float LIST_MIN_WIDTH   = 200.0f;  // the list's column: at least...
constexpr float LIST_MAX_WIDTH   = 360.0f;  // ...and at most
constexpr float ROW_GAP          = 5.0f;    // between two rows of the list
constexpr float ROW_PADDING      = 4.0f;
constexpr float ROW_PICTURE_SHARE = 0.3f;   // a row's picture: at most this share of the list's width
// The shelf's header: its words, the line under it, the space under that.
constexpr float SHELF_HEADER = TEXT_LINE + LINE_GAP + 1.0f + 4.0f;
// Under a tile: its name, then "core · when".
constexpr float TILE_CAPTION = 3.0f + TEXT_LINE + 2.0f + TEXT_LINE;
// The words under the picture when they don't sit beside it (stacked, list):
// the name, "core · when", room for a reason of two lines, the buttons, the
// hint. Always reserved, so the picture doesn't change size with the words.
constexpr float COLUMN_WORDS = 6.0f + NAME_LINE + LINE_GAP + TEXT_LINE + LINE_GAP + 2.0f * TEXT_LINE + LINE_GAP +
                               BUTTON_GAP + BUTTON_HEIGHT + LINE_GAP + TEXT_LINE;
// The buttons and the hint at the bottom of the card's words.
constexpr float BUTTONS_AND_HINT = BUTTON_HEIGHT + LINE_GAP + TEXT_LINE;

// The phone's card, in retro pixels of the touch UI.
constexpr float CARD_PADDING        = 6.0f;
constexpr float CARD_GAP            = 3.0f;
constexpr float CARD_BUTTON         = 24.0f;   // 48 dp
// The sideways card: the band over the picture's bottom.
constexpr float COVER_PADDING       = 5.0f;
constexpr float COVER_GAP           = 2.0f;
constexpr float COVER_SIDE_MIN      = 190.0f;  // a band this wide has its buttons at its right
constexpr float COVER_BUTTON_WIDTH  = 76.0f;
constexpr uint32_t COVER_BAND       = 0xDC101828u;   // the palette's dark, mostly opaque

static double card_aspect(const ContinueViewCard* card)
{
    if (card->aspect_width > 0 && card->aspect_height > 0)
    {
        return static_cast<double>(card->aspect_width) / static_cast<double>(card->aspect_height);
    }
    return 4.0 / 3.0;
}

void continue_view_picture_size(uint32_t aspect_width, uint32_t aspect_height, float max_width, float max_height,
                                float* width, float* height)
{
    double aspect = 4.0 / 3.0;
    if (aspect_width > 0 && aspect_height > 0)
    {
        aspect = static_cast<double>(aspect_width) / static_cast<double>(aspect_height);
    }
    // As tall as it may be; narrower when that's too wide.
    double fit_height = static_cast<double>(max_height);
    double fit_width = std::floor(fit_height * aspect + 0.5);
    if (fit_width > static_cast<double>(max_width))
    {
        fit_width = static_cast<double>(max_width);
        fit_height = std::floor(fit_width / aspect + 0.5);
    }
    *width = static_cast<float>(fit_width > 1.0 ? fit_width : 1.0);
    *height = static_cast<float>(fit_height > 1.0 ? fit_height : 1.0);
}

ContinueViewLayout continue_view_panel_layout(float screen_width, float screen_height, int32_t scale)
{
    if (screen_height > screen_width)
    {
        return ContinueViewLayout::stacked_shelf;
    }
    float block = static_cast<float>(scale);
    if (scale <= 1 && screen_height < CONTINUE_VIEW_LIST_MAX_HEIGHT &&
        screen_width >= CONTINUE_VIEW_LIST_MIN_WIDTH * block)
    {
        return ContinueViewLayout::side_list;
    }
    return ContinueViewLayout::side_shelf;
}

// The biggest 4:3 box in `max_width` x `max_height` (whole retro pixels).
static void box_fit(float max_width, float max_height, float* width, float* height)
{
    float fit_height = std::floor(max_width * 3.0f / 4.0f);
    fit_height = fit_height < max_height ? fit_height : std::floor(max_height);
    fit_height = fit_height > 1.0f ? fit_height : 1.0f;
    *height = fit_height;
    *width = std::floor(fit_height * 4.0f / 3.0f);
}

static float at_least(float value, float least)
{
    return value > least ? value : least;
}

void continue_view_panel_geometry(int32_t scale, bool shelf, float top, float screen_width, float screen_height,
                                  ContinueViewPanelGeometry* geometry)
{
    ContinueViewPanelGeometry g;
    int32_t whole_scale = scale > 0 ? scale : 1;
    float block = static_cast<float>(whole_scale);
    g.block = block;
    g.layout = continue_view_panel_layout(screen_width, screen_height, whole_scale);
    // All in retro pixels from here; window pixels at the end.
    float room_width = std::floor(screen_width / block);
    float room_height = std::floor((screen_height - top) / block);
    float width = std::floor(room_width * PANEL_SHARE);
    float height = std::floor(room_height * PANEL_SHARE);
    float picture_width = 1.0f;
    float picture_height = 1.0f;
    float card_height = 0.0f;
    float words_width = 0.0f;
    float list_width = 0.0f;
    float row_height = 0.0f;
    float tile_width = 0.0f;
    float tile_height = 0.0f;
    float shelf_height = 0.0f;   // the gap above it included
    if (g.layout == ContinueViewLayout::side_shelf)
    {
        width = width < std::floor(height * PANEL_MAX_ASPECT) ? width : std::floor(height * PANEL_MAX_ASPECT);
        float inner_width = at_least(width - 2.0f * PANEL_PADDING, 1.0f);
        float inner_height = at_least(height - 2.0f * PANEL_PADDING, 1.0f);
        card_height = inner_height;
        if (shelf)
        {
            // Four tiles across, 4:3; never more than half of what the card
            // and the tiles share (a wide, short window).
            tile_width = at_least(std::floor((inner_width - 3.0f * TILE_GAP) / 4.0f), 4.0f);
            tile_height = std::floor(tile_width * 3.0f / 4.0f);
            float most = std::floor((inner_height - SECTION_GAP - SHELF_HEADER - TILE_CAPTION) / 2.0f);
            most = at_least(most, 3.0f);
            if (tile_height > most)
            {
                tile_height = most;
                tile_width = std::floor(tile_height * 4.0f / 3.0f);
            }
            shelf_height = SECTION_GAP + SHELF_HEADER + tile_height + TILE_CAPTION;
            card_height = at_least(inner_height - shelf_height, 1.0f);
        }
        box_fit(at_least(inner_width - PANEL_GAP - WORDS_MIN_WIDTH, 1.0f), card_height, &picture_width,
                &picture_height);
        card_height = picture_height;
        words_width = at_least(inner_width - picture_width - PANEL_GAP, 1.0f);
        height = 2.0f * PANEL_PADDING + card_height + shelf_height;
    }
    else if (g.layout == ContinueViewLayout::side_list)
    {
        float inner_width = at_least(width - 2.0f * PANEL_PADDING, 1.0f);
        float inner_height = at_least(height - 2.0f * PANEL_PADDING, 1.0f);
        box_fit(at_least(inner_width - PANEL_GAP - LIST_MIN_WIDTH, 1.0f), at_least(inner_height - COLUMN_WORDS, 1.0f),
                &picture_width, &picture_height);
        card_height = picture_height + COLUMN_WORDS;
        words_width = picture_width;
        list_width = at_least(inner_width - picture_width - PANEL_GAP, 1.0f);
        list_width = list_width < LIST_MAX_WIDTH ? list_width : LIST_MAX_WIDTH;
        // Four rows fill the column's height; each one's picture as tall as
        // the row lets it, no wider than a share of the list.
        row_height = at_least(std::floor((card_height - SHELF_HEADER - 3.0f * ROW_GAP) / 4.0f), TEXT_LINE * 2.0f);
        if (shelf)
        {
            tile_height = at_least(row_height - 2.0f * ROW_PADDING, 1.0f);
            float most = std::floor(list_width * ROW_PICTURE_SHARE * 3.0f / 4.0f);
            tile_height = tile_height < most ? tile_height : at_least(most, 1.0f);
            tile_width = std::floor(tile_height * 4.0f / 3.0f);
        }
        width = 2.0f * PANEL_PADDING + picture_width + PANEL_GAP + list_width;
        height = 2.0f * PANEL_PADDING + card_height;
    }
    else
    {
        float inner_width = at_least(width - 2.0f * PANEL_PADDING, 1.0f);
        float room = height - 2.0f * PANEL_PADDING - COLUMN_WORDS;
        if (shelf)
        {
            tile_width = at_least(std::floor((inner_width - 3.0f * TILE_GAP) / 4.0f), 4.0f);
            tile_height = std::floor(tile_width * 3.0f / 4.0f);
            shelf_height = SECTION_GAP + SHELF_HEADER + tile_height + TILE_CAPTION;
            room -= shelf_height;
        }
        // The picture across the inside, as tall as 4:3 makes it or as the
        // room lets it (never under a quarter of its width).
        picture_width = inner_width;
        picture_height = std::floor(inner_width * 3.0f / 4.0f);
        float least = std::floor(inner_width / 4.0f);
        room = at_least(room, least);
        picture_height = at_least(picture_height < room ? picture_height : std::floor(room), 1.0f);
        card_height = picture_height + COLUMN_WORDS;
        words_width = picture_width;
        height = 2.0f * PANEL_PADDING + card_height + shelf_height;
    }
    g.width = width * block;
    g.height = height * block;
    g.picture_width = picture_width * block;
    g.picture_height = picture_height * block;
    g.card_height = card_height * block;
    g.words_width = words_width * block;
    g.list_width = list_width * block;
    g.row_height = row_height * block;
    g.tile_width = tile_width * block;
    g.tile_height = tile_height * block;
    float area_height = screen_height - top;
    g.x = std::floor((screen_width - g.width) * 0.5f);
    g.y = std::floor(top + (area_height - g.height) * 0.5f);
    g.y = g.y > top ? g.y : top;
    *geometry = g;
}

void continue_view_panel_picture_size(const ContinueViewCard* card, const ContinueViewPanelGeometry* geometry,
                                      float* width, float* height)
{
    continue_view_picture_size(card->aspect_width, card->aspect_height, geometry->picture_width,
                               geometry->picture_height, width, height);
}

// ---------------------------------------------------------------------------
// The desktop panel

// One wrapped text's height, in window pixels.
static float wrapped_height(float font_size, float width, const char* text)
{
    if (text[0] == '\0')
    {
        return 0.0f;
    }
    ImVec2 size = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, width, text);
    return std::ceil(size.y);
}

static float draw_wrapped(ImDrawList* draw_list, float font_size, float x, float y, float width, const char* text,
                          uint32_t argb)
{
    draw_list->AddText(ImGui::GetFont(), font_size, ImVec2(x, y), retro_ui_color_imgui(argb), text, nullptr, width);
    return wrapped_height(font_size, width, text);
}

// `text` on one line, cut with "..." where it doesn't fit `width` (a game's
// name is long, a tile is narrow). Returns the line's height.
constexpr size_t CLIP_TEXT_MAX = 400;
static float draw_clipped(ImDrawList* draw_list, float font_size, float x, float y, float width, const char* text,
                          uint32_t argb)
{
    char line[CLIP_TEXT_MAX] = {};
    char shown[CLIP_TEXT_MAX + 8] = {};   // the cut text and "..."
    std::snprintf(line, sizeof(line), "%s", text);
    std::snprintf(shown, sizeof(shown), "%s", line);
    ImFont* font = ImGui::GetFont();
    if (font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, line).x > width)
    {
        size_t length = std::strlen(line);
        while (length > 0)
        {
            length--;
            while (length > 0 && (static_cast<uint8_t>(line[length]) & 0xC0u) == 0x80u)
            {
                length--;   // back to the start of a UTF-8 character
            }
            line[length] = '\0';
            std::snprintf(shown, sizeof(shown), "%.*s...", static_cast<int>(length), line);
            if (font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, shown).x <= width)
            {
                break;
            }
        }
    }
    draw_list->AddText(font, font_size, ImVec2(x, y), retro_ui_color_imgui(argb), shown);
    return std::ceil(font_size);
}

// A button of the panel; the highlighted one in the palette's highlight
// (the mockup's "on" button) while the panel is active.
static bool panel_button(const char* id, const char* label, float x, float y, float width, float height,
                         bool highlighted)
{
    RetroUiPalette lit = *retro_ui_palette();
    if (highlighted)
    {
        lit.button_face = lit.highlight;
        lit.button_bottom = lit.highlight;
        lit.button_text = lit.text_highlight;
    }
    retro_ui_push_palette(&lit);
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    ImGui::PushID(id);
    bool pressed = retro_ui_button_sized(label, width, height);
    ImGui::PopID();
    retro_ui_pop_palette();
    return pressed;
}

// The "core · when" line of the card when its words are under the picture.
static void core_when_line(char* out, size_t out_size, const ContinueViewCard* card)
{
    if (card->core[0] != '\0' && card->when[0] != '\0')
    {
        std::snprintf(out, out_size, "%s · %s", card->core, card->when);
    }
    else
    {
        std::snprintf(out, out_size, "%s%s", card->core, card->when);
    }
}

// A rectangle of one color, `thickness` wide, around a box.
static void draw_frame(ImDrawList* draw_list, float x, float y, float width, float height, float thickness,
                       uint32_t argb)
{
    uint32_t color = retro_ui_color_imgui(argb);
    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + thickness), color);
    draw_list->AddRectFilled(ImVec2(x, y + height - thickness), ImVec2(x + width, y + height), color);
    draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + thickness, y + height), color);
    draw_list->AddRectFilled(ImVec2(x + width - thickness, y), ImVec2(x + width, y + height), color);
}

// The picture of a card or a tile in its box: black, the picture at its
// aspect centered in it (bars where it's narrower).
static void draw_picture(ImDrawList* draw_list, const ImGuiPlatformIO* platform_io, float box_x, float box_y,
                         float box_width, float box_height, float picture_width, float picture_height,
                         uint64_t texture, float u_max, float v_max)
{
    draw_list->AddRectFilled(ImVec2(box_x, box_y), ImVec2(box_x + box_width, box_y + box_height),
                             retro_ui_color_imgui(0xFF000000u));
    if (texture == 0)
    {
        return;
    }
    float x = std::floor(box_x + (box_width - picture_width) * 0.5f);
    float y = std::floor(box_y + (box_height - picture_height) * 0.5f);
    draw_list->AddCallback(platform_io->DrawCallback_SetSamplerLinear, nullptr);
    draw_list->AddImage(static_cast<ImTextureID>(texture), ImVec2(x, y), ImVec2(x + picture_width, y + picture_height),
                        ImVec2(0.0f, 0.0f), ImVec2(u_max, v_max));
    draw_list->AddCallback(platform_io->DrawCallback_SetSamplerNearest, nullptr);
}

// A click area over a rectangle (the window's own widgets draw nothing).
static bool click_area(const char* id, float x, float y, float width, float height, bool* hovered)
{
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    bool pressed = ImGui::InvisibleButton(id, ImVec2(width, height));
    *hovered = ImGui::IsItemHovered();
    return pressed;
}

// "◀ 1/3 ▶" with its right edge at `right_edge`: the arrows dim at the ends,
// and click.
static void draw_pager(ImDrawList* draw_list, const ContinueViewShelf* shelf, float font, float block,
                       float right_edge, float y, ContinueViewAction* action)
{
    if (shelf->page_count <= 1)
    {
        return;
    }
    const RetroUiPalette* palette = retro_ui_palette();
    float text_height = std::ceil(font);
    const char* left = "\xE2\x97\x80";    // U+25C0
    const char* right = "\xE2\x96\xB6";   // U+25B6
    char count[32] = {};
    std::snprintf(count, sizeof(count), "%u/%u", shelf->page + 1, shelf->page_count);
    ImFont* imfont = ImGui::GetFont();
    float arrow_width = std::ceil(imfont->CalcTextSizeA(font, FLT_MAX, 0.0f, left).x);
    float count_width = std::ceil(imfont->CalcTextSizeA(font, FLT_MAX, 0.0f, count).x);
    float margin = 3.0f * block;
    float right_x = right_edge - arrow_width;
    float count_x = right_x - margin - count_width;
    float left_x = count_x - margin - arrow_width;
    bool can_left = shelf->page > 0;
    bool can_right = shelf->page + 1 < shelf->page_count;
    draw_list->AddText(imfont, font, ImVec2(left_x, y),
                       retro_ui_color_imgui(can_left ? palette->text : palette->text_disabled), left);
    draw_list->AddText(imfont, font, ImVec2(count_x, y), retro_ui_color_imgui(palette->text_disabled), count);
    draw_list->AddText(imfont, font, ImVec2(right_x, y),
                       retro_ui_color_imgui(can_right ? palette->text : palette->text_disabled), right);
    bool hovered = false;
    if (click_area("page_left", left_x - margin, y, arrow_width + 2.0f * margin, text_height, &hovered) && can_left)
    {
        action->page_delta = -1;
    }
    if (click_area("page_right", right_x - margin, y, arrow_width + 2.0f * margin, text_height, &hovered) &&
        can_right)
    {
        action->page_delta = 1;
    }
}

// A tile's picture in its box (a shelf tile or a list row): black, the
// picture at its aspect; without one, the core's name dim in the middle.
static void draw_tile_picture(ImDrawList* draw_list, const ImGuiPlatformIO* platform_io, const ContinueViewTile* tile,
                              float font, float block, float x, float y, float width, float height)
{
    float picture_width = 0.0f;
    float picture_height = 0.0f;
    continue_view_picture_size(tile->aspect_width, tile->aspect_height, width, height, &picture_width,
                               &picture_height);
    draw_picture(draw_list, platform_io, x, y, width, height, picture_width, picture_height, tile->texture,
                 tile->u_max, tile->v_max);
    if (tile->texture != 0)
    {
        return;
    }
    float text_height = std::ceil(font);
    float room = width - 2.0f * block;
    ImVec2 size = ImGui::GetFont()->CalcTextSizeA(font, FLT_MAX, 0.0f, tile->core);
    float text_width = std::ceil(size.x) < room ? std::ceil(size.x) : room;
    draw_list->PushClipRect(ImVec2(x, y), ImVec2(x + width, y + height), true);
    draw_clipped(draw_list, font, std::floor(x + (width - text_width) * 0.5f),
                 std::floor(y + (height - text_height) * 0.5f), room, tile->core,
                 retro_ui_palette()->text_disabled);
    draw_list->PopClipRect();
}

// A header ("RECENT" with the page indicator at its right) and the thin
// line under it, `width` wide at (x, y). Returns where what follows starts.
static float draw_header(ImDrawList* draw_list, const ContinueViewShelf* shelf, float font, float block, float x,
                         float y, float width, ContinueViewAction* action)
{
    const RetroUiPalette* palette = retro_ui_palette();
    draw_list->AddText(ImGui::GetFont(), font, ImVec2(x, y), retro_ui_color_imgui(palette->section), shelf->header);
    draw_pager(draw_list, shelf, font, block, x + width, y, action);
    float line_y = y + std::ceil(font) + LINE_GAP * block;
    draw_list->AddRectFilled(ImVec2(x, line_y), ImVec2(x + width, line_y + block),
                             retro_ui_color_imgui(palette->button_face));
    return y + SHELF_HEADER * block;
}

// The shelf under the card: the header, then the tiles (picture, name,
// "core · when"), `width` wide at (x, y).
static void draw_shelf(ImDrawList* draw_list, const ImGuiPlatformIO* platform_io, const ContinueViewCard* card,
                       const ContinueViewPanelGeometry* g, float x, float y, float width, ContinueViewAction* action)
{
    const RetroUiPalette* palette = retro_ui_palette();
    const ContinueViewShelf* shelf = &card->shelf;
    float block = g->block;
    float font = ImGui::GetFontSize();
    float text_height = std::ceil(font);
    float tile_y = draw_header(draw_list, shelf, font, block, x, y, width, action);
    float tile_width = g->tile_width;
    float tile_height = g->tile_height;
    for (uint32_t i = 0; i < shelf->tile_count; i++)
    {
        const ContinueViewTile* tile = &shelf->tiles[i];
        float tile_x = x + static_cast<float>(i) * (tile_width + TILE_GAP * block);
        draw_tile_picture(draw_list, platform_io, tile, font, block, tile_x, tile_y, tile_width, tile_height);
        char id[16] = {};
        std::snprintf(id, sizeof(id), "tile%u", i);
        bool hovered = false;
        if (click_area(id, tile_x, tile_y, tile_width, tile_height + TILE_CAPTION * block, &hovered) && card->active)
        {
            action->tile_chosen = static_cast<int32_t>(i);
        }
        bool focused = card->active && card->focus_tile == static_cast<int32_t>(i);
        bool lit = card->active && hovered;
        if (focused)
        {
            draw_frame(draw_list, tile_x, tile_y, tile_width, tile_height, block, 0xFFFFFFFFu);
        }
        else if (lit)
        {
            draw_frame(draw_list, tile_x, tile_y, tile_width, tile_height, block, palette->highlight);
        }
        float caption_y = tile_y + tile_height + 3.0f * block;
        draw_clipped(draw_list, font, tile_x, caption_y, tile_width, tile->game,
                     focused || lit ? palette->text_highlight : palette->text);
        draw_clipped(draw_list, font, tile_x, caption_y + text_height + 2.0f * block, tile_width, tile->line,
                     palette->text_disabled);
    }
}

// The list of the short window, at the card's right (a thin line on its
// left): the header, then one row per game (its small picture, the name,
// "core · when" dim under it), four at most, filling the card's height.
static void draw_list_column(ImDrawList* draw_list, const ImGuiPlatformIO* platform_io, const ContinueViewCard* card,
                             const ContinueViewPanelGeometry* g, float x, float y, ContinueViewAction* action)
{
    const RetroUiPalette* palette = retro_ui_palette();
    const ContinueViewShelf* shelf = &card->shelf;
    float block = g->block;
    float font = ImGui::GetFontSize();
    float text_height = std::ceil(font);
    float line_gap = LINE_GAP * block;
    float width = g->list_width;
    float separator_x = std::floor(x - PANEL_GAP * block * 0.5f);
    draw_list->AddRectFilled(ImVec2(separator_x, y), ImVec2(separator_x + block, y + g->card_height),
                             retro_ui_color_imgui(palette->button_face));
    float row_y = draw_header(draw_list, shelf, font, block, x, y, width, action);
    float row_pad = ROW_PADDING * block;
    for (uint32_t i = 0; i < shelf->tile_count; i++)
    {
        const ContinueViewTile* tile = &shelf->tiles[i];
        char id[16] = {};
        std::snprintf(id, sizeof(id), "row%u", i);
        bool hovered = false;
        if (click_area(id, x, row_y, width, g->row_height, &hovered) && card->active)
        {
            action->tile_chosen = static_cast<int32_t>(i);
        }
        bool focused = card->active && card->focus_tile == static_cast<int32_t>(i);
        bool lit = card->active && hovered;
        draw_list->AddRectFilled(ImVec2(x, row_y), ImVec2(x + width, row_y + g->row_height),
                                 retro_ui_color_imgui(palette->panel_face));
        if (focused)
        {
            draw_frame(draw_list, x, row_y, width, g->row_height, block, 0xFFFFFFFFu);
        }
        else if (lit)
        {
            draw_frame(draw_list, x, row_y, width, g->row_height, block, palette->highlight);
        }
        float picture_x = x + row_pad;
        float picture_y = std::floor(row_y + (g->row_height - g->tile_height) * 0.5f);
        draw_tile_picture(draw_list, platform_io, tile, font, block, picture_x, picture_y, g->tile_width,
                          g->tile_height);
        float text_x = picture_x + g->tile_width + 2.0f * row_pad;
        float text_width = x + width - row_pad - text_x;
        float words_height = 2.0f * text_height + line_gap;
        float text_y = std::floor(row_y + (g->row_height - words_height) * 0.5f);
        draw_clipped(draw_list, font, text_x, text_y, text_width, tile->game,
                     focused || lit ? palette->text_highlight : palette->text);
        draw_clipped(draw_list, font, text_x, text_y + text_height + line_gap, text_width, tile->line,
                     palette->text_disabled);
        row_y += g->row_height + ROW_GAP * block;
    }
}

// `text` wrapped in `width`, no more than `lines` lines of `line_height`
// (the rest cut off). Returns the height drawn.
static float draw_wrapped_lines(ImDrawList* draw_list, float font_size, float x, float y, float width,
                                float line_height, float lines, const char* text, uint32_t argb)
{
    float most = line_height * lines;
    draw_list->PushClipRect(ImVec2(x, y), ImVec2(x + width, y + most), true);
    float drawn = draw_wrapped(draw_list, font_size, x, y, width, text, argb);
    draw_list->PopClipRect();
    return drawn < most ? drawn : most;
}

ContinueViewAction continue_view_draw_panel(ImFont* pixel_font, int32_t scale, const ContinueViewCard* card,
                                            float top, float screen_width, float screen_height)
{
    ContinueViewAction action;
    ContinueViewPanelGeometry g;
    continue_view_panel_geometry(scale, card->shelf.tile_count > 0, top, screen_width, screen_height, &g);
    float block = g.block;
    retro_ui_push_theme(pixel_font, scale);
    const RetroUiPalette* palette = retro_ui_palette();

    float pad = PANEL_PADDING * block;
    float line_gap = LINE_GAP * block;
    float font = ImGui::GetFontSize();
    float name_font = PIXEL_FONT_IMGUI_UNIT * 2.0f * block;   // double size: still whole pixels
    float text_height = std::ceil(font);

    ImGui::SetNextWindowPos(ImVec2(g.x, g.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(g.width, g.height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, block);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, retro_ui_color_vec4(palette->field_face));
    ImGui::PushStyleColor(ImGuiCol_Border, retro_ui_color_vec4(palette->button_face));
    // Part of the empty screen: never focused, never in front, no title.
    // (No keyboard navigation of ImGui's either: the keys are main.cpp's.)
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse;
    if (!card->active)
    {
        flags |= ImGuiWindowFlags_NoInputs;
    }
    ImGui::Begin("##continue_panel", nullptr, flags);
    // Behind every window, every frame (as retroui's background layers);
    // whatever is drawn back after it this frame goes behind it.
    ImGui::BringWindowToDisplayBack(ImGui::GetCurrentWindow());
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);   // the pixel font stays sharp

    // The picture in its box at the card's top left.
    float card_x = g.x + pad;
    float card_y = g.y + pad;
    float picture_width = 0.0f;
    float picture_height = 0.0f;
    continue_view_panel_picture_size(card, &g, &picture_width, &picture_height);
    draw_picture(draw_list, &platform_io, card_x, card_y, g.picture_width, g.picture_height, picture_width,
                 picture_height, card->texture, card->u_max, card->v_max);

    // The words: beside the picture (side by side) or under it.
    bool beside = g.layout == ContinueViewLayout::side_shelf;
    float words_x = beside ? card_x + g.picture_width + PANEL_GAP * block : card_x;
    float words_y = beside ? card_y : card_y + g.picture_height + 6.0f * block;
    float column = g.words_width;
    float buttons_y = card_y + g.card_height - BUTTONS_AND_HINT * block;
    if (beside)
    {
        words_y += draw_wrapped_lines(draw_list, name_font, words_x, words_y, column, NAME_LINE * block,
                                      NAME_LINES_SIDE, card->game, palette->text_highlight) + line_gap;
        words_y += draw_clipped(draw_list, font, words_x, words_y, column, card->core, palette->text_disabled) +
                   line_gap;
        words_y += draw_clipped(draw_list, font, words_x, words_y, column, card->when, palette->text_disabled);
        if (card->reason[0] != '\0')
        {
            // As many lines as fit above the buttons.
            words_y += line_gap;
            float room = buttons_y - BUTTON_GAP * block - words_y;
            float lines = std::floor(room / text_height);
            if (lines >= 1.0f)
            {
                draw_wrapped_lines(draw_list, font, words_x, words_y, column, text_height, lines, card->reason,
                                   palette->text);
            }
        }
    }
    else
    {
        words_y += draw_clipped(draw_list, name_font, words_x, words_y, column, card->game, palette->text_highlight) +
                   line_gap;
        char line[CLIP_TEXT_MAX] = {};
        core_when_line(line, sizeof(line), card);
        words_y += draw_clipped(draw_list, font, words_x, words_y, column, line, palette->text_disabled);
        if (card->reason[0] != '\0')
        {
            words_y += line_gap;
            draw_wrapped_lines(draw_list, font, words_x, words_y, column, text_height, 2.0f, card->reason,
                               palette->text);
        }
    }

    // The buttons at the bottom of the words, the hint under them. Two: the
    // recommended one twice as wide (the mockup's 2:1).
    float button_height = BUTTON_HEIGHT * block;
    bool on_buttons = card->active && card->focus_tile < 0;
    if (card->second_label[0] == '\0')
    {
        if (panel_button("first", card->first_label, words_x, buttons_y, column, button_height, on_buttons))
        {
            action.pressed = card->first;
        }
    }
    else
    {
        float first_width = std::floor((column - BUTTON_GAP * block) * 2.0f / 3.0f);
        float second_width = column - BUTTON_GAP * block - first_width;
        if (panel_button("first", card->first_label, words_x, buttons_y, first_width, button_height,
                         on_buttons && !card->focus_second))
        {
            action.pressed = card->first;
        }
        if (panel_button("second", card->second_label, words_x + column - second_width, buttons_y, second_width,
                         button_height, on_buttons && card->focus_second))
        {
            action.pressed = card->second;
        }
    }
    if (card->hint[0] != '\0')
    {
        draw_clipped(draw_list, font, words_x, buttons_y + button_height + line_gap, column, card->hint,
                     palette->text_disabled);
    }

    if (card->shelf.tile_count > 0 && g.layout == ContinueViewLayout::side_list)
    {
        draw_list_column(draw_list, &platform_io, card, &g, card_x + g.picture_width + PANEL_GAP * block, card_y,
                         &action);
    }
    else if (card->shelf.tile_count > 0)
    {
        draw_shelf(draw_list, &platform_io, card, &g, card_x, card_y + g.card_height + SECTION_GAP * block,
                   g.width - 2.0f * pad, &action);
    }
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    retro_ui_pop_theme();
    if (!card->active)
    {
        return ContinueViewAction{};
    }
    return action;
}

// ---------------------------------------------------------------------------
// The phone's card

void continue_view_card_picture_size(const ContinueViewCard* card, float width, float* picture_width,
                                     float* picture_height)
{
    // The card's whole width, any size (sharp bilinear takes the fraction),
    // but never taller than the card is wide (a DS's two screens): narrower
    // then, centered on black. Shorter again when a height is imposed.
    double aspect = card_aspect(card);
    double fit_width = static_cast<double>(width);
    double fit_height = fit_width / aspect;
    double max_height = static_cast<double>(width);
    if (card->picture_max_height > 0.0f && static_cast<double>(card->picture_max_height) < max_height)
    {
        max_height = static_cast<double>(card->picture_max_height);
    }
    if (fit_height > max_height)
    {
        fit_height = max_height;
        fit_width = fit_height * aspect;
    }
    *picture_width = static_cast<float>(fit_width > 1.0 ? fit_width : 1.0);
    *picture_height = static_cast<float>(fit_height > 1.0 ? fit_height : 1.0);
}

float continue_view_card_height(const TouchUi* ui, const ContinueViewCard* card, float width)
{
    float inner = width - 2.0f * CARD_PADDING;
    float picture_width = 0.0f;
    float picture_height = 0.0f;
    continue_view_card_picture_size(card, width, &picture_width, &picture_height);
    float line = touch_ui_text_height(ui, TouchText::normal);
    float content = picture_height + CARD_PADDING + touch_ui_text_wrapped_height(ui, inner, card->game, TouchText::big) +
                    CARD_GAP + line + CARD_GAP + line;
    if (card->reason[0] != '\0')
    {
        content += CARD_GAP + touch_ui_text_wrapped_height(ui, inner, card->reason, TouchText::normal);
    }
    content += CARD_GAP + CARD_BUTTON;
    return content + CARD_PADDING;   // the picture at the top edge, no padding above
}

ContinueViewButton continue_view_draw_card(const TouchUi* ui, const ContinueViewCard* card, float x, float y,
                                           float width, float* height)
{
    ContinueViewButton pressed = ContinueViewButton::none;
    const RetroUiPalette* palette = &ui->palette;
    float inner = width - 2.0f * CARD_PADDING;
    float picture_width = 0.0f;
    float picture_height = 0.0f;
    continue_view_card_picture_size(card, width, &picture_width, &picture_height);
    float line = touch_ui_text_height(ui, TouchText::normal);
    float card_height = continue_view_card_height(ui, card, width);
    *height = card_height;

    touch_ui_rect(ui, x, y, width, card_height, palette->field_face);
    float picture_x = x + (width - picture_width) * 0.5f;
    float top = y;
    touch_ui_rect(ui, x, top, width, picture_height, 0xFF000000u);   // bars beside a narrower picture
    if (card->texture != 0)
    {
        touch_ui_image_smooth(ui, card->texture, picture_x, top, picture_width, picture_height, card->u_max,
                              card->v_max);
    }
    top += picture_height + CARD_PADDING;
    float text_x = x + CARD_PADDING;
    top += touch_ui_text_wrapped(ui, text_x, top, inner, card->game, palette->text_highlight, TouchText::big) +
           CARD_GAP;
    touch_ui_text(ui, text_x, top, card->core, palette->text_disabled, TouchText::normal);
    top += line;
    top += CARD_GAP;
    touch_ui_text(ui, text_x, top, card->when, palette->text_disabled, TouchText::normal);
    top += line;
    if (card->reason[0] != '\0')
    {
        top += CARD_GAP;
        top += touch_ui_text_wrapped(ui, text_x, top, inner, card->reason, palette->text, TouchText::normal);
    }
    top += CARD_GAP;
    if (card->second_label[0] == '\0')
    {
        if (touch_ui_button(ui, "##continue_first", text_x, top, inner, CARD_BUTTON, card->first_label,
                            TouchButton::primary, TouchText::normal))
        {
            pressed = card->first;
        }
    }
    else
    {
        float half = std::floor((inner - CARD_GAP) * 0.5f);
        if (touch_ui_button(ui, "##continue_first", text_x, top, half, CARD_BUTTON, card->first_label,
                            TouchButton::primary, TouchText::normal))
        {
            pressed = card->first;
        }
        if (touch_ui_button(ui, "##continue_second", text_x + inner - half, top, half, CARD_BUTTON,
                            card->second_label, TouchButton::normal, TouchText::normal))
        {
            pressed = card->second;
        }
    }
    return pressed;
}

// ---------------------------------------------------------------------------
// The phone on its side: the picture is the card, the words on a band

void continue_view_cover_picture_size(const ContinueViewCard* card, float max_width, float max_height,
                                      float* picture_width, float* picture_height)
{
    double aspect = card_aspect(card);
    double fit_height = static_cast<double>(max_height);
    double fit_width = fit_height * aspect;
    if (fit_width > static_cast<double>(max_width))
    {
        fit_width = static_cast<double>(max_width);
        fit_height = fit_width / aspect;
    }
    *picture_width = static_cast<float>(fit_width > 1.0 ? fit_width : 1.0);
    *picture_height = static_cast<float>(fit_height > 1.0 ? fit_height : 1.0);
}

// The band's words, top down, `width` wide: the name (two lines at most),
// the core, when, the line (the band stays short);
// their height. Nothing drawn when `draw` is false.
static float cover_words(const TouchUi* ui, const ContinueViewCard* card, float x, float y, float width, bool draw)
{
    const RetroUiPalette* palette = &ui->palette;
    float line = touch_ui_text_height(ui, TouchText::normal);
    float name = touch_ui_text_wrapped_height(ui, width, card->game, TouchText::normal);
    float name_max = 2.0f * line;
    name = name < name_max ? name : name_max;
    if (draw)
    {
        touch_ui_push_clip(ui, x, y, width, name);
        touch_ui_text_wrapped(ui, x, y, width, card->game, palette->text_highlight, TouchText::normal);
        touch_ui_pop_clip();
    }
    float top = y + name;
    const char* lines[3] = {card->core, card->when, card->reason};
    uint32_t colors[3] = {palette->text_disabled, palette->text_disabled, palette->text};
    for (int32_t i = 0; i < 3; i++)
    {
        if (lines[i][0] == '\0')
        {
            continue;
        }
        top += COVER_GAP;
        if (draw)
        {
            top += touch_ui_text_wrapped(ui, x, top, width, lines[i], colors[i], TouchText::normal);
        }
        else
        {
            top += touch_ui_text_wrapped_height(ui, width, lines[i], TouchText::normal);
        }
    }
    return top - y;
}

ContinueViewButton continue_view_draw_cover(const TouchUi* ui, const ContinueViewCard* card, float x, float y,
                                            float max_width, float max_height, float* width)
{
    ContinueViewButton pressed = ContinueViewButton::none;
    float picture_width = 0.0f;
    float picture_height = 0.0f;
    continue_view_cover_picture_size(card, max_width, max_height, &picture_width, &picture_height);
    *width = picture_width;
    touch_ui_rect(ui, x, y, picture_width, picture_height, 0xFF000000u);
    if (card->texture != 0)
    {
        touch_ui_image_smooth(ui, card->texture, x, y, picture_width, picture_height, card->u_max, card->v_max);
    }

    // Wide enough: the buttons at the band's right, the words at its left.
    // Narrow (a DS): the words, then the buttons across.
    bool two = card->second_label[0] != '\0';
    float inner = picture_width - 2.0f * COVER_PADDING;
    bool side = inner >= COVER_SIDE_MIN;
    float words_width = side ? inner - COVER_BUTTON_WIDTH - COVER_PADDING : inner;
    float words = cover_words(ui, card, 0.0f, 0.0f, words_width, false);
    float buttons = side && two ? 2.0f * CARD_BUTTON + COVER_GAP : CARD_BUTTON;
    float band = side ? (words > buttons ? words : buttons) : words + COVER_PADDING + CARD_BUTTON;
    band += 2.0f * COVER_PADDING;
    band = band < picture_height ? band : picture_height;
    float band_y = y + picture_height - band;
    touch_ui_rect(ui, x, band_y, picture_width, band, COVER_BAND);

    float text_x = x + COVER_PADDING;
    cover_words(ui, card, text_x, band_y + COVER_PADDING, words_width, true);
    float button_x = side ? x + picture_width - COVER_PADDING - COVER_BUTTON_WIDTH : text_x;
    float button_width = side ? COVER_BUTTON_WIDTH : inner;
    float button_y = y + picture_height - COVER_PADDING - CARD_BUTTON;
    float first_x = button_x;
    float first_y = button_y;
    float first_width = button_width;
    float second_x = button_x;
    float second_y = button_y;
    float second_width = button_width;
    if (two && side)
    {
        first_y = button_y - CARD_BUTTON - COVER_GAP;   // START above RESUME
    }
    else if (two)
    {
        first_width = std::floor((inner - COVER_GAP) * 0.5f);
        second_width = first_width;
        second_x = text_x + inner - second_width;
    }
    if (touch_ui_button(ui, "##continue_first", first_x, first_y, first_width, CARD_BUTTON, card->first_label,
                        TouchButton::primary, TouchText::normal))
    {
        pressed = card->first;
    }
    if (two && touch_ui_button(ui, "##continue_second", second_x, second_y, second_width, CARD_BUTTON,
                               card->second_label, TouchButton::normal, TouchText::normal))
    {
        pressed = card->second;
    }
    return pressed;
}
