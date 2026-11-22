#include "pad_layouts.h"

#include <cmath>

#include "pixel_font_imgui.h"

// Short names for the tables.
constexpr PadPartKind BUTTON = PadPartKind::button;
constexpr PadPartKind STICK = PadPartKind::stick;
constexpr PadPartKind ACTION = PadPartKind::action;   // its label comes from the frontend
constexpr PadPartKind MENU = PadPartKind::menu;       // the same (piccolo: MAIN MENU)
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
// d-pad and the face buttons, SELECT / START / the piccolo menu, both sticks
// at the bottom; the action button (piccoletto: HOTKEY) under SELECT / START. (The
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
    {"##select", "SELECT", 100, 496, 88, 44, BUTTON, NORMAL, CENTER},
    {"##start", "START", 202, 496, 88, 44, BUTTON, NORMAL, CENTER},
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
// on the right (all four the same size), the sticks inside, the menu button /
// SELECT / the action button / START in a row at the bottom (menu and action
// only when the frontend labels them; piccolo: MENU, QUICK; piccoletto:
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
    {"##menu", "MENU", 164, 350, 96, 36, MENU, NORMAL, CENTER},
    {"##select", "SELECT", 268, 350, 84, 36, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 360, 350, 124, 36, ACTION, NORMAL, CENTER},
    {"##start", "START", 492, 350, 84, 36, BUTTON, NORMAL, CENTER},
};

// REMOTE PLAY, portrait, under the picture (390 x 420 dp: the controls of the
// mockup's lower half, shrunk to whatever room the picture leaves). The whole
// RetroPad, as sideways (Andrés, 2026-11-21: every element in every
// orientation for now): L2 / R2 and the sticks too. The menu
// and action buttons (piccolo: MENU, QUICK) beside SELECT / START, shown
// only when the frontend labels them: without a keyboard they're the way to
// piccolo's menus.
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
    {"##y", "Y", 214, 82, 52, 52, BUTTON, BIG, FACE},
    {"##x", "X", 314, 62, 52, 52, BUTTON, BIG, FACE},
    {"##b", "B", 214, 166, 68, 68, BUTTON, BIG, FACE},
    {"##a", "A", 298, 118, 68, 68, BUTTON, BIG, FACE},
    {"##menu", "MENU", 8, 262, 88, 32, MENU, NORMAL, CENTER},
    {"##select", "SELECT", 104, 262, 80, 32, BUTTON, NORMAL, CENTER},
    {"##start", "START", 196, 262, 80, 32, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 294, 262, 88, 32, ACTION, NORMAL, CENTER},
    {"##lstick", "", 60, 312, 96, 96, STICK, NORMAL, LSTICK},
    {"##rstick", "", 234, 312, 96, 96, STICK, NORMAL, RSTICK},
};

// REMOTE PLAY, portrait, over a tall picture (390 x 844 dp): the controls
// over its lower half (the DS's bottom screen), small and see-through.
static const PadPart REMOTE_OVERLAY_PORTRAIT[] = {
    {"##l2", "L2", 12, 410, 96, 36, BUTTON, NORMAL, LSH},
    {"##l", "L1", 12, 452, 96, 36, BUTTON, NORMAL, LSH},
    {"##r2", "R2", 282, 410, 96, 36, BUTTON, NORMAL, RSH},
    {"##r", "R1", 282, 452, 96, 36, BUTTON, NORMAL, RSH},
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
    {"##menu", "MENU", 12, 806, 88, 32, MENU, NORMAL, CENTER},
    {"##select", "SELECT", 110, 806, 80, 32, BUTTON, NORMAL, CENTER},
    {"##start", "START", 200, 806, 80, 32, BUTTON, NORMAL, CENTER},
    {"##hotkey", "HOTKEY", 290, 806, 88, 32, ACTION, NORMAL, CENTER},
};

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

PadPlacement pad_layouts_place(PadLayouts* layouts, float width, float height, float aspect, float gap)
{
    PadPlacement placement;
    placement.picture_height = height;
    if (width >= height)
    {
        placement.id = PadLayoutId::remote_overlay_landscape;
        placement.frame = fill_screen(pad_layouts_get(layouts, placement.id), width, height);
        return placement;
    }
    // The picture across the top: does the pad still fit under it?
    float picture = std::floor(aspect > 0.0f ? width / aspect : width);
    picture = picture > height ? height : picture;
    float top = picture + gap;
    float room = height - top;
    const PadLayout* below = pad_layouts_get(layouts, PadLayoutId::remote_below);
    PadFrame frame = pad_frame_fit_into(below->width, below->height, 0.0f, top, width, room > 0.0f ? room : 0.0f);
    if (frame.dp_scale >= PAD_MIN_BELOW_SCALE)
    {
        placement.id = PadLayoutId::remote_below;
        placement.frame = frame;
        placement.below = true;
        placement.picture_height = picture;
        return placement;
    }
    placement.id = PadLayoutId::remote_overlay_portrait;
    placement.frame = fill_screen(pad_layouts_get(layouts, placement.id), width, height);
    return placement;
}
