#include "retro_ui_theme.h"

#include <cstdio>
#include <cstring>

#include <SDL3/SDL_log.h>

#include "settings.h"
#include "settings_file.h"

static const char* const THEME_KEYS[] = {"blue", "purple", "zsnes", "black"};
constexpr uint32_t THEME_KEY_COUNT = static_cast<uint32_t>(sizeof(THEME_KEYS) / sizeof(THEME_KEYS[0]));
static_assert(THEME_KEY_COUNT == static_cast<uint32_t>(RetroUiTheme::count), "a key for every theme");

// The color keys, in palette_color()'s order (SETTINGS > INTERFACE lists
// them in this order too).
static const char* const COLOR_KEYS[] = {
    "ui_color_background",
    "ui_color_bar",
    "ui_color_bar_center",
    "ui_color_bar_light",
    "ui_color_bar_dark",
    "ui_color_menu_box",
    "ui_color_menu_box_light",
    "ui_color_menu_box_dark",
    "ui_color_menu",
    "ui_color_window",
    "ui_color_window_light",
    "ui_color_window_dark",
    "ui_color_field",
    "ui_color_field_hover",
    "ui_color_highlight",
    "ui_color_text",
    "ui_color_text_highlight",
    "ui_color_text_disabled",
    "ui_color_text_shadow",
    "ui_color_title",
    "ui_color_title_bottom",
    "ui_color_check",
    "ui_color_section",
    "ui_color_button",
    "ui_color_button_bottom",
    "ui_color_button_light",
    "ui_color_button_dark",
    "ui_color_button_text",
    "ui_color_field_text",
    "ui_color_cursor",
    "ui_color_cursor_edge",
};
constexpr uint32_t COLOR_COUNT = static_cast<uint32_t>(sizeof(COLOR_KEYS) / sizeof(COLOR_KEYS[0]));

static const char* const FLAG_KEYS[] = {"ui_menu_boxes", "ui_shadows", "ui_theme_cursor"};
constexpr uint32_t FLAG_COUNT = static_cast<uint32_t>(sizeof(FLAG_KEYS) / sizeof(FLAG_KEYS[0]));

// Every uint32_t in RetroUiPalette is a color and every bool a switch; each
// bool sits between colors, so it takes a uint32_t's room. A new field
// breaks this at compile time: it needs a key above and a case below.
static_assert(sizeof(RetroUiPalette) == (COLOR_COUNT + FLAG_COUNT) * sizeof(uint32_t),
              "RetroUiPalette changed: update retro_ui_theme.cpp");

const char* retro_ui_theme_key(RetroUiTheme theme)
{
    uint32_t index = static_cast<uint32_t>(theme);
    return index < THEME_KEY_COUNT ? THEME_KEYS[index] : "";
}

bool retro_ui_theme_from_key(const char* key, RetroUiTheme* theme)
{
    if (key == nullptr)
    {
        return false;
    }
    for (uint32_t i = 0; i < THEME_KEY_COUNT; i++)
    {
        if (std::strcmp(key, THEME_KEYS[i]) == 0)
        {
            *theme = static_cast<RetroUiTheme>(i);
            return true;
        }
    }
    return false;
}

uint32_t retro_ui_palette_color_count()
{
    return COLOR_COUNT;
}

const char* retro_ui_palette_color_key(uint32_t index)
{
    return index < COLOR_COUNT ? COLOR_KEYS[index] : COLOR_KEYS[0];
}

uint32_t* retro_ui_palette_color(RetroUiPalette* palette, uint32_t index)
{
    switch (index)
    {
    case 0:
        return &palette->background;
    case 1:
        return &palette->bar_face;
    case 2:
        return &palette->bar_center;
    case 3:
        return &palette->bar_light;
    case 4:
        return &palette->bar_dark;
    case 5:
        return &palette->menu_box_face;
    case 6:
        return &palette->menu_box_light;
    case 7:
        return &palette->menu_box_dark;
    case 8:
        return &palette->menu_face;
    case 9:
        return &palette->panel_face;
    case 10:
        return &palette->panel_light;
    case 11:
        return &palette->panel_dark;
    case 12:
        return &palette->field_face;
    case 13:
        return &palette->field_hover;
    case 14:
        return &palette->highlight;
    case 15:
        return &palette->text;
    case 16:
        return &palette->text_highlight;
    case 17:
        return &palette->text_disabled;
    case 18:
        return &palette->text_shadow;
    case 19:
        return &palette->title_top;
    case 20:
        return &palette->title_bottom;
    case 21:
        return &palette->check;
    case 22:
        return &palette->section;
    case 23:
        return &palette->button_face;
    case 24:
        return &palette->button_bottom;
    case 25:
        return &palette->button_light;
    case 26:
        return &palette->button_dark;
    case 27:
        return &palette->button_text;
    case 28:
        return &palette->field_text;
    case 29:
        return &palette->cursor;
    case 30:
        return &palette->cursor_edge;
    default:
        return &palette->text;
    }
}

uint32_t retro_ui_palette_color_value(const RetroUiPalette* palette, uint32_t index)
{
    RetroUiPalette copy = *palette;   // (retro_ui_palette_color hands out a pointer into a non-const palette)
    return *retro_ui_palette_color(&copy, index);
}

uint32_t retro_ui_palette_flag_count()
{
    return FLAG_COUNT;
}

const char* retro_ui_palette_flag_key(uint32_t index)
{
    return index < FLAG_COUNT ? FLAG_KEYS[index] : FLAG_KEYS[0];
}

bool* retro_ui_palette_flag(RetroUiPalette* palette, uint32_t index)
{
    switch (index)
    {
    case 0:
        return &palette->menu_boxes;
    case 1:
        return &palette->shadows;
    default:
        return &palette->theme_cursor;
    }
}

bool retro_ui_palette_flag_value(const RetroUiPalette* palette, uint32_t index)
{
    RetroUiPalette copy = *palette;
    return *retro_ui_palette_flag(&copy, index);
}

bool retro_ui_palette_equal(const RetroUiPalette* a, const RetroUiPalette* b)
{
    for (uint32_t i = 0; i < FLAG_COUNT; i++)
    {
        if (retro_ui_palette_flag_value(a, i) != retro_ui_palette_flag_value(b, i))
        {
            return false;
        }
    }
    for (uint32_t i = 0; i < COLOR_COUNT; i++)
    {
        if ((retro_ui_palette_color_value(a, i) | 0xFF000000u) != (retro_ui_palette_color_value(b, i) | 0xFF000000u))
        {
            return false;
        }
    }
    return true;
}

void retro_ui_palette_read(RetroUiPalette* palette, const SettingsFile* file, const char* source)
{
    for (uint32_t i = 0; i < FLAG_COUNT; i++)
    {
        const char* value = settings_file_get(file, "", FLAG_KEYS[i]);
        if (value == nullptr)
        {
            continue;
        }
        if (std::strcmp(value, "true") == 0 || std::strcmp(value, "false") == 0)
        {
            *retro_ui_palette_flag(palette, i) = std::strcmp(value, "true") == 0;
        }
        else
        {
            SDL_Log("Theme %s: %s = %s is not true or false", source, FLAG_KEYS[i], value);
        }
    }
    for (uint32_t i = 0; i < COLOR_COUNT; i++)
    {
        const char* value = settings_file_get(file, "", COLOR_KEYS[i]);
        uint32_t color = 0;
        if (value == nullptr)
        {
            continue;
        }
        if (!settings_color_from_text(value, &color))
        {
            SDL_Log("Theme %s: %s = %s is not a #RRGGBB color", source, COLOR_KEYS[i], value);
            continue;
        }
        *retro_ui_palette_color(palette, i) = color | 0xFF000000u;
    }
}

bool retro_ui_palette_write(const RetroUiPalette* palette, SettingsFile* file)
{
    bool ok = true;
    for (uint32_t i = 0; i < FLAG_COUNT; i++)
    {
        ok = settings_file_set(file, "", FLAG_KEYS[i], retro_ui_palette_flag_value(palette, i) ? "true" : "false") && ok;
    }
    for (uint32_t i = 0; i < COLOR_COUNT; i++)
    {
        char color[16] = {};
        std::snprintf(color, sizeof(color), "#%06X", static_cast<unsigned>(retro_ui_palette_color_value(palette, i) & 0xFFFFFFu));
        ok = settings_file_set(file, "", COLOR_KEYS[i], color) && ok;
    }
    return ok;
}
