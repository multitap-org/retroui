#include "touch_ui.h"

#include <cfloat>
#include <cmath>
#include <cstdio>

#include "imgui.h"

#include "pixel_font_imgui.h"

// Retro pixels across the short side of the screen (a phone's 390 dp at 2 dp
// per retro pixel: the mockups' size).
constexpr float SHORT_SIDE_RETRO_PIXELS = 195.0f;

// The lip under normal and primary buttons, in retro pixels, and the black
// that darkens the face there (28%: darker than any theme's face).
constexpr float BUTTON_LIP = 2.0f;
constexpr uint32_t LIP_SHADE = 0x48000000u;

// Window pixels per font pixel for a text size.
static float font_scale(const TouchUi* ui, TouchText size)
{
    if (size == TouchText::big)
    {
        return static_cast<float>((ui->scale * 3) / 2);
    }
    return static_cast<float>(ui->scale);
}

static float font_size(const TouchUi* ui, TouchText size)
{
    return PIXEL_FONT_IMGUI_UNIT * font_scale(ui, size);
}

// Retro pixels to window pixels, on whole pixels.
static float px(const TouchUi* ui, float retro)
{
    return std::floor(retro * static_cast<float>(ui->scale));
}

int32_t touch_ui_scale_for(float width, float height)
{
    float short_side = width < height ? width : height;
    int32_t scale = static_cast<int32_t>(short_side / SHORT_SIDE_RETRO_PIXELS);
    return scale < 1 ? 1 : scale;
}

void touch_ui_begin_screen(const TouchUi* ui, const char* id)
{
    const ImGuiIO& io = ImGui::GetIO();
    // Gamepad navigation needs a focused window: the screen's when none is
    // (at start, after a sheet closes).
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
    {
        ImGui::SetNextWindowFocus();
    }
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, retro_ui_color_imgui(ui->palette.background));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove;
    ImGui::Begin(id, nullptr, flags);
    // The pixel font must be sampled with nearest filtering to stay sharp.
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);
}

void touch_ui_begin_overlay(const TouchUi* ui, const char* id)
{
    (void)ui;
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowFocus();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBackground;
    ImGui::Begin(id, nullptr, flags);
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);
}

void touch_ui_end_screen()
{
    ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

float touch_ui_width(const TouchUi* ui)
{
    return std::floor(ImGui::GetIO().DisplaySize.x / static_cast<float>(ui->scale));
}

float touch_ui_height(const TouchUi* ui)
{
    return std::floor(ImGui::GetIO().DisplaySize.y / static_cast<float>(ui->scale));
}

float touch_ui_text_width(const TouchUi* ui, const char* text, TouchText size)
{
    ImVec2 measured = ui->font->CalcTextSizeA(font_size(ui, size), FLT_MAX, 0.0f, text);
    return measured.x / static_cast<float>(ui->scale);
}

float touch_ui_text_height(const TouchUi* ui, TouchText size)
{
    return font_size(ui, size) / static_cast<float>(ui->scale);
}

float touch_ui_imgui_font_size(const TouchUi* ui, TouchText size)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    float imgui_scale = style.FontScaleMain * style.FontScaleDpi;
    return font_size(ui, size) / (imgui_scale > 0.0f ? imgui_scale : 1.0f);
}

float touch_ui_text(const TouchUi* ui, float x, float y, const char* text, uint32_t argb, TouchText size)
{
    ImGui::GetWindowDrawList()->AddText(ui->font, font_size(ui, size), ImVec2(px(ui, x), px(ui, y)), retro_ui_color_imgui(argb), text);
    return touch_ui_text_width(ui, text, size);
}

void touch_ui_rect(const TouchUi* ui, float x, float y, float width, float height, uint32_t argb)
{
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(px(ui, x), px(ui, y)), ImVec2(px(ui, x + width), px(ui, y + height)),
                                              retro_ui_color_imgui(argb));
}

float touch_ui_text_wrapped_height(const TouchUi* ui, float width, const char* text, TouchText size)
{
    ImVec2 measured = ui->font->CalcTextSizeA(font_size(ui, size), FLT_MAX, px(ui, width), text);
    return std::ceil(measured.y / static_cast<float>(ui->scale));
}

float touch_ui_text_wrapped(const TouchUi* ui, float x, float y, float width, const char* text, uint32_t argb,
                            TouchText size)
{
    ImGui::GetWindowDrawList()->AddText(ui->font, font_size(ui, size), ImVec2(px(ui, x), px(ui, y)),
                                        retro_ui_color_imgui(argb), text, nullptr, px(ui, width));
    return touch_ui_text_wrapped_height(ui, width, text, size);
}

void touch_ui_image_smooth(const TouchUi* ui, uint64_t texture, float x, float y, float width, float height,
                           float u_max, float v_max)
{
    // Bilinear for this image only, then back to the screen's nearest (the
    // pixel font's: touch_ui_begin_screen).
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
    draw_list->AddImage(ImTextureRef(static_cast<ImTextureID>(texture)), ImVec2(px(ui, x), px(ui, y)),
                        ImVec2(px(ui, x + width), px(ui, y + height)), ImVec2(0.0f, 0.0f), ImVec2(u_max, v_max));
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);
}

// Where text of `size` goes to look centered in a box `height` tall: the
// letters are rows 2..6 of the font's 8-row cell, so the cell sits half a
// font pixel higher than plain centering would put it.
float touch_ui_centered_text_y(const TouchUi* ui, float y, float height, TouchText size)
{
    float font_pixel = font_scale(ui, size) / static_cast<float>(ui->scale);
    float cell = touch_ui_text_height(ui, size);
    return std::floor(y + (height - cell) * 0.5f - 0.5f * font_pixel);
}

bool touch_ui_button(const TouchUi* ui, const char* id, float x, float y, float width, float height,
                     const char* label, TouchButton style, TouchText size)
{
    // ImGui does the hit testing and the press state; we draw everything.
    ImGui::SetCursorScreenPos(ImVec2(px(ui, x), px(ui, y)));
    // EnableNav: reachable with a gamepad (ImGui draws the focus around it).
    bool tapped = ImGui::InvisibleButton(id, ImVec2(px(ui, x + width) - px(ui, x), px(ui, y + height) - px(ui, y)),
                                         ImGuiButtonFlags_EnableNav);
    if (ui->focus != nullptr && ImGui::IsItemFocused())
    {
        ui->focus->has = true;
        ui->focus->y = y;
        ui->focus->height = height;
    }
    bool pressed = ImGui::IsItemActive();
    const RetroUiPalette* palette = &ui->palette;

    uint32_t face = palette->button_face;
    uint32_t text = palette->button_text;
    bool lip = true;
    if (style == TouchButton::primary)
    {
        face = palette->highlight;
        text = palette->text_highlight;
    }
    else if (style == TouchButton::card)
    {
        face = pressed ? palette->highlight : palette->panel_face;
        text = palette->text_highlight;
        lip = false;
    }
    else if (style == TouchButton::field)
    {
        face = pressed ? palette->highlight : palette->field_face;
        text = palette->field_text;
        lip = false;
    }
    else if (style == TouchButton::ghost || style == TouchButton::plain)
    {
        text = pressed && style == TouchButton::plain ? palette->highlight : palette->text;
        lip = false;
    }

    // The lip is the face's bottom BUTTON_LIP rows, darkened. Pressed, the
    // face moves down by the lip's height and the lip is gone: pushed in.
    float face_top = y;
    float face_height = height;
    if (lip)
    {
        face_height = height - BUTTON_LIP;
        face_top = pressed ? y + BUTTON_LIP : y;
    }

    if (style == TouchButton::ghost)
    {
        // An outline one retro pixel thick.
        uint32_t edge = pressed ? palette->highlight : palette->panel_light;
        touch_ui_rect(ui, x, y, width, 1.0f, edge);
        touch_ui_rect(ui, x, y + height - 1.0f, width, 1.0f, edge);
        touch_ui_rect(ui, x, y, 1.0f, height, edge);
        touch_ui_rect(ui, x + width - 1.0f, y, 1.0f, height, edge);
    }
    else if (style == TouchButton::plain)
    {
        // Nothing behind the label.
    }
    else if (lip && !pressed)
    {
        touch_ui_rect(ui, x, y, width, height, face);
        touch_ui_rect(ui, x, y + face_height, width, BUTTON_LIP, LIP_SHADE);
    }
    else
    {
        touch_ui_rect(ui, x, face_top, width, face_height, face);
    }
    if (style == TouchButton::normal)
    {
        // An edge one retro pixel thick (the mockups' buttons): in BLUE and
        // PURPLE a button's face is the screen's background, so without it
        // a button on the screen is only its label (Andrés, LOAD GAME).
        float edge_height = lip && !pressed ? height : face_height;
        touch_ui_outline(ui, x, face_top, width, edge_height, 1.0f, palette->panel_light);
    }

    if (label != nullptr && label[0] != '\0')
    {
        float text_width = touch_ui_text_width(ui, label, size);
        bool left = style == TouchButton::card || style == TouchButton::ghost;
        float text_x = left ? x + 10.0f
                                                                                 : std::floor(x + (width - text_width) * 0.5f);
        touch_ui_text(ui, text_x, touch_ui_centered_text_y(ui, face_top, face_height, size), label, text, size);
    }
    return tapped;
}

bool touch_ui_area(const TouchUi* ui, const char* id, float x, float y, float width, float height, bool* held,
                   TouchNav nav)
{
    ImGui::SetCursorScreenPos(ImVec2(px(ui, x), px(ui, y)));
    ImGuiButtonFlags flags = nav == TouchNav::reachable ? ImGuiButtonFlags_EnableNav : ImGuiButtonFlags_None;
    bool tapped = ImGui::InvisibleButton(id, ImVec2(px(ui, x + width) - px(ui, x), px(ui, y + height) - px(ui, y)), flags);
    *held = ImGui::IsItemActive();
    return tapped;
}

void touch_ui_lamp(const TouchUi* ui, float x, float y, uint32_t argb)
{
    touch_ui_rect(ui, x, y + 2.0f, TOUCH_UI_LAMP, TOUCH_UI_LAMP, argb);
}

void touch_ui_push_clip(const TouchUi* ui, float x, float y, float width, float height)
{
    ImGui::PushClipRect(ImVec2(px(ui, x), px(ui, y)), ImVec2(px(ui, x + width), px(ui, y + height)), true);
}

void touch_ui_pop_clip()
{
    ImGui::PopClipRect();
}

int32_t touch_ui_choices(const TouchUi* ui, const char* id, const char* const* names, int32_t count, int32_t current,
                         float x, float y, float width, float height)
{
    if (count <= 0)
    {
        return -1;
    }
    float each = (width - static_cast<float>(count - 1) * TOUCH_UI_GAP) / static_cast<float>(count);
    int32_t picked = -1;
    for (int32_t i = 0; i < count; i++)
    {
        char button_id[64];
        std::snprintf(button_id, sizeof(button_id), "%s%d", id, static_cast<int>(i));
        TouchButton style = i == current ? TouchButton::primary : TouchButton::normal;
        if (touch_ui_button(ui, button_id, x + static_cast<float>(i) * (each + TOUCH_UI_GAP), y, each, height, names[i],
                            style, TouchText::normal))
        {
            picked = i;
        }
    }
    return picked;
}

int32_t touch_ui_stepper(const TouchUi* ui, const char* id, const char* value, float x, float y, float width,
                         float height)
{
    const RetroUiPalette* palette = &ui->palette;
    float value_x = x + height + TOUCH_UI_GAP;
    float value_width = width - 2.0f * (height + TOUCH_UI_GAP);
    int32_t step = 0;
    char button_id[64];
    std::snprintf(button_id, sizeof(button_id), "%s_minus", id);
    if (touch_ui_button(ui, button_id, x, y, height, height, "-", TouchButton::normal, TouchText::big))
    {
        step = -1;
    }
    touch_ui_rect(ui, value_x, y, value_width, height, palette->field_face);
    float text_width = touch_ui_text_width(ui, value, TouchText::normal);
    touch_ui_text(ui, std::floor(value_x + (value_width - text_width) * 0.5f),
                  touch_ui_centered_text_y(ui, y, height, TouchText::normal), value, palette->field_text, TouchText::normal);
    std::snprintf(button_id, sizeof(button_id), "%s_plus", id);
    if (touch_ui_button(ui, button_id, x + width - height, y, height, height, "+", TouchButton::normal, TouchText::big))
    {
        step = 1;
    }
    return step;
}

void touch_ui_outline(const TouchUi* ui, float x, float y, float width, float height, float thickness, uint32_t argb)
{
    touch_ui_rect(ui, x, y, width, thickness, argb);
    touch_ui_rect(ui, x, y + height - thickness, width, thickness, argb);
    touch_ui_rect(ui, x, y, thickness, height, argb);
    touch_ui_rect(ui, x + width - thickness, y, thickness, height, argb);
}
