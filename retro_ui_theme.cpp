#include "retro_ui_theme.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <SDL3/SDL_log.h>

#include "settings.h"
#include "settings_file.h"

static const char* const THEME_KEYS[] = {"blue", "purple", "zsnes", "black", "gray8", "red8", "gray16a", "gray16b"};
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
    "ui_color_pad_background",
    "ui_color_pad_dpad",
    "ui_color_pad_dpad_held",
    "ui_color_pad_dpad_text",
    "ui_color_pad_a",
    "ui_color_pad_a_held",
    "ui_color_pad_a_text",
    "ui_color_pad_b",
    "ui_color_pad_b_held",
    "ui_color_pad_b_text",
    "ui_color_pad_x",
    "ui_color_pad_x_held",
    "ui_color_pad_x_text",
    "ui_color_pad_y",
    "ui_color_pad_y_held",
    "ui_color_pad_y_text",
    "ui_color_pad_shoulder",
    "ui_color_pad_shoulder_held",
    "ui_color_pad_shoulder_text",
    "ui_color_pad_stick",
    "ui_color_pad_stick_held",
    "ui_color_pad_stick_nub",
    "ui_color_pad_center",
    "ui_color_pad_center_held",
    "ui_color_pad_center_text",
    "ui_color_pad_border",
};
constexpr uint32_t COLOR_COUNT = static_cast<uint32_t>(sizeof(COLOR_KEYS) / sizeof(COLOR_KEYS[0]));

static const char* const FLAG_KEYS[] = {"ui_menu_boxes", "ui_shadows", "ui_theme_cursor"};
constexpr uint32_t FLAG_COUNT = static_cast<uint32_t>(sizeof(FLAG_KEYS) / sizeof(FLAG_KEYS[0]));

static const char* const LEVEL_KEYS[] = {"ui_background_transparency", "ui_pad_fill", "ui_pad_border", "ui_pad_labels", "ui_pad_background"};
constexpr uint32_t LEVEL_COUNT = static_cast<uint32_t>(sizeof(LEVEL_KEYS) / sizeof(LEVEL_KEYS[0]));
constexpr uint32_t LEVEL_MAX = 100;

// Every uint32_t in RetroUiPalette is a color or a level and every bool a
// switch; each bool sits between colors, so it takes a uint32_t's room. A
// new field breaks this at compile time: it needs a key above and a case below.
static_assert(sizeof(RetroUiPalette) == (COLOR_COUNT + FLAG_COUNT + LEVEL_COUNT) * sizeof(uint32_t),
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
    case 31:
        return &palette->pad_background;
    case 32:
        return &palette->pad_dpad;
    case 33:
        return &palette->pad_dpad_held;
    case 34:
        return &palette->pad_dpad_text;
    case 35:
        return &palette->pad_a;
    case 36:
        return &palette->pad_a_held;
    case 37:
        return &palette->pad_a_text;
    case 38:
        return &palette->pad_b;
    case 39:
        return &palette->pad_b_held;
    case 40:
        return &palette->pad_b_text;
    case 41:
        return &palette->pad_x;
    case 42:
        return &palette->pad_x_held;
    case 43:
        return &palette->pad_x_text;
    case 44:
        return &palette->pad_y;
    case 45:
        return &palette->pad_y_held;
    case 46:
        return &palette->pad_y_text;
    case 47:
        return &palette->pad_shoulder;
    case 48:
        return &palette->pad_shoulder_held;
    case 49:
        return &palette->pad_shoulder_text;
    case 50:
        return &palette->pad_stick;
    case 51:
        return &palette->pad_stick_held;
    case 52:
        return &palette->pad_stick_nub;
    case 53:
        return &palette->pad_center;
    case 54:
        return &palette->pad_center_held;
    case 55:
        return &palette->pad_center_text;
    case 56:
        return &palette->pad_border;
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

uint32_t retro_ui_palette_level_count()
{
    return LEVEL_COUNT;
}

const char* retro_ui_palette_level_key(uint32_t index)
{
    return index < LEVEL_COUNT ? LEVEL_KEYS[index] : LEVEL_KEYS[0];
}

uint32_t* retro_ui_palette_level(RetroUiPalette* palette, uint32_t index)
{
    switch (index)
    {
    case 0:
        return &palette->background_transparency;
    case 1:
        return &palette->pad_fill;
    case 2:
        return &palette->pad_border_opacity;
    case 3:
        return &palette->pad_labels;
    default:
        return &palette->pad_background_opacity;
    }
}

uint32_t retro_ui_palette_level_value(const RetroUiPalette* palette, uint32_t index)
{
    RetroUiPalette copy = *palette;
    return *retro_ui_palette_level(&copy, index);
}

bool retro_ui_palette_equal(const RetroUiPalette* a, const RetroUiPalette* b)
{
    for (uint32_t i = 0; i < LEVEL_COUNT; i++)
    {
        if (retro_ui_palette_level_value(a, i) != retro_ui_palette_level_value(b, i))
        {
            return false;
        }
    }
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
    for (uint32_t i = 0; i < LEVEL_COUNT; i++)
    {
        const char* value = settings_file_get(file, "", LEVEL_KEYS[i]);
        if (value == nullptr)
        {
            continue;
        }
        char* end = nullptr;
        long number = std::strtol(value, &end, 10);
        if (end == value || *end != '\0' || number < 0 || number > static_cast<long>(LEVEL_MAX))
        {
            SDL_Log("Theme %s: %s = %s is not a number from 0 to %u", source, LEVEL_KEYS[i], value, LEVEL_MAX);
            continue;
        }
        *retro_ui_palette_level(palette, i) = static_cast<uint32_t>(number);
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
    for (uint32_t i = 0; i < LEVEL_COUNT; i++)
    {
        char level[16] = {};
        std::snprintf(level, sizeof(level), "%u", static_cast<unsigned>(retro_ui_palette_level_value(palette, i)));
        ok = settings_file_set(file, "", LEVEL_KEYS[i], level) && ok;
    }
    return ok;
}
