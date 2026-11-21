#include "hud_view.h"

#include <cmath>
#include <cstdio>

#include "pixel_font.h"
#include "retro_ui.h"

constexpr float LINE = 7.0f;              // a line of the pixel font (glyphs 1..5 of it)
constexpr float CHIP = 11.0f;             // the title chip
constexpr float CHIP_LIP = 2.0f;          // a chip's bottom rows, darker (piccoletto's buttons)
constexpr float CHIP_PADDING = 4.0f;
constexpr float KEY = 10.0f;              // the key chip on the second line
constexpr float PAD_LEFT = 6.0f;          // inside a card, after its coloured edge
constexpr float PAD_RIGHT = 5.0f;
constexpr float PAD_TOP = 3.0f;
constexpr float EDGE = 2.0f;              // the card's coloured left edge
constexpr float BAR = 2.0f;               // the countdown bar
constexpr float SECOND_LINE = 13.0f;      // from the title chip's top to the second line's
constexpr float ONE_LINE_HEIGHT = 17.0f;
constexpr float TWO_LINE_HEIGHT = 30.0f;
constexpr float ICON_GLYPH = 7.0f;        // the icon's picture, centred in its chip
constexpr float SLIDE = 12.0f;            // a new card slides in this far from the right
constexpr uint64_t SLIDE_MS = 150;        // like QUICK MENU's slide

constexpr uint32_t CARD_ALPHA = 0xC8000000u;

static void copy_text(char* out, size_t size, const char* text)
{
    std::snprintf(out, size, "%s", text != nullptr ? text : "");
}

static float text_width(const char* text)
{
    int32_t width = 0;
    int32_t height = 0;
    pixel_font_measure(text, 1, &width, &height);
    return static_cast<float>(width);
}

static bool has_second_line(const HudCard* card)
{
    return card->before[0] != '\0' || card->key[0] != '\0' || card->after[0] != '\0';
}

void hud_view_clear(HudStack* stack)
{
    stack->card_count = 0;
    stack->icon_count = 0;
}

bool hud_view_add_card(HudStack* stack, const HudCard* card)
{
    if (stack->card_count >= HUD_VIEW_MAX_CARDS)
    {
        return false;
    }
    stack->cards[stack->card_count] = *card;
    stack->card_count++;
    return true;
}

bool hud_view_add_icon(HudStack* stack, HudIcon icon, const char* label)
{
    if (stack->icon_count >= HUD_VIEW_MAX_ICONS)
    {
        return false;
    }
    HudIconChip* chip = &stack->icons[stack->icon_count];
    chip->icon = icon;
    copy_text(chip->label, sizeof(chip->label), label);
    stack->icon_count++;
    return true;
}

float hud_view_card_height(const HudCard* card)
{
    return has_second_line(card) ? TWO_LINE_HEIGHT : ONE_LINE_HEIGHT;
}

// Newer cards go below older ones: a card's place counts the cards newer
// than it (later since_ms; the same since_ms: added later).
static bool newer(const HudStack* stack, uint32_t a, uint32_t b)
{
    uint64_t since_a = stack->cards[a].since_ms;
    uint64_t since_b = stack->cards[b].since_ms;
    return since_a > since_b || (since_a == since_b && a > b);
}

HudRect hud_view_card_rect(const HudStack* stack, const HudArea* area, uint32_t index)
{
    HudRect rect = {};
    if (index >= stack->card_count)
    {
        return rect;
    }
    float below = 0.0f;
    for (uint32_t i = 0; i < stack->card_count; i++)
    {
        if (i != index && newer(stack, i, index))
        {
            below += hud_view_card_height(&stack->cards[i]) + HUD_VIEW_GAP;
        }
    }
    rect.width = HUD_VIEW_CARD_WIDTH;
    rect.height = hud_view_card_height(&stack->cards[index]);
    rect.x = area->right - HUD_VIEW_MARGIN - rect.width;
    rect.y = area->bottom - below - rect.height;
    rect.shown = rect.y >= area->top && rect.x >= 0.0f;
    return rect;
}

static float icon_width(const HudIconChip* chip)
{
    if (chip->label[0] == '\0')
    {
        return HUD_VIEW_ICON;
    }
    return HUD_VIEW_ICON + text_width(chip->label) + 2.0f;
}

HudRect hud_view_icon_rect(const HudStack* stack, const HudArea* area, uint32_t index)
{
    HudRect rect = {};
    if (index >= stack->icon_count)
    {
        return rect;
    }
    float right = area->right - HUD_VIEW_MARGIN;
    for (uint32_t i = 0; i < index; i++)
    {
        right -= icon_width(&stack->icons[i]) + HUD_VIEW_GAP;
    }
    rect.width = icon_width(&stack->icons[index]);
    rect.height = HUD_VIEW_ICON;
    rect.x = right - rect.width;
    rect.y = area->top + HUD_VIEW_MARGIN;
    rect.shown = rect.x >= 0.0f && rect.y + rect.height <= area->bottom;
    return rect;
}

// The icon's 7 x 7 picture at (x, y) in window pixels.
static void draw_icon_glyph(HudIcon icon, float x, float y, float block, uint32_t color)
{
    switch (icon)
    {
        case HudIcon::play:
            for (int32_t row = 0; row < 7; row++)
            {
                float width = static_cast<float>(4 - std::abs(3 - row) + 1);
                retro_ui_layer_rect(x + block, y + static_cast<float>(row) * block, width * block, block, color);
            }
            break;
        case HudIcon::pause:
            retro_ui_layer_rect(x + block, y, 2.0f * block, 7.0f * block, color);
            retro_ui_layer_rect(x + 4.0f * block, y, 2.0f * block, 7.0f * block, color);
            break;
        case HudIcon::rec:
        {
            static const int32_t WIDTHS[7] = {3, 5, 7, 7, 7, 5, 3};
            for (int32_t row = 0; row < 7; row++)
            {
                float width = static_cast<float>(WIDTHS[row]);
                retro_ui_layer_rect(x + std::floor((7.0f - width) * 0.5f) * block, y + static_cast<float>(row) * block,
                                    width * block, block, color);
            }
            break;
        }
        case HudIcon::rewind:
        case HudIcon::fast_forward:
            for (int32_t row = 0; row < 7; row++)
            {
                float width = static_cast<float>(4 - std::abs(3 - row));
                float row_y = y + static_cast<float>(row) * block;
                if (icon == HudIcon::fast_forward)
                {
                    retro_ui_layer_rect(x, row_y, width * block, block, color);
                    retro_ui_layer_rect(x + 4.0f * block, row_y, width * block, block, color);
                }
                else
                {
                    retro_ui_layer_rect(x + (3.0f - width) * block, row_y, width * block, block, color);
                    retro_ui_layer_rect(x + (7.0f - width) * block, row_y, width * block, block, color);
                }
            }
            break;
    }
}

static void draw_icons(const HudStack* stack, const HudArea* area, float block)
{
    const RetroUiPalette* palette = retro_ui_palette();
    for (uint32_t i = 0; i < stack->icon_count; i++)
    {
        HudRect rect = hud_view_icon_rect(stack, area, i);
        if (!rect.shown)
        {
            continue;
        }
        const HudIconChip* chip = &stack->icons[i];
        // REC is the one that isn't a state of play: a dark chip, a red dot.
        bool rec = chip->icon == HudIcon::rec;
        uint32_t face = rec ? retro_ui_color_set_alpha(palette->panel_dark, CARD_ALPHA) : palette->highlight;
        uint32_t ink = rec ? 0xFFE05050u : palette->text_highlight;
        float x = rect.x * block;
        float y = rect.y * block;
        retro_ui_layer_rect(x, y, rect.width * block, (rect.height - CHIP_LIP) * block, face);
        retro_ui_layer_rect(x, y + (rect.height - CHIP_LIP) * block, rect.width * block, CHIP_LIP * block,
                            retro_ui_color_darker(face));
        float inset = std::floor((HUD_VIEW_ICON - ICON_GLYPH) * 0.5f);
        draw_icon_glyph(chip->icon, x + inset * block, y + (inset - 1.0f) * block, block, ink);
        if (chip->label[0] != '\0')
        {
            retro_ui_layer_text(x + (HUD_VIEW_ICON - 1.0f) * block, y + 2.0f * block, chip->label, ink);
        }
    }
}

static void draw_card(const HudCard* card, HudRect rect, float block, uint64_t now_ms)
{
    const RetroUiPalette* palette = retro_ui_palette();
    float in = retro_ui_ease_out(now_ms, card->since_ms, SLIDE_MS);
    float fade = card->alpha * in;
    float x = (rect.x + std::floor((1.0f - in) * SLIDE)) * block;
    float y = rect.y * block;
    float width = rect.width * block;
    float height = rect.height * block;

    retro_ui_layer_rect(x, y, width, height, retro_ui_color_with_alpha(retro_ui_color_set_alpha(palette->panel_dark, CARD_ALPHA), fade));
    retro_ui_layer_rect(x, y, EDGE * block, height, retro_ui_color_with_alpha(palette->highlight, fade));

    // The title line: chip, title, corner.
    float line_x = x + PAD_LEFT * block;
    float chip_y = y + PAD_TOP * block;
    if (card->chip[0] != '\0')
    {
        float chip_width = (text_width(card->chip) + 2.0f * CHIP_PADDING) * block;
        uint32_t face = retro_ui_color_with_alpha(palette->highlight, fade);
        retro_ui_layer_rect(line_x, chip_y, chip_width, (CHIP - CHIP_LIP) * block, face);
        retro_ui_layer_rect(line_x, chip_y + (CHIP - CHIP_LIP) * block, chip_width, CHIP_LIP * block, retro_ui_color_darker(face));
        retro_ui_layer_text(line_x + CHIP_PADDING * block, chip_y + block, card->chip,
                            retro_ui_color_with_alpha(palette->text_highlight, fade));
        line_x += chip_width + 4.0f * block;
    }
    float corner_width = card->corner[0] != '\0' ? (text_width(card->corner) + 4.0f) * block : 0.0f;
    retro_ui_layer_clip(line_x, y, x + width - PAD_RIGHT * block - corner_width - line_x, height);
    retro_ui_layer_text(line_x, chip_y + 2.0f * block, card->title, retro_ui_color_with_alpha(palette->text, fade));
    retro_ui_layer_unclip();
    if (card->corner[0] != '\0')
    {
        float corner_x = x + width - PAD_RIGHT * block - text_width(card->corner) * block;
        retro_ui_layer_text(corner_x, chip_y + 2.0f * block, card->corner, retro_ui_color_with_alpha(palette->text_disabled, fade));
    }

    // The second line: before [key] after.
    if (has_second_line(card))
    {
        float second_y = chip_y + SECOND_LINE * block;
        float second_x = x + PAD_LEFT * block;
        retro_ui_layer_clip(second_x, y, width - (PAD_LEFT + PAD_RIGHT) * block, height);
        if (card->before[0] != '\0')
        {
            retro_ui_layer_text(second_x, second_y + block, card->before, retro_ui_color_with_alpha(palette->text, fade));
            second_x += (text_width(card->before) + 4.0f) * block;
        }
        if (card->key[0] != '\0')
        {
            float key_width = (text_width(card->key) + 2.0f * CHIP_PADDING) * block;
            uint32_t face = retro_ui_color_with_alpha(palette->button_face, fade);
            retro_ui_layer_rect(second_x, second_y, key_width, (KEY - CHIP_LIP) * block, face);
            float fill = card->key_fill < 0.0f ? 0.0f : (card->key_fill > 1.0f ? 1.0f : card->key_fill);
            if (fill > 0.0f)
            {
                retro_ui_layer_rect(second_x, second_y, std::floor(key_width * fill), (KEY - CHIP_LIP) * block,
                                    retro_ui_color_with_alpha(palette->highlight, fade));
            }
            retro_ui_layer_rect(second_x, second_y + (KEY - CHIP_LIP) * block, key_width, CHIP_LIP * block,
                                retro_ui_color_darker(face));
            retro_ui_layer_text(second_x + CHIP_PADDING * block, second_y, card->key,
                                retro_ui_color_with_alpha(palette->button_text, fade));
            second_x += key_width + 4.0f * block;
        }
        if (card->after[0] != '\0')
        {
            retro_ui_layer_text(second_x, second_y + block, card->after, retro_ui_color_with_alpha(palette->text, fade));
        }
        retro_ui_layer_unclip();
    }

    if (card->bar >= 0.0f)
    {
        float bar = card->bar > 1.0f ? 1.0f : card->bar;
        float bar_y = y + height - BAR * block;
        float bar_width = width - EDGE * block;
        retro_ui_layer_rect(x + EDGE * block, bar_y, bar_width, BAR * block, retro_ui_color_with_alpha(palette->field_face, fade));
        retro_ui_layer_rect(x + EDGE * block, bar_y, std::floor(bar_width * bar), BAR * block,
                            retro_ui_color_with_alpha(palette->highlight, fade));
    }
}

void hud_view_draw(const HudStack* stack, const HudArea* area, ImFont* pixel_font, int32_t scale, uint64_t now_ms)
{
    if (stack->card_count == 0 && stack->icon_count == 0)
    {
        return;
    }
    float block = static_cast<float>(scale > 0 ? scale : 1);
    retro_ui_begin_layer("##hud", pixel_font, scale, false);
    draw_icons(stack, area, block);
    for (uint32_t i = 0; i < stack->card_count; i++)
    {
        HudRect rect = hud_view_card_rect(stack, area, i);
        if (rect.shown)
        {
            draw_card(&stack->cards[i], rect, block, now_ms);
        }
    }
    retro_ui_end_layer();
}
