#include "port_indicators.h"

#include "imgui.h"

#include "retro_ui.h"

static void fill(float x, float y, float width, float height, uint32_t argb)
{
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), retro_ui_color_imgui(argb));
}

float port_indicators_draw_leds(float x, float top, float block, uint32_t count, int32_t lit_index, uint32_t off_argb)
{
    for (uint32_t i = 0; i < count; i++)
    {
        bool lit = lit_index >= 0 && i == static_cast<uint32_t>(lit_index);
        fill(x + static_cast<float>(i) * (PORT_LED_SIZE + PORT_LED_GAP) * block, top, PORT_LED_SIZE * block,
             PORT_LED_SIZE * block, lit ? PORT_LED_ON : off_argb);
    }
    return count == 0 ? 0.0f : (static_cast<float>(count) * (PORT_LED_SIZE + PORT_LED_GAP) - PORT_LED_GAP) * block;
}

float port_indicators_ping_width(float block)
{
    return (static_cast<float>(PING_BARS) * PING_BAR_WIDTH + static_cast<float>(PING_BARS - 1) * PING_BAR_GAP) * block;
}

void port_indicators_draw_ping(float x, float bottom, float block, bool known, uint32_t ping_ms, uint32_t off_argb)
{
    uint32_t lit = known ? ping_bars_lit(ping_ms) : 0;
    uint32_t color = known ? ping_color(ping_ms) : off_argb;
    for (uint32_t i = 0; i < PING_BARS; i++)
    {
        float height = ping_bar_height(i) * block;
        fill(x + static_cast<float>(i) * (PING_BAR_WIDTH + PING_BAR_GAP) * block, bottom - height, PING_BAR_WIDTH * block,
             height, i < lit ? color : off_argb);
    }
}
