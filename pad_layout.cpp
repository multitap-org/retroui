#include "pad_layout.h"

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "retropad.h"
#include "settings_file.h"
#include "touch_fingers.h"

// The yellow of a lit part: the same in every theme, as in piccolo's pad picture.
constexpr uint32_t PART_LIT      = 0xFFFFDC50u;
constexpr uint32_t PART_LIT_TEXT = 0xFF000000u;

// The mockups put 2 dp in a retro pixel.
constexpr float DP_PER_RETRO_PIXEL = 2.0f;

static const char* const GROUP_LABELS[PAD_GROUP_COUNT] = {"L SHOULDERS", "R SHOULDERS", "D-PAD", "BUTTONS",
                                                          "SELECT / START", "LEFT STICK", "RIGHT STICK"};
static const char* const GROUP_KEYS[PAD_GROUP_COUNT] = {"left_shoulders", "right_shoulders", "dpad", "face",
                                                        "center", "left_stick", "right_stick"};

const char* pad_group_label(PadGroup group)
{
    return GROUP_LABELS[static_cast<int32_t>(group)];
}

const char* pad_group_key(PadGroup group)
{
    return GROUP_KEYS[static_cast<int32_t>(group)];
}

void pad_layout_reset(PadLayout* layout)
{
    for (int32_t g = 0; g < PAD_GROUP_COUNT; g++)
    {
        layout->groups[g] = PadGroupEdit{};
    }
    layout->alpha = layout->default_alpha;
    layout->alpha_set = false;
}

// A number from the layouts file, kept in [minimum, maximum]; `fallback`
// when it's missing or not a number.
static float read_float(const SettingsFile* file, const char* section, const char* key, float fallback, float minimum,
                        float maximum)
{
    const char* text = settings_file_get(file, section, key);
    if (text == nullptr)
    {
        return fallback;
    }
    char* end = nullptr;
    float value = std::strtof(text, &end);
    if (end == text || !std::isfinite(value))
    {
        return fallback;
    }
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

void pad_layout_load(PadLayout* layout, const SettingsFile* file)
{
    pad_layout_reset(layout);
    char key[64];
    for (int32_t g = 0; g < PAD_GROUP_COUNT; g++)
    {
        PadGroupEdit* edit = &layout->groups[g];
        std::snprintf(key, sizeof(key), "%s.x", GROUP_KEYS[g]);
        edit->dx = read_float(file, layout->name, key, 0.0f, -layout->width, layout->width);
        std::snprintf(key, sizeof(key), "%s.y", GROUP_KEYS[g]);
        edit->dy = read_float(file, layout->name, key, 0.0f, -layout->height, layout->height);
        std::snprintf(key, sizeof(key), "%s.size", GROUP_KEYS[g]);
        edit->scale = read_float(file, layout->name, key, 1.0f, PAD_SCALE_MIN, PAD_SCALE_MAX);
    }
    // An empty alpha = not set here (the default, which SETTINGS may change).
    const char* alpha = settings_file_get(file, layout->name, "alpha");
    layout->alpha_set = alpha != nullptr && alpha[0] != '\0';
    layout->alpha = read_float(file, layout->name, "alpha", layout->default_alpha, PAD_ALPHA_MIN, 1.0f);
}

void pad_layout_save(const PadLayout* layout, SettingsFile* file)
{
    char key[64];
    char value[32];
    for (int32_t g = 0; g < PAD_GROUP_COUNT; g++)
    {
        const PadGroupEdit* edit = &layout->groups[g];
        std::snprintf(key, sizeof(key), "%s.x", GROUP_KEYS[g]);
        std::snprintf(value, sizeof(value), "%.1f", static_cast<double>(edit->dx));
        settings_file_set(file, layout->name, key, value);
        std::snprintf(key, sizeof(key), "%s.y", GROUP_KEYS[g]);
        std::snprintf(value, sizeof(value), "%.1f", static_cast<double>(edit->dy));
        settings_file_set(file, layout->name, key, value);
        std::snprintf(key, sizeof(key), "%s.size", GROUP_KEYS[g]);
        std::snprintf(value, sizeof(value), "%.2f", static_cast<double>(edit->scale));
        settings_file_set(file, layout->name, key, value);
    }
    std::snprintf(value, sizeof(value), "%.2f", static_cast<double>(layout->alpha));
    settings_file_set(file, layout->name, "alpha", layout->alpha_set ? value : "");
}

void pad_layout_set_default_alpha(PadLayout* layout, float alpha)
{
    layout->default_alpha = alpha;
    if (!layout->alpha_set)
    {
        layout->alpha = alpha;
    }
}

bool pad_layout_has_group(const PadLayout* layout, PadGroup group)
{
    for (int32_t i = 0; i < layout->count; i++)
    {
        if (layout->parts[i].group == group)
        {
            return true;
        }
    }
    return false;
}

// A group's default bounding box (before the user's change), in dp.
static void default_group_box(const PadLayout* layout, PadGroup group, float* x0, float* y0, float* x1, float* y1)
{
    *x0 = FLT_MAX;
    *y0 = FLT_MAX;
    *x1 = -FLT_MAX;
    *y1 = -FLT_MAX;
    for (int32_t i = 0; i < layout->count; i++)
    {
        const PadPart* part = &layout->parts[i];
        if (part->group != group)
        {
            continue;
        }
        *x0 = part->x < *x0 ? part->x : *x0;
        *y0 = part->y < *y0 ? part->y : *y0;
        *x1 = part->x + part->width > *x1 ? part->x + part->width : *x1;
        *y1 = part->y + part->height > *y1 ? part->y + part->height : *y1;
    }
}

// How much of the screen's extra room a group at `middle` (of `size`) takes:
// 0 in the first third, 0.5 in the middle one, 1 in the last.
static float edge_share(float middle, float size)
{
    if (size <= 0.0f || middle < size / 3.0f)
    {
        return 0.0f;
    }
    return middle > size * 2.0f / 3.0f ? 1.0f : 0.5f;
}

// With a six-button style: the cell (0..5) of a part in the 3 x 2 face, or
// -1. Below A B C on the RetroPad's Y B A, above X Y Z on L X R.
static int32_t six_face_cell(const PadLayout* layout, const PadPart* part)
{
    if (layout->style == nullptr || !layout->style->six_face)
    {
        return -1;
    }
    RetroPadButton button = RetroPadButton::b;
    if (!retropad_button_for_part(part->id, &button))
    {
        return -1;
    }
    switch (button)
    {
        case RetroPadButton::y: return 0;
        case RetroPadButton::b: return 1;
        case RetroPadButton::a: return 2;
        case RetroPadButton::l: return 3;
        case RetroPadButton::x: return 4;
        case RetroPadButton::r: return 5;
        default:                return -1;
    }
}

void pad_layout_part_rect(const PadLayout* layout, int32_t part, float* x, float* y, float* width, float* height)
{
    PadPart cell_part = layout->parts[part];
    const PadPart* p = &cell_part;
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    int32_t cell = six_face_cell(layout, p);
    if (cell >= 0)
    {
        // Six buttons in the face buttons' box, a 3 x 2 grid, each a square
        // a little smaller than its cell; they move and resize as the face.
        cell_part.group = PadGroup::face;
        default_group_box(layout, PadGroup::face, &x0, &y0, &x1, &y1);
        float cell_width = (x1 - x0) / 3.0f;
        float cell_height = (y1 - y0) / 2.0f;
        float side = (cell_width < cell_height ? cell_width : cell_height) * 0.9f;
        int32_t column = cell % 3;
        int32_t row = cell < 3 ? 1 : 0;
        cell_part.x = x0 + cell_width * static_cast<float>(column) + (cell_width - side) * 0.5f;
        cell_part.y = y0 + cell_height * static_cast<float>(row) + (cell_height - side) * 0.5f;
        cell_part.width = side;
        cell_part.height = side;
    }
    else
    {
        default_group_box(layout, p->group, &x0, &y0, &x1, &y1);
    }
    const PadGroupEdit* edit = &layout->groups[static_cast<int32_t>(p->group)];
    // Resized around the group's middle, then moved.
    float middle_x = (x0 + x1) * 0.5f;
    float middle_y = (y0 + y1) * 0.5f;
    // The screen's extra room (extra_width / extra_height): by the group's
    // place in the reference, left / top third none, middle half, the rest all.
    float spread_x = edge_share(middle_x, layout->width) * layout->extra_width;
    float spread_y = edge_share(middle_y, layout->height) * layout->extra_height;
    *x = middle_x + spread_x + edit->dx + (p->x - middle_x) * edit->scale;
    *y = middle_y + spread_y + edit->dy + (p->y - middle_y) * edit->scale;
    *width = p->width * edit->scale;
    *height = p->height * edit->scale;
}

void pad_layout_group_rect(const PadLayout* layout, PadGroup group, float* x, float* y, float* width, float* height)
{
    float x0 = FLT_MAX;
    float y0 = FLT_MAX;
    float x1 = -FLT_MAX;
    float y1 = -FLT_MAX;
    for (int32_t i = 0; i < layout->count; i++)
    {
        if (layout->parts[i].group != group)
        {
            continue;
        }
        float px = 0.0f;
        float py = 0.0f;
        float pw = 0.0f;
        float ph = 0.0f;
        pad_layout_part_rect(layout, i, &px, &py, &pw, &ph);
        x0 = px < x0 ? px : x0;
        y0 = py < y0 ? py : y0;
        x1 = px + pw > x1 ? px + pw : x1;
        y1 = py + ph > y1 ? py + ph : y1;
    }
    *x = x0;
    *y = y0;
    *width = x1 - x0;
    *height = y1 - y0;
}

PadFrame pad_frame_fit_into(float width, float height, float x, float y, float room_width, float room_height)
{
    float scale_x = room_width / (width / DP_PER_RETRO_PIXEL);
    float scale_y = room_height / (height / DP_PER_RETRO_PIXEL);
    float scale = scale_x < scale_y ? scale_x : scale_y;
    PadFrame frame = {};
    frame.dp_scale = scale / DP_PER_RETRO_PIXEL;
    frame.x = std::floor(x + (room_width - width * frame.dp_scale) * 0.5f);
    frame.y = std::floor(y + (room_height - height * frame.dp_scale) * 0.5f);
    return frame;
}

PadFrame pad_frame_fit(const TouchUi* ui, float width, float height)
{
    return pad_frame_fit_into(width, height, 0.0f, 0.0f, touch_ui_width(ui), touch_ui_height(ui));
}

PadFrame pad_frame_fit_bottom(const TouchUi* ui, float width, float height)
{
    PadFrame frame = pad_frame_fit(ui, width, height);
    frame.y = std::floor(touch_ui_height(ui) - height * frame.dp_scale);
    return frame;
}

float pad_x(const PadFrame* frame, float dp)
{
    return frame->x + dp * frame->dp_scale;
}

float pad_y(const PadFrame* frame, float dp)
{
    return frame->y + dp * frame->dp_scale;
}

float pad_size(const PadFrame* frame, float dp)
{
    return dp * frame->dp_scale;
}

void pad_block(const TouchUi* ui, const PadFrame* frame, float x, float y, float width, float height, uint32_t argb)
{
    touch_ui_rect(ui, pad_x(frame, x), pad_y(frame, y), pad_size(frame, width), pad_size(frame, height), argb);
}

bool pad_button(const TouchUi* ui, const PadFrame* frame, const char* id, float x, float y, float width, float height,
                const char* label, TouchButton style, TouchText size)
{
    return touch_ui_button(ui, id, pad_x(frame, x), pad_y(frame, y), pad_size(frame, width), pad_size(frame, height), label,
                           style, size);
}

void pad_text(const TouchUi* ui, const PadFrame* frame, float x, float y, const char* text, uint32_t argb, TouchText size,
              bool centered)
{
    float left = pad_x(frame, x);
    if (centered)
    {
        left -= touch_ui_text_width(ui, text, size) * 0.5f;
    }
    touch_ui_text(ui, std::floor(left), pad_y(frame, y), text, argb, size);
}

void pad_part_draw(const TouchUi* ui, const PadPart* part, float x, float y, float width, float height, float alpha, bool held,
                   bool overlay, float stick_x, float stick_y)
{
    const RetroUiPalette* palette = &ui->palette;
    uint32_t face = overlay ? palette->button_face : palette->field_face;
    uint32_t text = overlay ? palette->text_highlight : palette->field_text;
    uint32_t nub = overlay ? palette->field_face : palette->button_face;
    touch_ui_rect(ui, x, y, width, height, retro_ui_color_with_alpha(held ? PART_LIT : face, alpha));
    if (part->kind == PadPartKind::stick)
    {
        // The nub: the body's color, 40% of the block, in the middle at
        // rest, pushed toward the block's edges as the stick is.
        float nub_width = std::floor(width * 0.4f);
        float nub_height = std::floor(height * 0.4f);
        float travel_x = (width - nub_width) * 0.5f;
        float travel_y = (height - nub_height) * 0.5f;
        touch_ui_rect(ui, std::floor(x + travel_x + stick_x * travel_x), std::floor(y + travel_y + stick_y * travel_y), nub_width,
                      nub_height, retro_ui_color_with_alpha(nub, alpha));
    }
    if (part->label[0] != '\0')
    {
        float text_width = touch_ui_text_width(ui, part->label, part->size);
        touch_ui_text(ui, std::floor(x + (width - text_width) * 0.5f), touch_ui_centered_text_y(ui, y, height, part->size),
                      part->label, retro_ui_color_with_alpha(held ? PART_LIT_TEXT : text, alpha), part->size);
    }
}

// A held stick's position, -1..1 on each axis: where its finger is from the
// block's middle, full tilt at the edge the nub can reach (40% of the block,
// see pad_part_draw) and kept there when the finger goes past it.
static float stick_axis(float finger, float start, float size)
{
    float travel = size * 0.3f;   // (size - the nub's 40%) / 2
    float value = (finger - (start + size * 0.5f)) / travel;
    return value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
}

static bool inside(float x, float y, float left, float top, float width, float height)
{
    return x >= left && x < left + width && y >= top && y < top + height;
}

// Whether a part is held, from the fingers (retro pixels: finger / scale). A
// stick: the finger it owns, or a free finger that just went down on it
// (which it takes). A button: any free finger on it.
static const TouchFinger* holding_finger(const TouchUi* ui, const PadPart* part, float x, float y, float width, float height)
{
    TouchFingers* fingers = ui->fingers;
    if (fingers == nullptr)
    {
        return nullptr;
    }
    float scale = static_cast<float>(ui->scale);
    for (int32_t i = 0; i < fingers->count; i++)
    {
        TouchFinger* finger = &fingers->fingers[i];
        if (part->kind == PadPartKind::stick && finger->owner == part->id)
        {
            return finger;
        }
    }
    for (int32_t i = 0; i < fingers->count; i++)
    {
        TouchFinger* finger = &fingers->fingers[i];
        if (finger->owner != nullptr || !inside(finger->x / scale, finger->y / scale, x, y, width, height))
        {
            continue;
        }
        if (part->kind == PadPartKind::stick)
        {
            if (finger->began)
            {
                finger->owner = part->id;
                return finger;
            }
            continue;
        }
        return finger;
    }
    return nullptr;
}

// tan(22.5 degrees): within it of an axis, a finger presses that one
// direction; between, the diagonal (two).
constexpr float DPAD_SECTOR_SLOPE = 0.41421356f;

// A RetroPad button's bit (directions, A / B...).
static uint32_t direction_bit(RetroPadButton button)
{
    return 1u << static_cast<uint32_t>(button);
}

uint32_t pad_dpad_directions(float dx, float dy)
{
    if (dx * dx + dy * dy < PAD_DPAD_DEADZONE * PAD_DPAD_DEADZONE)
    {
        return 0;
    }
    float across = std::fabs(dx);
    float down = std::fabs(dy);
    uint32_t horizontal = dx < 0.0f ? direction_bit(RetroPadButton::left) : direction_bit(RetroPadButton::right);
    uint32_t vertical = dy < 0.0f ? direction_bit(RetroPadButton::up) : direction_bit(RetroPadButton::down);
    if (down <= across * DPAD_SECTOR_SLOPE)
    {
        return horizontal;
    }
    if (across <= down * DPAD_SECTOR_SLOPE)
    {
        return vertical;
    }
    return horizontal | vertical;
}

// The directions the fingers press on the d-pad: every free finger inside its
// square (the group's box: the arrows and the corners between them).
static uint32_t dpad_held(const TouchUi* ui, const PadFrame* frame, const PadLayout* layout)
{
    TouchFingers* fingers = ui->fingers;
    if (fingers == nullptr || !pad_layout_has_group(layout, PadGroup::dpad))
    {
        return 0;
    }
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    pad_layout_group_rect(layout, PadGroup::dpad, &x, &y, &width, &height);
    float screen_x = pad_x(frame, x);
    float screen_y = pad_y(frame, y);
    float half_width = pad_size(frame, width) * 0.5f;
    float half_height = pad_size(frame, height) * 0.5f;
    if (half_width <= 0.0f || half_height <= 0.0f)
    {
        return 0;
    }
    float scale = static_cast<float>(ui->scale);
    uint32_t directions = 0;
    for (int32_t i = 0; i < fingers->count; i++)
    {
        const TouchFinger* finger = &fingers->fingers[i];
        float finger_x = finger->x / scale;
        float finger_y = finger->y / scale;
        if (finger->owner != nullptr || !inside(finger_x, finger_y, screen_x, screen_y, half_width * 2.0f, half_height * 2.0f))
        {
            continue;
        }
        float dx = (finger_x - (screen_x + half_width)) / half_width;
        float dy = (finger_y - (screen_y + half_height)) / half_height;
        directions |= pad_dpad_directions(dx, dy);
    }
    return directions;
}

void pad_face_zone(float x, float y, float width, float height, float* zone_x, float* zone_y, float* zone_width,
                   float* zone_height)
{
    float shorter = width < height ? width : height;
    float grow = shorter * PAD_FACE_REACH;
    *zone_x = x - grow;
    *zone_y = y - grow;
    *zone_width = width + 2.0f * grow;
    *zone_height = height + 2.0f * grow;
}

bool pad_pair_zone(float x, float y, float a_x, float a_y, float b_x, float b_y)
{
    float apart_x = b_x - a_x;
    float apart_y = b_y - a_y;
    float reach = PAD_PAIR_REACH * PAD_PAIR_REACH * (apart_x * apart_x + apart_y * apart_y);
    float to_a = (x - a_x) * (x - a_x) + (y - a_y) * (y - a_y);
    float to_b = (x - b_x) * (x - b_x) + (y - b_y) * (y - b_y);
    return to_a <= reach && to_b <= reach;
}

// Whether the layout's style has A and B and no other face button (NES /
// Game Boy): only then does the space between them press both.
static bool two_face_buttons(const PadLayout* layout)
{
    const PadStyle* style = layout->style;
    if (style == nullptr || style->six_face)
    {
        return false;
    }
    return style->labels[static_cast<int32_t>(RetroPadButton::a)] != nullptr &&
           style->labels[static_cast<int32_t>(RetroPadButton::b)] != nullptr &&
           style->labels[static_cast<int32_t>(RetroPadButton::x)] == nullptr &&
           style->labels[static_cast<int32_t>(RetroPadButton::y)] == nullptr;
}

// A part's middle on the screen (retro pixels).
static void part_middle(const PadFrame* frame, const PadLayout* layout, int32_t part, float* x, float* y)
{
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    pad_layout_part_rect(layout, part, &left, &top, &width, &height);
    *x = pad_x(frame, left + width * 0.5f);
    *y = pad_y(frame, top + height * 0.5f);
}

// A and B (their bits) when a free finger is between them on a two-button
// style (pad_pair_zone), else 0.
static uint32_t pair_held(const TouchUi* ui, const PadFrame* frame, const PadLayout* layout)
{
    TouchFingers* fingers = ui->fingers;
    if (fingers == nullptr || !two_face_buttons(layout))
    {
        return 0;
    }
    int32_t a_part = -1;
    int32_t b_part = -1;
    for (int32_t i = 0; i < layout->count; i++)
    {
        RetroPadButton button = RetroPadButton::up;
        if (layout->parts[i].group != PadGroup::face || !retropad_button_for_part(layout->parts[i].id, &button))
        {
            continue;
        }
        a_part = button == RetroPadButton::a ? i : a_part;
        b_part = button == RetroPadButton::b ? i : b_part;
    }
    if (a_part < 0 || b_part < 0)
    {
        return 0;
    }
    float a_x = 0.0f;
    float a_y = 0.0f;
    float b_x = 0.0f;
    float b_y = 0.0f;
    part_middle(frame, layout, a_part, &a_x, &a_y);
    part_middle(frame, layout, b_part, &b_x, &b_y);
    float scale = static_cast<float>(ui->scale);
    for (int32_t i = 0; i < fingers->count; i++)
    {
        const TouchFinger* finger = &fingers->fingers[i];
        if (finger->owner == nullptr && pad_pair_zone(finger->x / scale, finger->y / scale, a_x, a_y, b_x, b_y))
        {
            return direction_bit(RetroPadButton::a) | direction_bit(RetroPadButton::b);
        }
    }
    return 0;
}

// A stick held on a RetroPad: pushed past a small dead zone, or clicked.
constexpr float LIT_STICK_DEADZONE = 0.25f;

// Whether a part is held on `lit` (a real gamepad as a RetroPad), and a
// stick's position there (-1..1).
static bool lit_part(const TouchUi* ui, const PadPart* part, float* stick_x, float* stick_y)
{
    *stick_x = 0.0f;
    *stick_y = 0.0f;
    if (part->kind == PadPartKind::action)
    {
        return ui->action_lit;
    }
    if (part->kind == PadPartKind::menu)
    {
        return false;   // no gamepad button lights it
    }
    const RetroPad* lit = ui->lit;
    if (lit == nullptr)
    {
        return false;
    }
    if (part->kind == PadPartKind::stick)
    {
        int32_t which = part->group == PadGroup::right_stick ? 1 : 0;
        RetroPadButton click = which == 1 ? RetroPadButton::r3 : RetroPadButton::l3;
        *stick_x = static_cast<float>(lit->analog[which][0]) / 32767.0f;
        *stick_y = static_cast<float>(lit->analog[which][1]) / 32767.0f;
        bool clicked = (lit->buttons & (1u << static_cast<uint32_t>(click))) != 0;
        return clicked || std::fabs(*stick_x) > LIT_STICK_DEADZONE || std::fabs(*stick_y) > LIT_STICK_DEADZONE;
    }
    RetroPadButton button = RetroPadButton::b;
    return retropad_button_for_part(part->id, &button) && (lit->buttons & (1u << static_cast<uint32_t>(button))) != 0;
}

bool pad_layout_shown_part(const PadLayout* layout, int32_t part, PadPart* shown)
{
    *shown = layout->parts[part];
    if (shown->kind == PadPartKind::action || shown->kind == PadPartKind::menu)
    {
        const char* label = shown->kind == PadPartKind::action ? layout->action_label : layout->menu_label;
        if (label == nullptr)
        {
            return false;
        }
        shown->label = label;
        return true;
    }
    const PadStyle* style = layout->style;
    if (style == nullptr)
    {
        return true;
    }
    if (shown->kind == PadPartKind::stick)
    {
        return style->sticks;
    }
    RetroPadButton button = RetroPadButton::b;
    if (!retropad_button_for_part(shown->id, &button))
    {
        return true;   // the d-pad's middle
    }
    const char* label = style->labels[static_cast<int32_t>(button)];
    if (label == nullptr)
    {
        return false;
    }
    if (label[0] != '\0')
    {
        shown->label = label;
    }
    return true;
}

void pad_layout_draw(const TouchUi* ui, const PadFrame* frame, PadLayout* layout)
{
    uint32_t held_now = 0;
    bool action_was_held = layout->action_held;
    bool menu_was_held = layout->menu_held;
    layout->action_held = false;
    layout->action_pressed = false;
    layout->menu_held = false;
    layout->menu_pressed = false;
    uint32_t dpad = dpad_held(ui, frame, layout);
    uint32_t pair = pair_held(ui, frame, layout);
    for (int32_t i = 0; i < layout->count && i < PAD_MAX_PARTS; i++)
    {
        PadPart shown;   // the action / menu button with the frontend's label
        if (!pad_layout_shown_part(layout, i, &shown))
        {
            continue;
        }
        const PadPart* part = &shown;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        pad_layout_part_rect(layout, i, &x, &y, &width, &height);
        float screen_x = pad_x(frame, x);
        float screen_y = pad_y(frame, y);
        float screen_width = pad_size(frame, width);
        float screen_height = pad_size(frame, height);
        // The d-pad's parts are held from its area (dpad_held; its middle
        // presses nothing), the face buttons by a finger in their bigger
        // zone (pad_face_zone), every other part by a finger on it; A and B
        // also from the space between them on a two-button style (pair_held).
        const TouchFinger* finger = nullptr;
        bool touched = false;
        RetroPadButton button = RetroPadButton::b;
        bool is_button = retropad_button_for_part(part->id, &button);
        if (part->group == PadGroup::dpad)
        {
            touched = is_button && (dpad & direction_bit(button)) != 0;
        }
        else
        {
            float zone_x = screen_x;
            float zone_y = screen_y;
            float zone_width = screen_width;
            float zone_height = screen_height;
            if (part->group == PadGroup::face && part->kind == PadPartKind::button)
            {
                pad_face_zone(screen_x, screen_y, screen_width, screen_height, &zone_x, &zone_y, &zone_width, &zone_height);
            }
            finger = holding_finger(ui, part, zone_x, zone_y, zone_width, zone_height);
            touched = finger != nullptr || (is_button && (pair & direction_bit(button)) != 0);
        }
        float stick_x = 0.0f;
        float stick_y = 0.0f;
        bool on_gamepad = lit_part(ui, part, &stick_x, &stick_y);
        bool held = touched || on_gamepad;
        if (touched && part->kind == PadPartKind::action)
        {
            layout->action_held = true;
            layout->action_pressed = !action_was_held;
        }
        if (touched && part->kind == PadPartKind::menu)
        {
            layout->menu_held = true;
            layout->menu_pressed = !menu_was_held;
        }
        if (touched && is_button && ui->pad != nullptr)
        {
            // What the pad gives: the parts fingers hold (a real gamepad's
            // presses are added from the gamepad itself).
            retropad_press(ui->pad, button);
        }
        if (touched)
        {
            uint32_t bit = 1u << static_cast<uint32_t>(i);
            held_now |= bit;
            if ((layout->held & bit) == 0 && ui->fingers != nullptr)
            {
                ui->fingers->presses++;
            }
            if (part->kind == PadPartKind::stick && finger != nullptr)
            {
                float scale = static_cast<float>(ui->scale);
                stick_x = stick_axis(finger->x / scale, screen_x, screen_width);
                stick_y = stick_axis(finger->y / scale, screen_y, screen_height);
                if (ui->pad != nullptr)
                {
                    int32_t which = part->group == PadGroup::right_stick ? 1 : 0;
                    ui->pad->analog[which][0] = retropad_axis(stick_x);
                    ui->pad->analog[which][1] = retropad_axis(stick_y);
                }
            }
        }
        pad_part_draw(ui, part, screen_x, screen_y, screen_width, screen_height, layout->alpha, held, layout->overlay, stick_x,
                      stick_y);
    }
    layout->held = held_now;
}
