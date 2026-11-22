// The touch pad's layouts and the d-pad: every layout's parts apart from each
// other and on its screen, and the d-pad's eight directions.

#include <cstdio>

#include "pad_layouts.h"
#include "retropad.h"

static int g_failures = 0;

static void check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        g_failures++;
    }
}

static uint32_t bit(RetroPadButton button)
{
    return 1u << static_cast<uint32_t>(button);
}

// Two parts overlap (touching edges is fine: the d-pad's arrows do).
static bool overlap(const PadPart* a, const PadPart* b)
{
    return a->x < b->x + b->width && b->x < a->x + a->width && a->y < b->y + b->height && b->y < a->y + a->height;
}

static void check_layout(const PadLayout* layout)
{
    char what[160];
    for (int32_t i = 0; i < layout->count; i++)
    {
        const PadPart* part = &layout->parts[i];
        std::snprintf(what, sizeof(what), "%s: %s inside the layout", layout->name, part->id);
        check(part->x >= 0.0f && part->y >= 0.0f && part->x + part->width <= layout->width &&
                  part->y + part->height <= layout->height,
              what);
        for (int32_t j = i + 1; j < layout->count; j++)
        {
            std::snprintf(what, sizeof(what), "%s: %s and %s apart", layout->name, part->id, layout->parts[j].id);
            check(!overlap(part, &layout->parts[j]), what);
        }
    }
}

int main()
{
    PadLayouts* layouts = new PadLayouts{};
    pad_layouts_init(layouts);
    for (int32_t i = 0; i < PAD_LAYOUT_COUNT; i++)
    {
        check_layout(&layouts->layouts[i]);
    }
    delete layouts;

    // The d-pad: (dx, dy) in halves of its size, y down.
    uint32_t up = bit(RetroPadButton::up);
    uint32_t down = bit(RetroPadButton::down);
    uint32_t left = bit(RetroPadButton::left);
    uint32_t right = bit(RetroPadButton::right);
    check(pad_dpad_directions(0.0f, 0.0f) == 0, "d-pad: the middle presses nothing");
    check(pad_dpad_directions(0.1f, 0.05f) == 0, "d-pad: a resting thumb near the middle neither");
    check(pad_dpad_directions(0.0f, -0.8f) == up, "d-pad: up");
    check(pad_dpad_directions(0.0f, 0.8f) == down, "d-pad: down");
    check(pad_dpad_directions(-0.8f, 0.0f) == left, "d-pad: left");
    check(pad_dpad_directions(0.8f, 0.0f) == right, "d-pad: right");
    check(pad_dpad_directions(0.7f, -0.7f) == (up | right), "d-pad: the top right corner, up and right");
    check(pad_dpad_directions(-0.7f, 0.7f) == (down | left), "d-pad: the bottom left corner, down and left");
    check(pad_dpad_directions(0.8f, -0.25f) == right, "d-pad: a little above right (under 22.5 degrees), right only");
    check(pad_dpad_directions(0.8f, -0.45f) == (up | right), "d-pad: past 22.5 degrees, the diagonal");
    check(pad_dpad_directions(0.3f, 0.9f) == down, "d-pad: a little right of down, down only");

    // A and B (the landscape pad's middles, 1.46 widths apart on a slant):
    // between them both, on either one only that one.
    float a_x = 780.0f;
    float a_y = 220.0f;
    float b_x = 712.0f;
    float b_y = 284.0f;
    check(pad_pair_zone(746.0f, 252.0f, a_x, a_y, b_x, b_y), "A+B: the midpoint presses both");
    check(!pad_pair_zone(a_x, a_y, a_x, a_y, b_x, b_y), "A+B: A's middle presses A only");
    check(!pad_pair_zone(b_x, b_y, a_x, a_y, b_x, b_y), "A+B: B's middle presses B only");
    check(pad_pair_zone(766.0f, 273.0f, a_x, a_y, b_x, b_y), "A+B: a little off the line between them, still both");
    check(!pad_pair_zone(800.0f, 300.0f, a_x, a_y, b_x, b_y), "A+B: well off to the side, neither");

    if (g_failures == 0)
    {
        std::printf("pad layouts OK\n");
        return 0;
    }
    return 1;
}
