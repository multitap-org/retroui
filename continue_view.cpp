#include "continue_view.h"

#include <cfloat>
#include <cmath>
#include <cstdio>

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

void continue_view_panel_picture_size(const ContinueViewCard* card, int32_t scale, float* width, float* height)
{
    float block = static_cast<float>(scale);
    continue_view_picture_size(card->aspect_width, card->aspect_height, CONTINUE_VIEW_PICTURE_WIDTH * block,
                               CONTINUE_VIEW_PICTURE_HEIGHT * block, width, height);
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

ContinueViewButton continue_view_draw_panel(ImFont* pixel_font, int32_t scale, const ContinueViewCard* card,
                                            float top)
{
    ContinueViewButton pressed = ContinueViewButton::none;
    float block = static_cast<float>(scale);
    const ImGuiIO& io = ImGui::GetIO();
    retro_ui_push_theme(pixel_font, scale);
    const RetroUiPalette* palette = retro_ui_palette();

    float picture_width = 0.0f;
    float picture_height = 0.0f;
    continue_view_panel_picture_size(card, scale, &picture_width, &picture_height);
    float pad = PANEL_PADDING * block;
    float gap = PANEL_GAP * block;
    float info = CONTINUE_VIEW_INFO_WIDTH * block;
    float line_gap = LINE_GAP * block;
    float font = ImGui::GetFontSize();
    float big = PIXEL_FONT_IMGUI_UNIT * (block + 1.0f);   // one whole step up: still sharp
    float button_height = BUTTON_HEIGHT * block;

    // The text column, top down, then the buttons and the hint at its bottom.
    float text_height = wrapped_height(big, info, card->game) + line_gap + wrapped_height(font, info, card->core) +
                        line_gap + wrapped_height(font, info, card->when);
    if (card->reason[0] != '\0')
    {
        text_height += line_gap + wrapped_height(font, info, card->reason);
    }
    float bottom_height = BUTTON_GAP * block + button_height;
    if (card->hint[0] != '\0')
    {
        bottom_height += line_gap + wrapped_height(font, info, card->hint);
    }
    float inner_height = text_height + bottom_height;
    inner_height = inner_height > picture_height ? inner_height : picture_height;
    float width = std::floor(pad + picture_width + gap + info + pad);
    float height = std::floor(pad + inner_height + pad);
    float room = io.DisplaySize.y - top;
    float x = std::floor((io.DisplaySize.x - width) * 0.5f);
    float y = std::floor(top + (room - height) * 0.5f);
    y = y > top ? y : top;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, block);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, retro_ui_color_vec4(palette->field_face));
    ImGui::PushStyleColor(ImGuiCol_Border, retro_ui_color_vec4(palette->button_face));
    // Part of the empty screen: never focused, never in front, no title.
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

    // The picture, vertically centered by the text; black without one.
    float picture_x = x + pad;
    float picture_y = std::floor(y + pad + (inner_height - picture_height) * 0.5f);
    draw_list->AddRectFilled(ImVec2(picture_x, picture_y), ImVec2(picture_x + picture_width, picture_y + picture_height),
                             retro_ui_color_imgui(0xFF000000u));
    if (card->texture != 0)
    {
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
        draw_list->AddImage(static_cast<ImTextureID>(card->texture), ImVec2(picture_x, picture_y),
                            ImVec2(picture_x + picture_width, picture_y + picture_height), ImVec2(0.0f, 0.0f),
                            ImVec2(card->u_max, card->v_max));
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);
    }

    float text_x = x + pad + picture_width + gap;
    float text_y = y + pad;
    text_y += draw_wrapped(draw_list, big, text_x, text_y, info, card->game, palette->text_highlight) + line_gap;
    text_y += draw_wrapped(draw_list, font, text_x, text_y, info, card->core, palette->text_disabled) + line_gap;
    text_y += draw_wrapped(draw_list, font, text_x, text_y, info, card->when, palette->text_disabled);
    if (card->reason[0] != '\0')
    {
        text_y += line_gap;
        draw_wrapped(draw_list, font, text_x, text_y, info, card->reason, palette->text);
    }

    // The buttons at the bottom of the column (the hint under them).
    float button_y = y + pad + inner_height - bottom_height + BUTTON_GAP * block;
    if (card->second_label[0] == '\0')
    {
        if (panel_button("first", card->first_label, text_x, button_y, info, button_height, card->active))
        {
            pressed = card->first;
        }
    }
    else
    {
        float half = std::floor((info - BUTTON_GAP * block) * 0.5f);
        if (panel_button("first", card->first_label, text_x, button_y, half, button_height, card->active))
        {
            pressed = card->first;
        }
        if (panel_button("second", card->second_label, text_x + info - half, button_y, half, button_height, false))
        {
            pressed = card->second;
        }
    }
    if (card->hint[0] != '\0')
    {
        draw_wrapped(draw_list, font, text_x, button_y + button_height + line_gap, info, card->hint,
                     palette->text_disabled);
    }
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    retro_ui_pop_theme();
    return card->active ? pressed : ContinueViewButton::none;
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
