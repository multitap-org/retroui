#include "pad_editor.h"

#include <cmath>
#include <cstdio>

#include "imgui.h"

#include "touch_fingers.h"

constexpr float SNAP_DP       = 8.0f;
constexpr float SIZE_STEP     = 0.1f;
constexpr float OPACITY_STEP  = 0.1f;
constexpr uint32_t PICKED     = 0xFFFFDC50u;   // the lit yellow
constexpr float HANDLE        = 4.0f;          // corner squares, retro pixels

// Toolbar, in retro pixels.
constexpr float BAR_WIDTH   = 190.0f;
constexpr float BAR_PADDING = 8.0f;
constexpr float BAR_ROW     = 24.0f;
constexpr float BAR_GAP     = 4.0f;
constexpr float BAR_TITLE   = 12.0f;

static float clampf(float value, float minimum, float maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

static float snapped(float value)
{
    return std::round(value / SNAP_DP) * SNAP_DP;
}

// A group's box on the screen, in retro pixels.
static void group_box(const PadFrame* frame, const PadLayout* layout, PadGroup group, float* x, float* y, float* width,
                      float* height)
{
    float dx = 0.0f;
    float dy = 0.0f;
    float dw = 0.0f;
    float dh = 0.0f;
    pad_layout_group_rect(layout, group, &dx, &dy, &dw, &dh);
    *x = pad_x(frame, dx);
    *y = pad_y(frame, dy);
    *width = pad_size(frame, dw);
    *height = pad_size(frame, dh);
}

// One "NAME   [-] value [+]" row. Returns -1, +1 or 0.
static int32_t stepper(const TouchUi* ui, const char* id, const char* name, const char* value, float x, float y, float width)
{
    touch_ui_text(ui, x, touch_ui_centered_text_y(ui, y, BAR_ROW, TouchText::normal), name, ui->palette.text, TouchText::normal);
    constexpr float VALUE_WIDTH = 40.0f;
    float stepper_width = 2.0f * (BAR_ROW + TOUCH_UI_GAP) + VALUE_WIDTH;
    return touch_ui_stepper(ui, id, value, x + width - stepper_width, y, stepper_width, BAR_ROW);
}

// Two fingers down: the picked group's size follows the distance between
// them (relative to where the pinch began). True while pinching.
static bool pinch(PadEditor* editor, const TouchUi* ui, PadLayout* layout)
{
    const TouchFingers* fingers = ui->fingers;
    const TouchFinger* down[2] = {nullptr, nullptr};
    int32_t count = 0;
    for (int32_t i = 0; fingers != nullptr && i < fingers->count; i++)
    {
        if (!fingers->fingers[i].lifted)
        {
            if (count < 2)
            {
                down[count] = &fingers->fingers[i];
            }
            count++;
        }
    }
    if (count != 2)
    {
        editor->pinching = false;
        return false;
    }
    float dx = down[1]->x - down[0]->x;
    float dy = down[1]->y - down[0]->y;
    float distance = std::sqrt(dx * dx + dy * dy);
    PadGroupEdit* edit = &layout->groups[static_cast<int32_t>(editor->selected)];
    if (!editor->pinching)
    {
        editor->pinching = distance > 1.0f;
        editor->pinch_distance = distance;
        editor->pinch_scale = edit->scale;
        return editor->pinching;
    }
    edit->scale = clampf(editor->pinch_scale * distance / editor->pinch_distance, PAD_SCALE_MIN, PAD_SCALE_MAX);
    return true;
}

bool pad_editor_draw(PadEditor* editor, const TouchUi* ui, const PadFrame* frame, PadLayout* layout)
{
    bool pinching = pinch(editor, ui, layout);
    const RetroUiPalette* palette = &ui->palette;
    float screen_width = touch_ui_width(ui);
    float screen_height = touch_ui_height(ui);
    if (!pad_layout_has_group(layout, editor->selected))
    {
        editor->selected = PadGroup::dpad;
    }

    // The parts (not playable while editing), each group outlined, the
    // picked one in yellow with corner handles.
    for (int32_t i = 0; i < layout->count; i++)
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        pad_layout_part_rect(layout, i, &x, &y, &width, &height);
        PadPart part;
        if (!pad_layout_shown_part(layout, i, &part))
        {
            continue;   // (not shown in play either)
        }
        pad_part_draw(ui, &part, pad_x(frame, x), pad_y(frame, y), pad_size(frame, width), pad_size(frame, height),
                      layout->alpha, false, layout->overlay, 0.0f, 0.0f);
    }
    for (int32_t g = 0; g < PAD_GROUP_COUNT; g++)
    {
        PadGroup group = static_cast<PadGroup>(g);
        if (!pad_layout_has_group(layout, group))
        {
            continue;
        }
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        group_box(frame, layout, group, &x, &y, &width, &height);
        x -= 2.0f;
        y -= 2.0f;
        width += 4.0f;
        height += 4.0f;
        if (group == editor->selected)
        {
            touch_ui_outline(ui, x, y, width, height, 1.0f, PICKED);
            float half = HANDLE * 0.5f;
            touch_ui_rect(ui, x - half, y - half, HANDLE, HANDLE, PICKED);
            touch_ui_rect(ui, x + width - half, y - half, HANDLE, HANDLE, PICKED);
            touch_ui_rect(ui, x - half, y + height - half, HANDLE, HANDLE, PICKED);
            touch_ui_rect(ui, x + width - half, y + height - half, HANDLE, HANDLE, PICKED);
        }
        else
        {
            touch_ui_outline(ui, x, y, width, height, 1.0f, retro_ui_color_with_alpha(palette->text_highlight, 0.5f));
        }
    }

    // The toolbar, away from the picked group: at the bottom when the group
    // is in the top half, else at the top.
    float picked_x = 0.0f;
    float picked_y = 0.0f;
    float picked_width = 0.0f;
    float picked_height = 0.0f;
    group_box(frame, layout, editor->selected, &picked_x, &picked_y, &picked_width, &picked_height);
    float bar_width = BAR_WIDTH < screen_width - 16.0f ? BAR_WIDTH : screen_width - 16.0f;
    float bar_height = 2.0f * BAR_PADDING + BAR_TITLE + 3.0f * BAR_ROW + 3.0f * BAR_GAP;
    float bar_x = std::floor((screen_width - bar_width) * 0.5f);
    bool group_on_top = picked_y + picked_height * 0.5f < screen_height * 0.5f;
    float bar_y = group_on_top ? screen_height - 8.0f - bar_height : 8.0f;
    touch_ui_rect(ui, bar_x, bar_y, bar_width, bar_height, palette->panel_face);

    float x = bar_x + BAR_PADDING;
    float inner = bar_width - 2.0f * BAR_PADDING;
    float y = bar_y + BAR_PADDING;
    touch_ui_text(ui, x, y, "EDIT LAYOUT", palette->text_highlight, TouchText::normal);
    const char* group_name = pad_group_label(editor->selected);
    touch_ui_text(ui, x + inner - touch_ui_text_width(ui, group_name, TouchText::normal), y, group_name, PICKED,
                  TouchText::normal);
    y += BAR_TITLE + BAR_GAP;

    PadGroupEdit* edit = &layout->groups[static_cast<int32_t>(editor->selected)];
    char value[16];
    std::snprintf(value, sizeof(value), "%d%%", static_cast<int>(std::lround(edit->scale * 100.0f)));
    int32_t step = stepper(ui, "##size", "SIZE", value, x, y, inner);
    edit->scale = clampf(edit->scale + static_cast<float>(step) * SIZE_STEP, PAD_SCALE_MIN, PAD_SCALE_MAX);
    y += BAR_ROW + BAR_GAP;

    std::snprintf(value, sizeof(value), "%d%%", static_cast<int>(std::lround(layout->alpha * 100.0f)));
    step = stepper(ui, "##opacity", "OPACITY", value, x, y, inner);
    if (step != 0)
    {
        layout->alpha = clampf(layout->alpha + static_cast<float>(step) * OPACITY_STEP, PAD_ALPHA_MIN, 1.0f);
        layout->alpha_set = true;
    }
    y += BAR_ROW + BAR_GAP;

    float third = (inner - 2.0f * BAR_GAP) / 3.0f;
    bool done = false;
    if (touch_ui_button(ui, "##snap", x, y, third, BAR_ROW, "SNAP", editor->snap ? TouchButton::primary : TouchButton::normal,
                        TouchText::normal))
    {
        editor->snap = !editor->snap;
    }
    if (touch_ui_button(ui, "##reset", x + third + BAR_GAP, y, third, BAR_ROW, "RESET", TouchButton::normal, TouchText::normal))
    {
        pad_layout_reset(layout);
    }
    if (touch_ui_button(ui, "##done", x + 2.0f * (third + BAR_GAP), y, third, BAR_ROW, "DONE", TouchButton::primary,
                        TouchText::normal))
    {
        done = true;
    }

    // Taps go to the first item under them: the toolbar's buttons above, then
    // the toolbar itself (so they don't reach a group under it), then the groups.
    bool held = false;
    touch_ui_area(ui, "##edit_bar", bar_x, bar_y, bar_width, bar_height, &held, TouchNav::taps_only);
    float pixels_per_dp = static_cast<float>(ui->scale) * frame->dp_scale;
    for (int32_t g = 0; g < PAD_GROUP_COUNT; g++)
    {
        PadGroup group = static_cast<PadGroup>(g);
        if (!pad_layout_has_group(layout, group))
        {
            continue;
        }
        float gx = 0.0f;
        float gy = 0.0f;
        float gw = 0.0f;
        float gh = 0.0f;
        group_box(frame, layout, group, &gx, &gy, &gw, &gh);
        char id[32];
        std::snprintf(id, sizeof(id), "##edit_group%d", static_cast<int>(g));
        bool released = touch_ui_area(ui, id, gx, gy, gw, gh, &held, TouchNav::taps_only);
        PadGroupEdit* group_edit = &layout->groups[g];
        if (held && !pinching)
        {
            // Dragging: the finger's movement this frame, in dp.
            editor->selected = group;
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            group_edit->dx += delta.x / pixels_per_dp;
            group_edit->dy += delta.y / pixels_per_dp;
        }
        if (released && editor->snap)
        {
            group_edit->dx = snapped(group_edit->dx);
            group_edit->dy = snapped(group_edit->dy);
        }
    }
    return done;
}
