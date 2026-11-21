#include "retropad.h"

#include <cstring>

struct PartButton
{
    const char*    id;
    RetroPadButton button;
};

static const PartButton PART_BUTTONS[] = {
    {"##a", RetroPadButton::a},         {"##b", RetroPadButton::b},         {"##x", RetroPadButton::x},
    {"##y", RetroPadButton::y},         {"##l", RetroPadButton::l},         {"##r", RetroPadButton::r},
    {"##l2", RetroPadButton::l2},       {"##r2", RetroPadButton::r2},       {"##up", RetroPadButton::up},
    {"##down", RetroPadButton::down},   {"##left", RetroPadButton::left},   {"##right", RetroPadButton::right},
    {"##select", RetroPadButton::select}, {"##start", RetroPadButton::start},
};

void retropad_press(RetroPad* pad, RetroPadButton button)
{
    pad->buttons = static_cast<uint16_t>(pad->buttons | (1u << static_cast<uint32_t>(button)));
}

bool retropad_button_for_part(const char* part_id, RetroPadButton* button)
{
    for (size_t i = 0; i < sizeof(PART_BUTTONS) / sizeof(PART_BUTTONS[0]); i++)
    {
        if (std::strcmp(part_id, PART_BUTTONS[i].id) == 0)
        {
            *button = PART_BUTTONS[i].button;
            return true;
        }
    }
    return false;
}

int16_t retropad_axis(float value)
{
    float clamped = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
    return static_cast<int16_t>(clamped * 32767.0f);
}

static int16_t stronger(int16_t a, int16_t b)
{
    int32_t magnitude_a = a < 0 ? -a : a;
    int32_t magnitude_b = b < 0 ? -b : b;
    return magnitude_b > magnitude_a ? b : a;
}

void retropad_merge(RetroPad* into, const RetroPad* other)
{
    into->buttons = static_cast<uint16_t>(into->buttons | other->buttons);
    for (int32_t stick = 0; stick < 2; stick++)
    {
        for (int32_t axis = 0; axis < 2; axis++)
        {
            into->analog[stick][axis] = stronger(into->analog[stick][axis], other->analog[stick][axis]);
        }
    }
}
