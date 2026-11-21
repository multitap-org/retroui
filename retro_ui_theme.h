#pragma once

// The palette as named values, for settings and theme files: the built-in
// themes' keys, and every color and switch of RetroUiPalette by index with
// its key. piccolo keeps each color as a setting (SETTINGS > INTERFACE) and
// reads / writes theme files (themes/*.cfg) with these keys; piccoletto only
// keeps a built-in theme's key. One table here instead of one per frontend,
// so a new palette field can't be forgotten in one of them.

#include <cstdint>

#include "retro_ui.h"

struct SettingsFile;

// A built-in theme's key in settings files: "blue", "purple", "zsnes", "black"
// ("" for RetroUiTheme::count). Names for people stay the frontend's
// (translated).
const char* retro_ui_theme_key(RetroUiTheme theme);
// The theme with that key; false (and *theme untouched) if none.
bool retro_ui_theme_from_key(const char* key, RetroUiTheme* theme);

// The palette's colors, in a fixed order, and their keys ("ui_color_background",
// "ui_color_bar", ...: piccolo's setting keys and the theme files' keys).
uint32_t        retro_ui_palette_color_count();
const char*     retro_ui_palette_color_key(uint32_t index);
uint32_t*       retro_ui_palette_color(RetroUiPalette* palette, uint32_t index);
uint32_t        retro_ui_palette_color_value(const RetroUiPalette* palette, uint32_t index);

// The palette's switches: "ui_menu_boxes", "ui_shadows", "ui_theme_cursor".
uint32_t        retro_ui_palette_flag_count();
const char*     retro_ui_palette_flag_key(uint32_t index);
bool*           retro_ui_palette_flag(RetroUiPalette* palette, uint32_t index);
bool            retro_ui_palette_flag_value(const RetroUiPalette* palette, uint32_t index);

// Same colors (alpha ignored) and switches.
bool retro_ui_palette_equal(const RetroUiPalette* a, const RetroUiPalette* b);

// A theme file's palette (its top section): colors as #RRGGBB, switches as
// true / false. Keys it doesn't have keep `palette`'s values; a bad value
// is logged (with `source`, e.g. the file's path) and skipped.
void retro_ui_palette_read(RetroUiPalette* palette, const SettingsFile* file, const char* source);
// The palette into a file's top section. False if a value couldn't be set.
bool retro_ui_palette_write(const RetroUiPalette* palette, SettingsFile* file);
