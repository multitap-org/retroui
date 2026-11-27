#include "pad_layouts.h"

#include <cmath>

#include "pixel_font_imgui.h"

// Short names for the tables.
constexpr PadPartKind BUTTON = PadPartKind::button;
constexpr PadPartKind STICK = PadPartKind::stick;
constexpr PadPartKind ACTION = PadPartKind::action;   // its label comes from the frontend
constexpr PadPartKind MENU = PadPartKind::menu;       // the same (piccolo: the burger, between SELECT and START)
constexpr TouchText NORMAL = TouchText::normal;
constexpr TouchText BIG = TouchText::big;
constexpr PadGroup LSH = PadGroup::left_shoulders;
constexpr PadGroup RSH = PadGroup::right_shoulders;
constexpr PadGroup DPAD = PadGroup::dpad;
constexpr PadGroup FACE = PadGroup::face;
constexpr PadGroup CENTER = PadGroup::center;
constexpr PadGroup LSTICK = PadGroup::left_stick;
constexpr PadGroup RSTICK = PadGroup::right_stick;

// GAMEPAD, portrait (390 x 844 dp): shoulders on top, the body below with the
// d-pad and the face buttons, SELECT, the menu button and START (the menu
// between them, as in every layout: Andrés, 2026-11-27), both sticks at the
// bottom; the action button (HOTKEY) under them. (The
// header line is the screen's, not the layout's.)
static const PadPart GAMEPAD_PORTRAIT[] = {
    {"##l2", "L2", 16, 92, 160, 40, BUTTON, NORMAL, LSH},
    {"##l", "L1", 16, 140, 160, 44, BUTTON, NORMAL, LSH},
    {"##r2", "R2", 214, 92, 160, 40, BUTTON, NORMAL, RSH},
    {"##r", "R1", 214, 140, 160, 44, BUTTON, NORMAL, RSH},
    {"##dpad_center", "", 76, 328, 56, 56, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 76, 272, 56, 56, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 76, 384, 56, 56, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 20, 328, 56, 56, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 132, 328, 56, 56, BUTTON, NORMAL, DPAD},
    {"##x", "X", 258, 272, 56, 56, BUTTON, BIG, FACE},
    {"##b", "B", 258, 384, 56, 56, BUTTON, BIG, FACE},
    {"##y", "Y", 202, 328, 56, 56, BUTTON, BIG, FACE},
    {"##a", "A", 314, 328, 56, 56, BUTTON, BIG, FACE},
    {"##select", "SELECT", 79, 496, 88, 44, BUTTON, NORMAL, CENTER},
    {"##menu", "", 173, 496, 44, 44, MENU, BIG, CENTER},
    {"##start", "START", 223, 496, 88, 44, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 155, 556, 80, 40, ACTION, NORMAL, CENTER},
    {"##lstick", "", 48, 652, 112, 112, STICK, NORMAL, LSTICK},
    {"##rstick", "", 230, 652, 112, 112, STICK, NORMAL, RSTICK},
};

// GAMEPAD and REMOTE PLAY, landscape (844 x 390 dp): the same layout for
// both and for piccolo's touch pad (Andrés: one experience in both apps),
// on the pad's body (GAMEPAD, the body is the whole screen) or see-through
// over the picture (spread to any screen's shape by pad_layouts_place). The
// whole RetroPad: the shoulders stacked in the top corners (L2 over L1,
// Andrés 2026-11-21: less height), the d-pad on the left, the face buttons
// on the right (all four the same size), the sticks inside, SELECT, the menu
// button and START in a row at the bottom (Andrés, 2026-11-26: the menu is
// always between SELECT and START) and the action button after them (menu and
// action only when the frontend labels them; piccolo: the burger; piccoletto:
// HOTKEY).
static const PadPart LANDSCAPE_PAD[] = {
    {"##l2", "L2", 16, 12, 140, 30, BUTTON, NORMAL, LSH},
    {"##l", "L1", 16, 46, 140, 34, BUTTON, NORMAL, LSH},
    {"##r2", "R2", 688, 12, 140, 30, BUTTON, NORMAL, RSH},
    {"##r", "R1", 688, 46, 140, 34, BUTTON, NORMAL, RSH},
    {"##dpad_center", "", 84, 188, 52, 52, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 84, 136, 52, 52, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 84, 240, 52, 52, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 32, 188, 52, 52, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 136, 188, 52, 52, BUTTON, NORMAL, DPAD},
    {"##x", "X", 680, 124, 64, 64, BUTTON, BIG, FACE},
    {"##y", "Y", 612, 188, 64, 64, BUTTON, BIG, FACE},
    {"##a", "A", 748, 188, 64, 64, BUTTON, BIG, FACE},
    {"##b", "B", 680, 252, 64, 64, BUTTON, BIG, FACE},
    {"##lstick", "", 230, 256, 80, 80, STICK, NORMAL, LSTICK},
    {"##rstick", "", 534, 256, 80, 80, STICK, NORMAL, RSTICK},
    {"##select", "SELECT", 318, 350, 76, 36, BUTTON, NORMAL, CENTER},
    {"##menu", "", 402, 350, 40, 36, MENU, BIG, CENTER},
    {"##start", "START", 450, 350, 76, 36, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 534, 350, 124, 36, ACTION, NORMAL, CENTER},
};

// REMOTE PLAY, portrait, under the picture (390 x 420 dp: the controls of the
// mockup's lower half, shrunk to whatever room the picture leaves). The whole
// RetroPad, as sideways (Andrés, 2026-11-21: every element in every
// orientation for now): L2 / R2 and the sticks too. The face buttons a
// diamond of four the same size, level with the d-pad and under R1. The menu
// button between SELECT and START and the action button between the sticks,
// shown only when the frontend labels them: without a keyboard the menu is
// the way to piccolo's menus.
static const PadPart REMOTE_BELOW[] = {
    {"##l2", "L2", 16, 6, 120, 28, BUTTON, NORMAL, LSH},
    {"##l", "L1", 16, 38, 120, 32, BUTTON, NORMAL, LSH},
    {"##r2", "R2", 254, 6, 120, 28, BUTTON, NORMAL, RSH},
    {"##r", "R1", 254, 38, 120, 32, BUTTON, NORMAL, RSH},
    {"##dpad_center", "", 70, 140, 52, 52, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 70, 88, 52, 52, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 70, 192, 52, 52, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 18, 140, 52, 52, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 122, 140, 52, 52, BUTTON, NORMAL, DPAD},
    {"##x", "X", 266, 82, 56, 56, BUTTON, BIG, FACE},
    {"##y", "Y", 210, 138, 56, 56, BUTTON, BIG, FACE},
    {"##a", "A", 322, 138, 56, 56, BUTTON, BIG, FACE},
    {"##b", "B", 266, 194, 56, 56, BUTTON, BIG, FACE},
    {"##select", "SELECT", 95, 262, 76, 32, BUTTON, NORMAL, CENTER},
    {"##menu", "", 175, 262, 40, 32, MENU, BIG, CENTER},
    {"##start", "START", 219, 262, 76, 32, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 157, 344, 76, 32, ACTION, NORMAL, CENTER},
    {"##lstick", "", 60, 312, 96, 96, STICK, NORMAL, LSTICK},
    {"##rstick", "", 234, 312, 96, 96, STICK, NORMAL, RSTICK},
};

// REMOTE PLAY, portrait, over a tall picture (390 x 844 dp): the controls
// over its lower half (the DS's bottom screen), small and see-through; the
// shoulders just over the d-pad and the face buttons, in the thumbs' reach.
static const PadPart REMOTE_OVERLAY_PORTRAIT[] = {
    {"##l2", "L2", 12, 484, 96, 36, BUTTON, NORMAL, LSH},
    {"##l", "L1", 12, 526, 96, 36, BUTTON, NORMAL, LSH},
    {"##r2", "R2", 282, 484, 96, 36, BUTTON, NORMAL, RSH},
    {"##r", "R1", 282, 526, 96, 36, BUTTON, NORMAL, RSH},
    {"##dpad_center", "", 64, 620, 46, 46, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 64, 574, 46, 46, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 64, 666, 46, 46, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 18, 620, 46, 46, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 110, 620, 46, 46, BUTTON, NORMAL, DPAD},
    {"##x", "X", 280, 574, 46, 46, BUTTON, BIG, FACE},
    {"##b", "B", 280, 666, 46, 46, BUTTON, BIG, FACE},
    {"##y", "Y", 234, 620, 46, 46, BUTTON, BIG, FACE},
    {"##a", "A", 326, 620, 46, 46, BUTTON, BIG, FACE},
    {"##lstick", "", 45, 718, 84, 84, STICK, NORMAL, LSTICK},
    {"##rstick", "", 261, 718, 84, 84, STICK, NORMAL, RSTICK},
    {"##select", "SELECT", 95, 806, 76, 32, BUTTON, NORMAL, CENTER},
    {"##menu", "", 175, 806, 40, 32, MENU, BIG, CENTER},
    {"##start", "START", 219, 806, 76, 32, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 155, 744, 80, 32, ACTION, NORMAL, CENTER},
};

// MINIMAL (Andrés, 2026-11-27): only the controller's own buttons, where it
// has them, bigger (mockups: piccolo-agents docs/mockups/pad-layouts). The
// d-pad and a four-button face are the same 3 x 3 grid, mirrored; upright
// everything sits at the bottom, over the middle row, L and R level. The
// building blocks (groups) are the RetroPad's, each moved on its own in
// EDIT LAYOUT, and each arrangement keeps its own changes.

// NES / Game Boy (PadMinimal::two), sideways: B and A big, on a slant.
static const PadPart TWO_LANDSCAPE[] = {
    {"##dpad_center", "", 82, 186, 56, 56, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 82, 130, 56, 56, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 82, 242, 56, 56, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 26, 186, 56, 56, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 138, 186, 56, 56, BUTTON, NORMAL, DPAD},
    {"##b", "B", 654, 198, 72, 72, BUTTON, BIG, FACE},
    {"##a", "A", 742, 158, 72, 72, BUTTON, BIG, FACE},
    {"##select", "SELECT", 318, 350, 76, 36, BUTTON, NORMAL, CENTER},
    {"##menu", "", 402, 350, 40, 36, MENU, BIG, CENTER},
    {"##start", "START", 450, 350, 76, 36, BUTTON, NORMAL, CENTER},
};

// NES / Game Boy, upright under the picture (390 x 420).
static const PadPart TWO_BELOW[] = {
    {"##dpad_center", "", 68, 234, 56, 56, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 68, 178, 56, 56, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 68, 290, 56, 56, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 12, 234, 56, 56, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 124, 234, 56, 56, BUTTON, NORMAL, DPAD},
    {"##b", "B", 210, 258, 76, 76, BUTTON, BIG, FACE},
    {"##a", "A", 302, 214, 76, 76, BUTTON, BIG, FACE},
    {"##select", "SELECT", 95, 376, 76, 32, BUTTON, NORMAL, CENTER},
    {"##menu", "", 175, 376, 40, 32, MENU, BIG, CENTER},
    {"##start", "START", 219, 376, 76, 32, BUTTON, NORMAL, CENTER},
};

// Super Nintendo (PadMinimal::snes), sideways: L and R, the d-pad, X Y A B.
static const PadPart SNES_LANDSCAPE[] = {
    {"##l", "L", 16, 12, 140, 40, BUTTON, NORMAL, LSH},
    {"##r", "R", 688, 12, 140, 40, BUTTON, NORMAL, RSH},
    {"##dpad_center", "", 82, 186, 56, 56, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 82, 130, 56, 56, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 82, 242, 56, 56, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 26, 186, 56, 56, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 138, 186, 56, 56, BUTTON, NORMAL, DPAD},
    {"##x", "X", 706, 130, 56, 56, BUTTON, BIG, FACE},
    {"##y", "Y", 650, 186, 56, 56, BUTTON, BIG, FACE},
    {"##a", "A", 762, 186, 56, 56, BUTTON, BIG, FACE},
    {"##b", "B", 706, 242, 56, 56, BUTTON, BIG, FACE},
    {"##select", "SELECT", 318, 350, 76, 36, BUTTON, NORMAL, CENTER},
    {"##menu", "", 402, 350, 40, 36, MENU, BIG, CENTER},
    {"##start", "START", 450, 350, 76, 36, BUTTON, NORMAL, CENTER},
};

// Super Nintendo, upright under the picture (390 x 420).
static const PadPart SNES_BELOW[] = {
    {"##l", "L", 16, 112, 120, 36, BUTTON, NORMAL, LSH},
    {"##r", "R", 254, 112, 120, 36, BUTTON, NORMAL, RSH},
    {"##dpad_center", "", 68, 234, 56, 56, BUTTON, NORMAL, DPAD},
    {"##up", PIXEL_FONT_UTF8_ARROW_UP, 68, 178, 56, 56, BUTTON, NORMAL, DPAD},
    {"##down", PIXEL_FONT_UTF8_ARROW_DOWN, 68, 290, 56, 56, BUTTON, NORMAL, DPAD},
    {"##left", PIXEL_FONT_UTF8_ARROW_LEFT, 12, 234, 56, 56, BUTTON, NORMAL, DPAD},
    {"##right", PIXEL_FONT_UTF8_ARROW_RIGHT, 124, 234, 56, 56, BUTTON, NORMAL, DPAD},
    {"##x", "X", 266, 178, 56, 56, BUTTON, BIG, FACE},
    {"##y", "Y", 210, 234, 56, 56, BUTTON, BIG, FACE},
    {"##a", "A", 322, 234, 56, 56, BUTTON, BIG, FACE},
    {"##b", "B", 266, 290, 56, 56, BUTTON, BIG, FACE},
    {"##select", "SELECT", 95, 376, 76, 32, BUTTON, NORMAL, CENTER},
    {"##menu", "", 175, 376, 40, 32, MENU, BIG, CENTER},
    {"##start", "START", 219, 376, 76, 32, BUTTON, NORMAL, CENTER},
};

#define COUNT(table) static_cast<int32_t>(sizeof(table) / sizeof(table[0]))

static void define(PadLayouts* layouts, PadLayoutId id, const char* name, const PadPart* parts, int32_t count, float width,
                   float height, float alpha)
{
    PadLayout* layout = pad_layouts_get(layouts, id);
    layout->name = name;
    layout->parts = parts;
    layout->count = count;
    layout->width = width;
    layout->height = height;
    layout->default_alpha = alpha;
    layout->overlay = alpha < 1.0f;   // the see-through ones go over the picture
    pad_layout_reset(layout);
}

void pad_layouts_init(PadLayouts* layouts)
{
    define(layouts, PadLayoutId::gamepad_portrait, "gamepad_portrait", GAMEPAD_PORTRAIT,
           static_cast<int32_t>(sizeof(GAMEPAD_PORTRAIT) / sizeof(GAMEPAD_PORTRAIT[0])), 390.0f, 844.0f, 1.0f);
    define(layouts, PadLayoutId::gamepad_landscape, "gamepad_landscape", LANDSCAPE_PAD,
           static_cast<int32_t>(sizeof(LANDSCAPE_PAD) / sizeof(LANDSCAPE_PAD[0])), 844.0f, 390.0f, 1.0f);
    define(layouts, PadLayoutId::remote_below, "remote_below", REMOTE_BELOW,
           static_cast<int32_t>(sizeof(REMOTE_BELOW) / sizeof(REMOTE_BELOW[0])), 390.0f, 420.0f, 1.0f);
    define(layouts, PadLayoutId::remote_overlay_portrait, "remote_overlay_portrait", REMOTE_OVERLAY_PORTRAIT,
           static_cast<int32_t>(sizeof(REMOTE_OVERLAY_PORTRAIT) / sizeof(REMOTE_OVERLAY_PORTRAIT[0])), 390.0f, 844.0f, 0.2f);
    define(layouts, PadLayoutId::remote_overlay_landscape, "remote_overlay_landscape", LANDSCAPE_PAD,
           static_cast<int32_t>(sizeof(LANDSCAPE_PAD) / sizeof(LANDSCAPE_PAD[0])), 844.0f, 390.0f, 0.2f);
    define(layouts, PadLayoutId::two_below, "two_below", TWO_BELOW, COUNT(TWO_BELOW), 390.0f, 420.0f, 1.0f);
    define(layouts, PadLayoutId::two_overlay_landscape, "two_overlay_landscape", TWO_LANDSCAPE, COUNT(TWO_LANDSCAPE), 844.0f,
           390.0f, 0.2f);
    define(layouts, PadLayoutId::snes_below, "snes_below", SNES_BELOW, COUNT(SNES_BELOW), 390.0f, 420.0f, 1.0f);
    define(layouts, PadLayoutId::snes_overlay_landscape, "snes_overlay_landscape", SNES_LANDSCAPE, COUNT(SNES_LANDSCAPE),
           844.0f, 390.0f, 0.2f);
}

PadLayout* pad_layouts_get(PadLayouts* layouts, PadLayoutId id)
{
    return &layouts->layouts[static_cast<int32_t>(id)];
}

void pad_layouts_load_file(PadLayouts* layouts, const SettingsFile* file)
{
    for (int32_t i = 0; i < PAD_LAYOUT_COUNT; i++)
    {
        pad_layout_load(&layouts->layouts[i], file);
    }
}

void pad_layouts_save_file(const PadLayouts* layouts, SettingsFile* file)
{
    for (int32_t i = 0; i < PAD_LAYOUT_COUNT; i++)
    {
        pad_layout_save(&layouts->layouts[i], file);
    }
}

// An overlay over the whole `width` x `height` screen: the scale that fits
// its reference, and the room left over spread among its groups (extra_width /
// extra_height), so a screen of another shape is filled to its edges.
static PadFrame fill_screen(PadLayout* overlay, float width, float height)
{
    PadFrame frame = pad_frame_fit_into(overlay->width, overlay->height, 0.0f, 0.0f, width, height);
    overlay->extra_width = std::floor(width / frame.dp_scale - overlay->width);
    overlay->extra_height = std::floor(height / frame.dp_scale - overlay->height);
    overlay->extra_width = overlay->extra_width > 0.0f ? overlay->extra_width : 0.0f;
    overlay->extra_height = overlay->extra_height > 0.0f ? overlay->extra_height : 0.0f;
    frame.x = 0.0f;
    frame.y = 0.0f;
    return frame;
}

PadPlacement pad_layouts_place(PadLayouts* layouts, float width, float height, float aspect, float gap, PadMinimal minimal)
{
    PadPlacement placement;
    placement.picture_height = height;
    PadLayoutId below_id = minimal == PadMinimal::two    ? PadLayoutId::two_below
                           : minimal == PadMinimal::snes ? PadLayoutId::snes_below
                                                         : PadLayoutId::remote_below;
    if (width >= height)
    {
        placement.id = minimal == PadMinimal::two    ? PadLayoutId::two_overlay_landscape
                       : minimal == PadMinimal::snes ? PadLayoutId::snes_overlay_landscape
                                                     : PadLayoutId::remote_overlay_landscape;
        placement.frame = fill_screen(pad_layouts_get(layouts, placement.id), width, height);
        return placement;
    }
    // The picture across the top: does the pad still fit under it?
    float picture = std::floor(aspect > 0.0f ? width / aspect : width);
    picture = picture > height ? height : picture;
    float top = picture + gap;
    float room = height - top;
    const PadLayout* below = pad_layouts_get(layouts, below_id);
    PadFrame frame = pad_frame_fit_into(below->width, below->height, 0.0f, top, width, room > 0.0f ? room : 0.0f);
    if (frame.dp_scale >= PAD_MIN_BELOW_SCALE)
    {
        placement.id = below_id;
        placement.frame = frame;
        placement.below = true;
        placement.picture_height = picture;
        return placement;
    }
    placement.id = PadLayoutId::remote_overlay_portrait;
    placement.frame = fill_screen(pad_layouts_get(layouts, placement.id), width, height);
    return placement;
}
