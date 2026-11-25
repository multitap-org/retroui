// retro_ui_controls.h drawn with ImGui and the retro theme.

#include "retro_ui_controls.h"

#include <cstdarg>
#include <cstdio>

#include "imgui.h"

#include "retro_ui.h"

// File state, as ImGui's own (an immediate-mode GUI keeps one current
// window, line and ID stack): it is only touched between a begin and its end
// in one frame, on the UI thread.
//
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

// ---------------------------------------------------------------------------
// Windows, text and layout.

// retro_ui_window_end needs to know whether retro_ui_begin_window ran.
constexpr int32_t WINDOW_DEPTH_MAX = 8;
static bool g_window_visible[WINDOW_DEPTH_MAX] = {};
static int32_t g_window_depth = 0;

// begin_disabled(false) must not reach ImGui (its swatches then draw with a
// saved opacity that may be 0): a stack of what each begin did.
constexpr int32_t DISABLED_DEPTH_MAX = 16;
static bool g_disabled[DISABLED_DEPTH_MAX] = {};
static int32_t g_disabled_depth = 0;

bool retro_ui_window_begin(const RetroUiWindow* window, int32_t scale, bool* closed)
{
    if (closed != nullptr)
    {
        *closed = false;
    }
    const ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (window->width > 0.0f || window->height > 0.0f)
    {
        ImGui::SetNextWindowSize(ImVec2(window->width, window->height),
                                 window->resizable ? ImGuiCond_FirstUseEver : ImGuiCond_Always);
    }
    // A side left at 0 follows the contents every frame (the other one, when
    // given, stays as set: ImGui leaves a side set by SetNextWindowSize).
    bool sized = window->width > 0.0f && window->height > 0.0f;
    if ((window->width <= 0.0f && window->height <= 0.0f) || (!sized && !window->resizable))
    {
        flags |= ImGuiWindowFlags_AlwaysAutoResize;
    }
    if (!window->resizable)
    {
        flags |= ImGuiWindowFlags_NoResize;
    }
    if (window->centered)
    {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                                ImVec2(0.5f, 0.5f));
        flags |= ImGuiWindowFlags_NoMove;
    }
    else
    {
        ImGui::SetNextWindowPos(ImVec2(window->x, window->y), ImGuiCond_FirstUseEver);
    }
    if (window->focus)
    {
        ImGui::SetNextWindowFocus();
    }
    char title[300] = {};
    std::snprintf(title, sizeof(title), "%s###%s", window->title != nullptr ? window->title : "", window->id);
    bool visible = ImGui::Begin(title, nullptr, flags);
    if (visible)
    {
        retro_ui_begin_window(scale);
        if (window->closable && retro_ui_close_button() && closed != nullptr)
        {
            *closed = true;
        }
    }
    if (g_window_depth < WINDOW_DEPTH_MAX)
    {
        g_window_visible[g_window_depth] = visible;
    }
    g_window_depth++;
    return visible;
}

void retro_ui_window_end()
{
    g_window_depth--;
    if (g_window_depth >= 0 && g_window_depth < WINDOW_DEPTH_MAX && g_window_visible[g_window_depth])
    {
        retro_ui_end_window();
    }
    ImGui::End();
}

void retro_ui_text(const char* text)
{
    ImGui::TextUnformatted(text);
}

void retro_ui_text_format(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    ImGui::TextV(format, arguments);
    va_end(arguments);
}

void retro_ui_text_wrapped(const char* text)
{
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
}

void retro_ui_text_colored(uint32_t argb, const char* text)
{
    ImGui::TextColored(retro_ui_color_vec4(argb), "%s", text);
}

void retro_ui_label(const char* text)
{
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text);
}

void retro_ui_same_line()
{
    ImGui::SameLine();
}

void retro_ui_same_line_at(float x)
{
    ImGui::SameLine(x);
}

void retro_ui_spacing()
{
    ImGui::Spacing();
}

void retro_ui_separator()
{
    ImGui::Separator();
}

void retro_ui_indent()
{
    ImGui::Indent();
}

void retro_ui_unindent()
{
    ImGui::Unindent();
}

void retro_ui_begin_disabled(bool disabled)
{
    if (g_disabled_depth < DISABLED_DEPTH_MAX)
    {
        g_disabled[g_disabled_depth] = disabled;
    }
    g_disabled_depth++;
    if (disabled)
    {
        ImGui::BeginDisabled();
    }
}

void retro_ui_end_disabled()
{
    g_disabled_depth--;
    if (g_disabled_depth >= 0 && g_disabled_depth < DISABLED_DEPTH_MAX && g_disabled[g_disabled_depth])
    {
        ImGui::EndDisabled();
    }
}

void retro_ui_push_id(const char* id)
{
    ImGui::PushID(id);
}

void retro_ui_push_index(uint32_t index)
{
    ImGui::PushID(static_cast<int>(index));
}

void retro_ui_pop_id()
{
    ImGui::PopID();
}

float retro_ui_width_left()
{
    return ImGui::GetContentRegionAvail().x;
}

float retro_ui_line_height()
{
    return ImGui::GetFrameHeight();
}

float retro_ui_button_width(const char* label)
{
    return button_width(label);
}
