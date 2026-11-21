#pragma once

// A RetroPad: what a touch pad (and a real gamepad) gives. Its buttons are
// libretro's RETRO_DEVICE_ID_JOYPAD_* numbers, its sticks libretro's analog
// axes (-32767..32767, y down). piccoletto sends it to the host as it is
// (Andrés: the phone does the mapping); piccolo hands it to its input router
// (input_devices_set_retropad).

#include <cstdint>

// RETRO_DEVICE_ID_JOYPAD_* (libretro.h), as bits.
enum class RetroPadButton : int32_t
{
    b = 0,
    y = 1,
    select = 2,
    start = 3,
    up = 4,
    down = 5,
    left = 6,
    right = 7,
    a = 8,
    x = 9,
    l = 10,
    r = 11,
    l2 = 12,
    r2 = 13,
    l3 = 14,
    r3 = 15,
};

struct RetroPad
{
    uint16_t buttons = 0;           // bit = RetroPadButton
    int16_t  analog[2][2] = {};     // [left / right][x / y]
};

void retropad_press(RetroPad* pad, RetroPadButton button);

// The RetroPad button a touch pad part is ("##a" -> a, "##l2" -> l2...);
// false for parts that aren't one (the d-pad's middle, the action button, the sticks).
bool retropad_button_for_part(const char* part_id, RetroPadButton* button);

// A stick's position (-1..1) as a RetroPad axis value.
int16_t retropad_axis(float value);

// Both pads at once: buttons held on either, the stick farther from rest.
void retropad_merge(RetroPad* into, const RetroPad* other);
