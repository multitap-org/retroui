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

// The panel, in retro pixels.
constexpr float PANEL_PADDING = 6.0f;
constexpr float PANEL_GAP     = 7.0f;   // between the picture and the text
constexpr float LINE_GAP      = 3.0f;   // between lines of text
constexpr float BUTTON_GAP    = 3.0f;   // above the buttons, and between two
constexpr float BUTTON_HEIGHT = 17.0f;  // a big button (the mockup's)

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

void continue_view_panel_picture_size(const ContinueViewCard* card, int32_t scale, bool stacked, float* width,
                                      float* height)
{
    float block = static_cast<float>(scale);
    float room_width = CONTINUE_VIEW_PICTURE_WIDTH;
    float room_height = CONTINUE_VIEW_PICTURE_HEIGHT;
    if (stacked)
    {
        room_width = CONTINUE_VIEW_PANEL_WIDTH - 2.0f * PANEL_PADDING;
        room_height = CONTINUE_VIEW_STACKED_PICTURE_HEIGHT;
    }
    continue_view_picture_size(card->aspect_width, card->aspect_height, room_width * block, room_height * block, width,
                               height);
}

void continue_view_tile_picture_size(uint32_t aspect_width, uint32_t aspect_height, int32_t scale, float* width,
                                     float* height)
{
    float block = static_cast<float>(scale);
    continue_view_picture_size(aspect_width, aspect_height, CONTINUE_VIEW_TILE_WIDTH * block,
                               CONTINUE_VIEW_TILE_HEIGHT * block, width, height);
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

// Where everything goes, in window pixels, measured before drawing (the
// panel is centered, so its height must be known first).
struct PanelMeasure
{
    bool  stacked = false;
    bool  hint = true;           // dropped first when the panel doesn't fit
    float picture_width = 0.0f;
    float picture_height = 0.0f;
    float column = 0.0f;         // the width the words and buttons have
    float words_height = 0.0f;   // the words above the buttons (side by side: in the column)
    float bottom_height = 0.0f;  // the buttons, and the hint under them
    float inner_height = 0.0f;   // the card's contents
    float shelf_height = 0.0f;   // 0 without a shelf
    float width = 0.0f;
    float height = 0.0f;
};

// The "core · when" line of the stacked card.
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

// `picture_room` the tallest picture the stacked card may have (retro pixels).
static void panel_measure(PanelMeasure* m, const ContinueViewCard* card, int32_t scale, bool stacked, bool list,
                          bool hint, float picture_room)
{
    float block = static_cast<float>(scale);
    float pad = PANEL_PADDING * block;
    float line_gap = LINE_GAP * block;
    float font = ImGui::GetFontSize();
    float big = PIXEL_FONT_IMGUI_UNIT * (block + 1.0f);   // one whole step up: still sharp
    float button_height = BUTTON_HEIGHT * block;
    *m = PanelMeasure{};
    m->stacked = stacked;
    m->hint = hint && card->hint[0] != '\0';
    // The list of the short window is a column at the card's right.
    m->width = std::floor((CONTINUE_VIEW_PANEL_WIDTH + (list ? CONTINUE_VIEW_LIST_WIDTH : 0.0f)) * block);
    float inner_width = std::floor(CONTINUE_VIEW_PANEL_WIDTH * block) - 2.0f * pad;
    if (stacked)
    {
        m->column = inner_width;
        continue_view_picture_size(card->aspect_width, card->aspect_height, inner_width, picture_room * block,
                                   &m->picture_width, &m->picture_height);
        m->words_height = wrapped_height(big, m->column, card->game) + line_gap + std::ceil(font);
        if (card->reason[0] != '\0')
        {
            m->words_height += line_gap + wrapped_height(font, m->column, card->reason);
        }
    }
    else
    {
        m->column = CONTINUE_VIEW_INFO_WIDTH * block;
        continue_view_panel_picture_size(card, scale, false, &m->picture_width, &m->picture_height);
        m->words_height = wrapped_height(big, m->column, card->game) + line_gap +
                          wrapped_height(font, m->column, card->core) + line_gap +
                          wrapped_height(font, m->column, card->when);
        if (card->reason[0] != '\0')
        {
            m->words_height += line_gap + wrapped_height(font, m->column, card->reason);
        }
    }
    m->bottom_height = BUTTON_GAP * block + button_height;
    if (m->hint)
    {
        m->bottom_height += line_gap + wrapped_height(font, m->column, card->hint);
    }
    if (stacked)
    {
        m->inner_height = m->picture_height + PANEL_GAP * block + m->words_height + m->bottom_height;
    }
    else
    {
        m->inner_height = m->words_height + m->bottom_height;
        m->inner_height = m->inner_height > m->picture_height ? m->inner_height : m->picture_height;
    }
    if (card->shelf.tile_count > 0 && !list)
    {
        // The header, its line, the tiles (box, name, "core · when").
        m->shelf_height = std::ceil(font) + line_gap + block + 4.0f * block +
                          CONTINUE_VIEW_TILE_HEIGHT * block + 3.0f * block + std::ceil(font) + 2.0f * block +
                          std::ceil(font) + pad;
    }
    m->height = std::floor(pad + m->inner_height + pad + m->shelf_height);
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

// The shelf: header with the page indicator, a thin line, the tiles.
static void draw_shelf(ImDrawList* draw_list, const ImGuiPlatformIO* platform_io, const ContinueViewCard* card,
                       float block, float x, float y, float width, ContinueViewAction* action)
{
    const RetroUiPalette* palette = retro_ui_palette();
    const ContinueViewShelf* shelf = &card->shelf;
    float pad = PANEL_PADDING * block;
    float line_gap = LINE_GAP * block;
    float font = ImGui::GetFontSize();
    float inner_x = x + pad;
    float inner_width = width - 2.0f * pad;
    float text_height = std::ceil(font);

    draw_list->AddText(ImGui::GetFont(), font, ImVec2(inner_x, y), retro_ui_color_imgui(palette->section),
                       shelf->header);
    draw_pager(draw_list, shelf, font, block, inner_x + inner_width, y, action);
    float line_y = y + text_height + line_gap;
    draw_list->AddRectFilled(ImVec2(inner_x, line_y), ImVec2(inner_x + inner_width, line_y + block),
                             retro_ui_color_imgui(palette->button_face));

    float tile_width = CONTINUE_VIEW_TILE_WIDTH * block;
    float tile_height = CONTINUE_VIEW_TILE_HEIGHT * block;
    float tile_gap = 3.0f * block;
    float tile_y = line_y + block + 4.0f * block;
    for (uint32_t i = 0; i < shelf->tile_count; i++)
    {
        const ContinueViewTile* tile = &shelf->tiles[i];
        float tile_x = inner_x + static_cast<float>(i) * (tile_width + tile_gap);
        float picture_width = 0.0f;
        float picture_height = 0.0f;
        continue_view_tile_picture_size(tile->aspect_width, tile->aspect_height, static_cast<int32_t>(block),
                                        &picture_width, &picture_height);
        draw_picture(draw_list, platform_io, tile_x, tile_y, tile_width, tile_height, picture_width, picture_height,
                     tile->texture, tile->u_max, tile->v_max);
        if (tile->texture == 0)
        {
            // Nothing to show: the dark box says which core it is.
            ImVec2 size = ImGui::GetFont()->CalcTextSizeA(font, FLT_MAX, 0.0f, tile->core);
            float text_width = std::ceil(size.x) < tile_width - 2.0f * block ? std::ceil(size.x) : tile_width - 2.0f * block;
            draw_list->PushClipRect(ImVec2(tile_x, tile_y), ImVec2(tile_x + tile_width, tile_y + tile_height), true);
            draw_clipped(draw_list, font, std::floor(tile_x + (tile_width - text_width) * 0.5f),
                         std::floor(tile_y + (tile_height - text_height) * 0.5f), tile_width - 2.0f * block, tile->core,
                         palette->text_disabled);
            draw_list->PopClipRect();
        }
        float caption_height = 3.0f * block + text_height + 2.0f * block + text_height;
        char id[16] = {};
        std::snprintf(id, sizeof(id), "tile%u", i);
        bool hovered = false;
        if (click_area(id, tile_x, tile_y, tile_width, tile_height + caption_height, &hovered) && card->active)
        {
            action->tile_chosen = static_cast<int32_t>(i);
        }
        bool focused = card->active && card->focus_tile == static_cast<int32_t>(i);
        if (focused)
        {
            draw_frame(draw_list, tile_x, tile_y, tile_width, tile_height, block, 0xFFFFFFFFu);
        }
        else if (hovered && card->active)
        {
            draw_frame(draw_list, tile_x, tile_y, tile_width, tile_height, block, palette->highlight);
        }
        uint32_t name_color = focused || (hovered && card->active) ? palette->text_highlight : palette->text;
        float caption_y = tile_y + tile_height + 3.0f * block;
        draw_clipped(draw_list, font, tile_x, caption_y, tile_width, tile->game, name_color);
        draw_clipped(draw_list, font, tile_x, caption_y + text_height + 2.0f * block, tile_width, tile->line,
                     palette->text_disabled);
    }
}

// The list of the short window: at the card's right, a thin line on its
// left, the header with the page indicator, then one row per game (the name,
// "core · when" dim under it), four at most.
static void draw_list_column(ImDrawList* draw_list, const ContinueViewCard* card, float block, float x, float y,
                             float width, float height, ContinueViewAction* action)
{
    const RetroUiPalette* palette = retro_ui_palette();
    const ContinueViewShelf* shelf = &card->shelf;
    float pad = PANEL_PADDING * block;
    float line_gap = LINE_GAP * block;
    float font = ImGui::GetFontSize();
    float text_height = std::ceil(font);
    draw_list->AddRectFilled(ImVec2(x, y + pad), ImVec2(x + block, y + height - pad),
                             retro_ui_color_imgui(palette->button_face));
    float inner_x = x + pad;
    float inner_width = width - 2.0f * pad;
    float top = y + pad;
    draw_list->AddText(ImGui::GetFont(), font, ImVec2(inner_x, top), retro_ui_color_imgui(palette->section),
                       shelf->header);
    draw_pager(draw_list, shelf, font, block, inner_x + inner_width, top, action);
    float line_y = top + text_height + line_gap;
    draw_list->AddRectFilled(ImVec2(inner_x, line_y), ImVec2(inner_x + inner_width, line_y + block),
                             retro_ui_color_imgui(palette->button_face));
    float row_pad = 2.0f * block;
    float row_height = 2.0f * text_height + line_gap + 2.0f * row_pad;
    float row_y = line_y + block + 3.0f * block;
    for (uint32_t i = 0; i < shelf->tile_count; i++)
    {
        const ContinueViewTile* tile = &shelf->tiles[i];
        char id[16] = {};
        std::snprintf(id, sizeof(id), "row%u", i);
        bool hovered = false;
        if (click_area(id, inner_x, row_y, inner_width, row_height, &hovered) && card->active)
        {
            action->tile_chosen = static_cast<int32_t>(i);
        }
        bool focused = card->active && card->focus_tile == static_cast<int32_t>(i);
        bool lit = card->active && hovered;
        if (focused)
        {
            draw_frame(draw_list, inner_x, row_y, inner_width, row_height, block, 0xFFFFFFFFu);
        }
        else if (lit)
        {
            draw_frame(draw_list, inner_x, row_y, inner_width, row_height, block, palette->highlight);
        }
        float text_x = inner_x + row_pad + block;
        float text_width = inner_width - 2.0f * (row_pad + block);
        draw_clipped(draw_list, font, text_x, row_y + row_pad, text_width, tile->game,
                     focused || lit ? palette->text_highlight : palette->text);
        draw_clipped(draw_list, font, text_x, row_y + row_pad + text_height + line_gap, text_width, tile->line,
                     palette->text_disabled);
        row_y += row_height + block;
    }
}

ContinueViewAction continue_view_draw_panel(ImFont* pixel_font, int32_t scale, const ContinueViewCard* card,
                                            float top, float screen_width, float screen_height)
{
    ContinueViewAction action;
    float block = static_cast<float>(scale);
    const ImGuiIO& io = ImGui::GetIO();
    retro_ui_push_theme(pixel_font, scale);
    const RetroUiPalette* palette = retro_ui_palette();

    float pad = PANEL_PADDING * block;
    float gap = PANEL_GAP * block;
    float line_gap = LINE_GAP * block;
    float font = ImGui::GetFontSize();
    float big = PIXEL_FONT_IMGUI_UNIT * (block + 1.0f);
    float button_height = BUTTON_HEIGHT * block;

    // Taller than wide: stacked. If it doesn't fit the height, the hint
    // goes first, then the picture gets shorter (never below a third).
    ContinueViewLayout layout = continue_view_panel_layout(screen_width, screen_height, scale);
    bool stacked = layout == ContinueViewLayout::stacked_shelf;
    bool list = layout == ContinueViewLayout::side_list;
    float area_height = screen_height - top;
    PanelMeasure m;
    float picture_room = CONTINUE_VIEW_STACKED_PICTURE_HEIGHT;
    panel_measure(&m, card, scale, stacked, list, true, picture_room);
    if (m.height > area_height && m.hint)
    {
        panel_measure(&m, card, scale, stacked, list, false, picture_room);
    }
    if (stacked && m.height > area_height)
    {
        float excess = (m.height - area_height) / block;
        picture_room = CONTINUE_VIEW_STACKED_PICTURE_HEIGHT - excess;
        float least = CONTINUE_VIEW_STACKED_PICTURE_HEIGHT / 3.0f;
        picture_room = picture_room > least ? picture_room : least;
        panel_measure(&m, card, scale, stacked, list, false, picture_room);
    }
    float x = std::floor((io.DisplaySize.x - m.width) * 0.5f);
    float y = std::floor(top + (area_height - m.height) * 0.5f);
    y = y > top ? y : top;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(m.width, m.height), ImGuiCond_Always);
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

    float text_x = x + pad;
    float text_y = y + pad;
    float column = m.column;
    if (stacked)
    {
        // The picture across the card's width, then the words under it.
        draw_picture(draw_list, &platform_io, x + pad, y + pad, column, m.picture_height, m.picture_width,
                     m.picture_height, card->texture, card->u_max, card->v_max);
        text_y = y + pad + m.picture_height + gap;
        text_y += draw_wrapped(draw_list, big, text_x, text_y, column, card->game, palette->text_highlight) + line_gap;
        char line[CLIP_TEXT_MAX] = {};
        core_when_line(line, sizeof(line), card);
        text_y += draw_clipped(draw_list, font, text_x, text_y, column, line, palette->text_disabled);
        if (card->reason[0] != '\0')
        {
            text_y += line_gap;
            draw_wrapped(draw_list, font, text_x, text_y, column, card->reason, palette->text);
        }
    }
    else
    {
        // The picture centered by the text in its square room; black without one.
        float room = CONTINUE_VIEW_PICTURE_WIDTH * block;
        float room_height = m.inner_height;
        draw_picture(draw_list, &platform_io, x + pad + (room - m.picture_width) * 0.5f,
                     std::floor(y + pad + (room_height - m.picture_height) * 0.5f), m.picture_width, m.picture_height,
                     m.picture_width, m.picture_height, card->texture, card->u_max, card->v_max);
        text_x = x + pad + room + gap;
        text_y += draw_wrapped(draw_list, big, text_x, text_y, column, card->game, palette->text_highlight) + line_gap;
        text_y += draw_wrapped(draw_list, font, text_x, text_y, column, card->core, palette->text_disabled) + line_gap;
        text_y += draw_wrapped(draw_list, font, text_x, text_y, column, card->when, palette->text_disabled);
        if (card->reason[0] != '\0')
        {
            text_y += line_gap;
            draw_wrapped(draw_list, font, text_x, text_y, column, card->reason, palette->text);
        }
    }

    // The buttons at the bottom of the card's column (the hint under them).
    float button_y = y + pad + m.inner_height - m.bottom_height + BUTTON_GAP * block;
    bool on_buttons = card->active && card->focus_tile < 0;
    if (card->second_label[0] == '\0')
    {
        if (panel_button("first", card->first_label, text_x, button_y, column, button_height, on_buttons))
        {
            action.pressed = card->first;
        }
    }
    else
    {
        float half = std::floor((column - BUTTON_GAP * block) * 0.5f);
        if (panel_button("first", card->first_label, text_x, button_y, half, button_height,
                         on_buttons && !card->focus_second))
        {
            action.pressed = card->first;
        }
        if (panel_button("second", card->second_label, text_x + column - half, button_y, half, button_height,
                         on_buttons && card->focus_second))
        {
            action.pressed = card->second;
        }
    }
    if (m.hint)
    {
        draw_wrapped(draw_list, font, text_x, button_y + button_height + line_gap, column, card->hint,
                     palette->text_disabled);
    }

    if (card->shelf.tile_count > 0 && list)
    {
        float card_width = std::floor(CONTINUE_VIEW_PANEL_WIDTH * block);
        draw_list_column(draw_list, card, block, x + card_width, y, m.width - card_width, m.height, &action);
    }
    else if (card->shelf.tile_count > 0)
    {
        draw_shelf(draw_list, &platform_io, card, block, x, y + pad + m.inner_height + pad, m.width, &action);
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
