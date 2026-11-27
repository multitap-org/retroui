// The built-in themes: no element's face is the color of what it sits on
// (Andrés, 2026-11-26: in BLUE a button was the screen's color, only its
// label showed). Each pair below is an element and its surface.

#include <cstdint>
#include <cstdio>

#include "retro_ui.h"

static int g_failures = 0;

static void check_apart(const char* theme, uint32_t element, uint32_t surface, const char* what)
{
    if ((element & 0x00FFFFFFu) == (surface & 0x00FFFFFFu))
    {
        std::printf("FAILED: %s: %s are both %06X\n", theme, what, element & 0x00FFFFFFu);
        g_failures++;
    }
}

int main()
{
    const char* names[] = {"BLUE", "ATOMIC PURPLE", "ZSNES", "BLACK", "8-BIT GRAY", "8-BIT RED", "16-BIT GRAY A", "16-BIT GRAY B"};
    for (int32_t i = 0; i < static_cast<int32_t>(RetroUiTheme::count); i++)
    {
        const RetroUiPalette* p = retro_ui_theme_palette(static_cast<RetroUiTheme>(i));
        const char* name = names[i];
        check_apart(name, p->button_face, p->background, "a button and the screen");
        check_apart(name, p->button_face, p->panel_face, "a button and a window");
        check_apart(name, p->button_face, p->field_face, "a button and a field (a card)");
        check_apart(name, p->panel_face, p->background, "a window (a list row) and the screen");
        check_apart(name, p->field_face, p->panel_face, "a field and a window");
        check_apart(name, p->field_face, p->background, "a field (a card) and the screen");
        check_apart(name, p->menu_face, p->background, "a menu and the screen");
        check_apart(name, p->highlight, p->button_face, "the highlight and a button");
        check_apart(name, p->highlight, p->field_face, "the highlight and a field");
        check_apart(name, p->highlight, p->panel_face, "the highlight and a window");
    }
    if (g_failures == 0)
    {
        std::printf("theme contrast: all checks passed\n");
        return 0;
    }
    return 1;
}
