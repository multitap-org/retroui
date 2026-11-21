#pragma once

// Every touch pad layout, with its defaults (from piccoletto's mockups) and
// the user's changes (EDIT LAYOUT), one per mode and orientation. piccoletto
// uses them all; piccolo the ones over (or under) the game: the remote_ ones.

#include <cstdint>

#include "pad_layout.h"

enum class PadLayoutId
{
    gamepad_portrait,
    gamepad_landscape,
    remote_below,               // portrait, under the picture
    remote_overlay_portrait,    // portrait over a tall picture (the DS's bottom screen)
    remote_overlay_landscape,   // landscape, over the picture
    count,
};
constexpr int32_t PAD_LAYOUT_COUNT = static_cast<int32_t>(PadLayoutId::count);

struct PadLayouts
{
    PadLayout layouts[PAD_LAYOUT_COUNT];
};

// The defaults; then pad_layout_load() each one.
void pad_layouts_init(PadLayouts* layouts);

PadLayout* pad_layouts_get(PadLayouts* layouts, PadLayoutId id);
