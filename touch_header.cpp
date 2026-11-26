#include "touch_header.h"

#include "pixel_font_imgui.h"   // PIXEL_FONT_UTF8_ARROW_LEFT

float touch_header_y(const TouchUi* ui)
{
    bool landscape = touch_ui_width(ui) > touch_ui_height(ui);
    return landscape ? 6.0f : TOUCH_HEADER_MARGIN;
}

float touch_header_bottom(const TouchUi* ui)
{
    return touch_header_y(ui) + TOUCH_HEADER_ICON;
}

float touch_header_icons_x(const TouchUi* ui, int32_t icon_count)
{
    return touch_ui_width(ui) - TOUCH_HEADER_MARGIN - static_cast<float>(icon_count) * TOUCH_HEADER_ICON;
}

void touch_header_title(const TouchUi* ui, const char* title)
{
    float y = touch_ui_centered_text_y(ui, touch_header_y(ui), TOUCH_HEADER_ICON, TouchText::big);
    touch_ui_text(ui, TOUCH_HEADER_MARGIN + 4.0f, y, title, ui->palette.text_highlight, TouchText::big);
}

bool touch_header_icon(const TouchUi* ui, const char* id, int32_t slot, int32_t icon_count, const char* glyph)
{
    float x = touch_header_icons_x(ui, icon_count) + static_cast<float>(slot) * TOUCH_HEADER_ICON;
    return touch_ui_button(ui, id, x, touch_header_y(ui), TOUCH_HEADER_ICON, TOUCH_HEADER_ICON, glyph, TouchButton::plain,
                           TouchText::big);
}

bool touch_header_back(const TouchUi* ui)
{
    return touch_ui_button(ui, "##header_back", TOUCH_HEADER_MARGIN - 2.0f, touch_header_y(ui), TOUCH_HEADER_ICON,
                           TOUCH_HEADER_ICON, PIXEL_FONT_UTF8_ARROW_LEFT, TouchButton::plain, TouchText::big);
}
