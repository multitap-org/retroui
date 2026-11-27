#pragma once

// Every touch pad layout, with its defaults (from piccoletto's mockups) and
// the user's changes (EDIT LAYOUT), one per mode and orientation. piccoletto
// uses them all; piccolo the ones over (or under) the game: the remote_ ones.

#include <cstdint>

#include "pad_layout.h"

struct SettingsFile;

enum class PadLayoutId
{
    gamepad_portrait,
    gamepad_landscape,
    remote_below,               // portrait, under the picture
    remote_overlay_portrait,    // portrait over a tall picture (the DS's bottom screen)
    remote_overlay_landscape,   // landscape, over the picture
    // MINIMAL (PadMinimal): only a controller's own buttons, bigger; piccolo's.
    two_below,                  // NES / Game Boy, portrait under the picture
    two_overlay_landscape,      //   ...landscape over it
    snes_below,                 // Super Nintendo, portrait under the picture
    snes_overlay_landscape,     //   ...landscape over it
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

// Every layout's user changes from / to the frontend's layouts file (each in
// its own section; the file's name and saving it stay the frontend's).
void pad_layouts_load_file(PadLayouts* layouts, const SettingsFile* file);
void pad_layouts_save_file(const PadLayouts* layouts, SettingsFile* file);

// Where the pad goes around a picture (REMOTE PLAY's stream, piccolo's game):
//   landscape: remote_overlay_landscape over the picture, at the bottom;
//   portrait:  remote_below under the picture (which keeps the top, as wide
//              as the screen) if the pad stays big enough for fingers there,
//              else remote_overlay_portrait over a tall picture (the DS).
struct PadPlacement
{
    PadLayoutId id             = PadLayoutId::remote_overlay_landscape;
    PadFrame    frame          = {};
    bool        below          = false;   // under the picture
    float       picture_height = 0.0f;    // retro pixels the picture keeps at the top (below; else the screen's)
};

// Under this the pad below the picture would be too small for fingers
// (retro pixels per dp; the mockups' size is 0.5).
constexpr float PAD_MIN_BELOW_SCALE = 0.3f;

// The touch pad's arrangement: the whole RetroPad (FULL, the remote_ layouts)
// or MINIMAL, only one controller's buttons where it has them (the two_ and
// snes_ layouts). A tall picture (the DS) keeps remote_overlay_portrait.
enum class PadMinimal
{
    none,
    two,    // NES / Game Boy: the d-pad, B and A on a slant, SELECT and START
    snes,   // Super Nintendo: L, R, the d-pad, X Y A B as a cross, SELECT and START
};

// `width` x `height`: the screen in retro pixels; `aspect` the picture's
// (width / height; 0 = unknown, square); `gap` retro pixels between the
// picture and a pad below it (piccoletto puts its status line there).
PadPlacement pad_layouts_place(PadLayouts* layouts, float width, float height, float aspect, float gap,
                               PadMinimal minimal = PadMinimal::none);
