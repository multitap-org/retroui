// retro_ui_controls.h drawn with ImGui and the retro theme.

#include "retro_ui_controls.h"

#include <cstdio>

#include "imgui.h"

#include "retro_ui.h"

// Room kept on the right of the controls: the row's end button (for every
// control in the row) and a reserved button (for the next control only).
static float g_row_reserved = 0.0f;
static float g_next_reserved = 0.0f;

static float button_width(const char* label)
{
    return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

static float spacing()
{
    return ImGui::GetStyle().ItemSpacing.x;
}

// The width the next control fills, and the reserved button is used up.
static float take_control_width()
{
    float width = ImGui::GetContentRegionAvail().x - g_row_reserved - g_next_reserved;
    g_next_reserved = 0.0f;
    return width;
}

void retro_ui_row_begin(const RetroUiRow* row)
{
    ImGui::PushID(row->id);
    g_row_reserved = row->end_button != nullptr ? button_width(row->end_button) + spacing() : 0.0f;
    g_next_reserved = 0.0f;
    if (row->marker_column)
    {
        if (row->marker == nullptr)
        {
            ImGui::Dummy(ImGui::CalcTextSize("C"));
        }
        else
        {
            ImGui::TextColored(retro_ui_color_vec4(row->marker_color), "%s", row->marker);
            if (row->marker_help != nullptr)
            {
                retro_ui_item_tooltip(row->marker_help);
            }
        }
        ImGui::SameLine();
    }
    // Only when it is: ImGui counts BeginDisabled(false) too, and its color
    // swatches then draw with a saved opacity that may be 0 (invisible).
    if (!row->enabled)
    {
        ImGui::BeginDisabled();
    }
    if (row->label != nullptr)
    {
        ImGui::AlignTextToFramePadding();
        float start = ImGui::GetCursorPosX();
        ImGui::TextUnformatted(row->label);
        if (row->help != nullptr)
        {
            retro_ui_item_tooltip(row->help);
        }
        if (row->label_width > 0.0f)
        {
            ImGui::SameLine(start + row->label_width);
        }
        else
        {
            ImGui::SameLine();
        }
    }
}

bool retro_ui_row_end(const RetroUiRow* row)
{
    bool pressed = false;
    if (!row->enabled)
    {
        ImGui::EndDisabled();
        // A disabled item needs AllowWhenDisabled for its tooltip.
        if (row->disabled_help != nullptr && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            retro_ui_tooltip(row->disabled_help);
        }
    }
    else if (row->end_button != nullptr && !row->end_button_hidden)
    {
        ImGui::SameLine(ImGui::GetContentRegionMax().x - button_width(row->end_button));
        ImGui::PushID("end");
        pressed = retro_ui_button(row->end_button);
        ImGui::PopID();
        if (row->end_button_help != nullptr)
        {
            retro_ui_item_tooltip(row->end_button_help);
        }
    }
    g_row_reserved = 0.0f;
    g_next_reserved = 0.0f;
    ImGui::PopID();
    return pressed;
}

void retro_ui_reserve_button(const char* label)
{
    g_next_reserved += button_width(label) + spacing();
}

bool retro_ui_row_button(const char* id, const char* label)
{
    ImGui::SameLine();
    ImGui::PushID(id);
    bool pressed = retro_ui_button(label);
    ImGui::PopID();
    return pressed;
}

float retro_ui_text_width(const char* text)
{
    return ImGui::CalcTextSize(text).x;
}

float retro_ui_label_column_width(float widest)
{
    return widest + spacing() * 2.0f;
}

void retro_ui_dim_text(const char* text, const char* help)
{
    ImGui::TextDisabled("%s", text);
    if (help != nullptr)
    {
        retro_ui_item_tooltip(help);
    }
}

bool retro_ui_check(const char* id, const char* label, bool* value)
{
    ImGui::PushID(id);
    char imgui_label[512] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###check", label);
    bool changed = retro_ui_checkbox(imgui_label, value);
    ImGui::PopID();
    return changed;
}

bool retro_ui_number_field(const char* id, int32_t* value, int32_t step)
{
    ImGui::PushID(id);
    int number = static_cast<int>(*value);
    ImGui::SetNextItemWidth(take_control_width());
    retro_ui_push_field_text();
    bool changed = ImGui::InputInt("##number", &number, step > 0 ? step : 1, 8);
    retro_ui_pop_field_text();
    ImGui::PopID();
    *value = static_cast<int32_t>(number);
    return changed;
}

bool retro_ui_slider(const char* id, float* value, float min, float max, const char* format)
{
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(take_control_width());
    bool changed = ImGui::SliderFloat("##slider", value, min, max, format);
    ImGui::PopID();
    return changed;
}

bool retro_ui_color_field(const char* id, uint32_t* argb)
{
    ImGui::PushID(id);
    ImVec4 color = retro_ui_color_vec4(*argb);
    float rgb[3] = {color.x, color.y, color.z};
    bool changed = ImGui::ColorEdit3("##color", rgb, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                                                        ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoAlpha);
    if (changed)
    {
        uint32_t picked = 0xFF000000u;
        for (int i = 0; i < 3; i++)
        {
            float channel = rgb[i] < 0.0f ? 0.0f : (rgb[i] > 1.0f ? 1.0f : rgb[i]);
            picked |= static_cast<uint32_t>(channel * 255.0f + 0.5f) << (16 - 8 * i);
        }
        *argb = picked;
    }
    retro_ui_raised_item_edge();
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextDisabled("#%06X", static_cast<unsigned>(*argb & 0xFFFFFFu));
    ImGui::PopID();
    g_next_reserved = 0.0f;
    return changed;
}

bool retro_ui_text_field(const char* id, const char* hint, const char* value, char* edited, size_t edited_size)
{
    // ImGui keeps its own copy of the text while the field is being edited;
    // `edited` only seeds it, and holds the result when editing ends.
    ImGui::PushID(id);
    std::snprintf(edited, edited_size, "%s", value);
    ImGui::SetNextItemWidth(take_control_width());
    retro_ui_push_field_text();
    ImGui::InputTextWithHint("##text", hint != nullptr ? hint : "", edited, edited_size);
    retro_ui_pop_field_text();
    bool done = ImGui::IsItemDeactivatedAfterEdit();
    ImGui::PopID();
    return done;
}

bool retro_ui_picker_field(const char* id, const char* hint, const char* value, const char* help)
{
    // Read-only: it looks like any other field (and a long path can be
    // scrolled), but a click opens the picker, as "..." does for whoever
    // doesn't think of clicking the field.
    ImGui::PushID(id);
    const char* dots = "...";
    char buffer[4096] = {};
    std::snprintf(buffer, sizeof(buffer), "%s", value);
    ImGui::SetNextItemWidth(take_control_width() - button_width(dots) - spacing());
    retro_ui_push_field_text();
    ImGui::InputTextWithHint("##field", hint != nullptr ? hint : "", buffer, sizeof(buffer),
                             ImGuiInputTextFlags_ReadOnly);
    retro_ui_pop_field_text();
    bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    if (help != nullptr)
    {
        retro_ui_item_tooltip(help);
    }
    ImGui::SameLine();
    if (retro_ui_button(dots))
    {
        clicked = true;
    }
    if (help != nullptr)
    {
        retro_ui_item_tooltip(help);
    }
    ImGui::PopID();
    return clicked;
}

bool retro_ui_begin_choice(const char* id, const char* preview)
{
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(take_control_width());
    bool open = retro_ui_begin_combo("##choice", preview);
    if (!open)
    {
        ImGui::PopID();
    }
    return open;
}

bool retro_ui_choice_item(uint32_t index, const char* label, bool selected)
{
    ImGui::PushID(static_cast<int>(index));
    bool picked = ImGui::Selectable(label, selected);
    ImGui::PopID();
    return picked && !selected;
}

void retro_ui_end_choice()
{
    ImGui::EndCombo();
    ImGui::PopID();
}
