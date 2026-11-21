#include "dvr_view.h"

#include <cmath>

#include "imgui.h"
#include "retro_ui.h"

// Sizes in retro pixels (times `scale` window pixels).
constexpr float MARGIN      = 4.0f;    // from the screen's sides
constexpr float GAP         = 2.0f;    // between the strip's rows
constexpr float HEADER      = 13.0f;
constexpr float CHIP        = 11.0f;   // a key chip (gamepad hints, the title)
constexpr float CHIP_LIP    = 2.0f;
constexpr float CHIP_PAD    = 4.0f;
constexpr float BUTTON      = 17.0f;   // a touch button
constexpr float BUTTON_PAD  = 8.0f;
constexpr float HINTS       = 15.0f;   // the hints' card
constexpr float LINE        = 7.0f;    // a line of the pixel font
constexpr float LABEL       = 9.0f;    // a thumbnail's time under it
constexpr float TRACK       = 6.0f;
constexpr float THUMB_GAP   = 6.0f;
// Thumbnail heights (their widths follow the pictures' aspect).
constexpr float THUMB       = 56.0f;
constexpr float THUMB_BIG   = 70.0f;
constexpr float TOUCH_THUMB = 42.0f;
constexpr float TOUCH_BIG   = 49.0f;

// Over the game: cards it shows through (ports_view's).
constexpr uint32_t CARD_ALPHA = 0xB0000000u;
constexpr uint32_t DIM_ALPHA  = 0x9E000000u;   // a thumbnail that isn't selected (62%)
constexpr uint32_t AFTER_ALPHA = 0x66000000u;  // one after the cursor (40%)


static bool clicked_in(float x, float y, float width, float height)
{
    const ImGuiIO& io = ImGui::GetIO();
    return io.MouseClicked[0] && io.MousePos.x >= x && io.MousePos.x < x + width && io.MousePos.y >= y &&
           io.MousePos.y < y + height;
}

// A chip: its face, its bottom rows darker, the text centred in what's
// above the lip. Returns its width (window pixels).
static float draw_chip(float x, float y, float height, float pad, const char* label, uint32_t face, uint32_t color,
                       float block)
{
    float width = std::floor(retro_ui_layer_text_width(label)) + 2.0f * pad * block;
    float lip = CHIP_LIP * block;
    retro_ui_layer_rect(x, y, width, height - lip, face);
    retro_ui_layer_rect(x, y + height - lip, width, lip, retro_ui_color_darker(face));
    float text_y = y + std::floor((height / block - CHIP_LIP - LINE) * 0.5f) * block;
    retro_ui_layer_text(x + pad * block, text_y, label, color);
    return width;
}

static float thumb_height(const DvrView* view, bool big)
{
    if (view->touch)
    {
        return big ? TOUCH_BIG : TOUCH_THUMB;
    }
    return big ? THUMB_BIG : THUMB;
}

float dvr_view_height(const DvrView* view, int32_t scale)
{
    float rows = GAP + HEADER + GAP + thumb_height(view, true) + GAP + LABEL;
    rows += view->touch ? GAP + BUTTON + GAP : TRACK + GAP + HINTS + GAP;
    return rows * static_cast<float>(scale > 0 ? scale : 1);
}

static void draw_header(const DvrView* view, const DvrViewText* text, float top, float width, float block,
                        DvrViewInput* input)
{
    const RetroUiPalette* palette = retro_ui_palette();
    float y = top + GAP * block;
    retro_ui_layer_rect(MARGIN * block, y, width - 2.0f * MARGIN * block, HEADER * block,
                        retro_ui_color_set_alpha(palette->panel_dark, CARD_ALPHA));
    float x = (MARGIN + 2.0f) * block;
    x += draw_chip(x, y + block, CHIP * block, CHIP_PAD, text->title, palette->highlight, palette->text_highlight, block);
    float text_y = y + 2.0f * block;
    retro_ui_layer_text(x + 6.0f * block, text_y, text->mode, palette->text);

    // The exit button at the right; focused (UP): lit, with a frame.
    float right = width - (MARGIN + 2.0f) * block;
    if (text->exit[0] != '\0')
    {
        float exit_w = std::floor(retro_ui_layer_text_width(text->exit)) + 2.0f * CHIP_PAD * block;
        float exit_x = right - exit_w;
        if (view->exit_focused)
        {
            retro_ui_layer_rect(exit_x - block, y, exit_w + 2.0f * block, HEADER * block, palette->text_highlight);
        }
        draw_chip(exit_x, y + block, CHIP * block, CHIP_PAD, text->exit,
                  view->exit_focused ? palette->highlight : palette->button_face,
                  view->exit_focused ? palette->text_highlight : palette->button_text, block);
        if (clicked_in(exit_x, y + block, exit_w, CHIP * block))
        {
            input->action = DvrViewAction::back_to_now;
        }
        right = exit_x - 8.0f * block;
    }
    else
    {
        right -= 2.0f * block;
    }
    float readout = std::floor(retro_ui_layer_text_width(text->readout));
    retro_ui_layer_text(right - readout, text_y, text->readout, palette->section);
    if (text->kept[0] != '\0')
    {
        float kept = std::floor(retro_ui_layer_text_width(text->kept));
        retro_ui_layer_text(right - readout - 12.0f * block - kept, text_y, text->kept, palette->text_disabled);
    }
}

// The thumbnails, the selected one centred; returns each one's centre (for
// the track) through `centres`.
static void draw_thumbs(const DvrView* view, float top, float width, float block, float* centres, float* label_y,
                        DvrViewInput* input)
{
    const RetroUiPalette* palette = retro_ui_palette();
    float small_h = thumb_height(view, false) * block;
    float big_h = thumb_height(view, true) * block;
    float small_w = std::floor(thumb_height(view, false) * view->aspect) * block;
    float big_w = std::floor(thumb_height(view, true) * view->aspect) * block;
    float gap = THUMB_GAP * block;
    float big_x = std::floor((width - big_w) * 0.5f / block) * block;
    float big_y = top;
    float small_y = top + std::floor((big_h - small_h) * 0.5f / block) * block;
    *label_y = top + big_h + GAP * block;

    for (uint32_t i = 0; i < view->count && i < DVR_VIEW_MAX_THUMBS; i++)
    {
        const DvrViewThumb* thumb = &view->thumbs[i];
        bool big = i == view->selected;
        float x = big_x;
        if (i < view->selected)
        {
            x = big_x - static_cast<float>(view->selected - i) * (small_w + gap);
        }
        else if (i > view->selected)
        {
            x = big_x + big_w + gap + static_cast<float>(i - view->selected - 1) * (small_w + gap);
        }
        float w = big ? big_w : small_w;
        float h = big ? big_h : small_h;
        float y = big ? big_y : small_y;
        centres[i] = x + std::floor(w * 0.5f);
        if (x + w < 0.0f || x > width)
        {
            continue;
        }
        if (big)
        {
            retro_ui_layer_rect(x - 2.0f * block, y - 2.0f * block, w + 4.0f * block, h + 4.0f * block,
                                palette->text_highlight);
        }
        else
        {
            retro_ui_layer_rect(x - block, y - block, w + 2.0f * block, h + 2.0f * block,
                                retro_ui_color_set_alpha(palette->panel_face, CARD_ALPHA));
        }
        if (thumb->texture != 0)
        {
            uint32_t tint = big ? 0xFFFFFFFFu : retro_ui_color_set_alpha(0xFFFFFFu, thumb->after ? AFTER_ALPHA : DIM_ALPHA);
            if (!big)
            {
                retro_ui_layer_rect(x, y, w, h, 0xFF000000u);   // dimmed toward black, not the card
            }
            retro_ui_layer_image(thumb->texture, x, y, w, h, thumb->u_max, thumb->v_max, tint);
        }
        if (thumb->after && !big)
        {
            for (float k = 0.0f; k < w; k += 4.0f * block)   // a dotted edge: kept until PLAY FROM HERE
            {
                retro_ui_layer_rect(x + k, y + h - 3.0f * block, 2.0f * block, 2.0f * block, palette->text_disabled);
            }
        }
        if (!view->touch || big)
        {
            float label_w = std::floor(retro_ui_layer_text_width(thumb->label));
            retro_ui_layer_text(x + std::floor((w - label_w) * 0.5f / block) * block, *label_y, thumb->label,
                                big ? palette->text_highlight : palette->text_disabled);
        }
        if (clicked_in(x, y, w, h))
        {
            input->clicked = static_cast<int32_t>(i);
        }
    }
}

static void draw_track(const DvrView* view, float y, float width, float block, const float* centres)
{
    const RetroUiPalette* palette = retro_ui_palette();
    retro_ui_layer_rect(0.0f, y, width, block, palette->button_face);
    uint32_t count = view->count < DVR_VIEW_MAX_THUMBS ? view->count : DVR_VIEW_MAX_THUMBS;
    for (uint32_t i = 0; i < count; i++)
    {
        retro_ui_layer_rect(centres[i], y - 2.0f * block, block, 5.0f * block, palette->panel_light);
    }
    if (count == 0)
    {
        return;
    }
    float x = centres[view->selected];
    if (view->selected + 1 < count)
    {
        x += std::floor((centres[view->selected + 1] - x) * view->between / block) * block;
    }
    retro_ui_layer_rect(x - block, y - 4.0f * block, 3.0f * block, 9.0f * block, palette->section);
    if (view->playing)
    {
        retro_ui_layer_text(x - 2.0f * block, y - 13.0f * block, "\xE2\x96\xB6", palette->section);   // ▶
    }
}

static void draw_hints(const DvrView* view, float y, float width, float block, DvrViewInput* input)
{
    const RetroUiPalette* palette = retro_ui_palette();
    uint32_t count = view->hint_count < DVR_VIEW_MAX_HINTS ? view->hint_count : DVR_VIEW_MAX_HINTS;
    if (view->touch)
    {
        // Big buttons, centred.
        float gap = 6.0f * block;
        float total = 0.0f;
        for (uint32_t i = 0; i < count; i++)
        {
            total += std::floor(retro_ui_layer_text_width(view->hints[i].label)) + 2.0f * BUTTON_PAD * block + gap;
        }
        float x = std::floor((width - total + gap) * 0.5f / block) * block;
        for (uint32_t i = 0; i < count; i++)
        {
            const DvrViewHint* hint = &view->hints[i];
            float w = draw_chip(x, y, BUTTON * block, BUTTON_PAD, hint->label,
                                hint->lit ? palette->highlight : palette->button_face,
                                hint->lit ? palette->text_highlight : palette->button_text, block);
            if (clicked_in(x, y, w, BUTTON * block))
            {
                input->action = hint->action;
            }
            x += w + gap;
        }
        return;
    }
    // Gamepad: a card along the bottom, key chips and what they do.
    retro_ui_layer_rect(MARGIN * block, y, width - 2.0f * MARGIN * block, HINTS * block,
                        retro_ui_color_set_alpha(palette->panel_dark, CARD_ALPHA));
    float spacing = 10.0f * block;
    float total = 0.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        total += std::floor(retro_ui_layer_text_width(view->hints[i].key)) + 2.0f * CHIP_PAD * block + 4.0f * block +
                 std::floor(retro_ui_layer_text_width(view->hints[i].label)) + spacing;
    }
    float x = std::floor((width - total + spacing) * 0.5f / block) * block;
    float chip_y = y + 2.0f * block;
    for (uint32_t i = 0; i < count; i++)
    {
        const DvrViewHint* hint = &view->hints[i];
        float start = x;
        x += draw_chip(x, chip_y, CHIP * block, CHIP_PAD, hint->key, hint->lit ? palette->highlight : palette->button_face,
                       palette->text_highlight, block);
        x += 4.0f * block;
        retro_ui_layer_text(x, chip_y + block, hint->label, hint->lit ? palette->text_highlight : palette->text);
        x += std::floor(retro_ui_layer_text_width(hint->label));
        if (clicked_in(start, chip_y, x - start, CHIP * block))
        {
            input->action = hint->action;
        }
        x += spacing;
    }
}

void dvr_view_draw(const DvrView* view, ImFont* pixel_font, int32_t scale, const DvrViewText* text,
                   DvrViewInput* input)
{
    *input = {};
    float block = static_cast<float>(scale > 0 ? scale : 1);
    const ImGuiIO& io = ImGui::GetIO();
    float width = io.DisplaySize.x;
    float height = io.DisplaySize.y;
    float top = height - dvr_view_height(view, scale);
    float centres[DVR_VIEW_MAX_THUMBS] = {};

    retro_ui_begin_layer("##dvr_view", pixel_font, scale, true);
    draw_header(view, text, top, width, block, input);
    float label_y = 0.0f;
    draw_thumbs(view, top + (GAP + HEADER + GAP) * block, width, block, centres, &label_y, input);
    float below = label_y + LABEL * block;
    if (view->touch)
    {
        draw_hints(view, below + GAP * block, width, block, input);
    }
    else
    {
        draw_track(view, below + 2.0f * block, width, block, centres);
        draw_hints(view, below + (TRACK + GAP) * block, width, block, input);
    }
    retro_ui_end_layer();
}
