// retro_ui_controls.h drawn with ImGui and the retro theme.

#include "retro_ui_controls.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>

#include "imgui.h"
#include "imgui_internal.h"   // BringWindowToDisplayFront

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

// UTF-8 characters in `text` (continuation bytes 10xxxxxx don't count).
static uint32_t character_count(const char* text, int32_t length)
{
    uint32_t count = 0;
    for (int32_t i = 0; i < length; i++)
    {
        if ((static_cast<unsigned char>(text[i]) & 0xC0u) != 0x80u)
        {
            count++;
        }
    }
    return count;
}

// Byte offset where character number `characters` starts (or the end).
static int32_t offset_of_character(const char* text, int32_t length, uint32_t characters)
{
    uint32_t count = 0;
    for (int32_t i = 0; i < length; i++)
    {
        if ((static_cast<unsigned char>(text[i]) & 0xC0u) != 0x80u)
        {
            if (count == characters)
            {
                return i;
            }
            count++;
        }
    }
    return length;
}

// Dear ImGui limits a field in bytes; the caller counts characters. Numbers
// take digits only. UserData: the RetroUiEditField.
static int edit_field_callback(ImGuiInputTextCallbackData* data)
{
    const RetroUiEditField* field = static_cast<const RetroUiEditField*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
    {
        bool digit = data->EventChar >= '0' && data->EventChar <= '9';
        return field->digits_only && !digit ? 1 : 0;   // 1 = drop the character
    }
    if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit && field->max_characters > 0 &&
        character_count(data->Buf, data->BufTextLen) > field->max_characters)
    {
        int32_t cut = offset_of_character(data->Buf, data->BufTextLen, field->max_characters);
        data->DeleteChars(cut, data->BufTextLen - cut);
    }
    return 0;
}

bool retro_ui_edit_field(const char* id, char* buffer, size_t buffer_size, const RetroUiEditField* field)
{
    ImGui::PushID(id);
    if (field->focus)
    {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(take_control_width());
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit |
                                ImGuiInputTextFlags_CallbackCharFilter;
    // The options aren't changed through UserData; Dear ImGui only takes a void*.
    void* user_data = const_cast<RetroUiEditField*>(field);
    retro_ui_push_field_text();
    bool entered = ImGui::InputText("##edit", buffer, buffer_size, flags, edit_field_callback, user_data);
    retro_ui_pop_field_text();
    ImGui::PopID();
    return entered;
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

bool retro_ui_list_item(uint32_t index, const char* label, bool selected, const char* note)
{
    ImGui::PushID(static_cast<int>(index));
    char imgui_label[512] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###item", label);
    bool clicked = ImGui::Selectable(imgui_label, selected);
    if (note != nullptr)
    {
        // At the list's right end, whether its scroll bar shows or not.
        ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(note).x - spacing() * 2.0f);
        ImGui::TextDisabled("%s", note);
    }
    ImGui::PopID();
    return clicked;
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
    else if (window->centered_on_open)
    {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing,
                                ImVec2(0.5f, 0.5f));
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

void retro_ui_bring_window_to_front()
{
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
}

void retro_ui_window_chrome(float* width, float* height)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    *width = style.WindowPadding.x * 2.0f;
    *height = style.WindowPadding.y * 2.0f + ImGui::GetFrameHeight();   // the title bar: a frame's height
}

bool retro_ui_section_header(const char* id, const char* label, bool open_at_first)
{
    char imgui_label[300] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###%s", label, id);
    return retro_ui_collapsing_header(imgui_label, open_at_first ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

// retro_ui_end_scroll_area needs to know whether its begin showed the contents.
constexpr int32_t SCROLL_AREA_DEPTH_MAX = 8;
static bool g_scroll_area_visible[SCROLL_AREA_DEPTH_MAX] = {};
static int32_t g_scroll_area_depth = 0;

bool retro_ui_begin_scroll_area(const char* id, float width, float height)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    bool visible = ImGui::BeginChild(id, ImVec2(width, height));
    if (visible)
    {
        // A child has its own draw list: the pixel font needs nearest sampling here too.
        ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest, nullptr);
    }
    if (g_scroll_area_depth < SCROLL_AREA_DEPTH_MAX)
    {
        g_scroll_area_visible[g_scroll_area_depth] = visible;
    }
    g_scroll_area_depth++;
    return visible;
}

void retro_ui_end_scroll_area()
{
    g_scroll_area_depth--;
    if (g_scroll_area_depth >= 0 && g_scroll_area_depth < SCROLL_AREA_DEPTH_MAX &&
        g_scroll_area_visible[g_scroll_area_depth])
    {
        ImGui::GetWindowDrawList()->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear, nullptr);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void retro_ui_begin_group()
{
    ImGui::BeginGroup();
}

void retro_ui_end_group()
{
    ImGui::EndGroup();
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

void retro_ui_text_wrapped_at(const char* text, float wrap_x)
{
    ImGui::PushTextWrapPos(wrap_x);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
}

void retro_ui_dim_text_wrapped_at(const char* text, float wrap_x)
{
    ImGui::PushTextWrapPos(wrap_x);
    ImGui::TextDisabled("%s", text);
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

float retro_ui_line_step()
{
    return ImGui::GetFrameHeightWithSpacing();
}

float retro_ui_text_line_step()
{
    return ImGui::GetTextLineHeightWithSpacing();
}

float retro_ui_item_spacing()
{
    return spacing();
}

float retro_ui_button_width(const char* label)
{
    return button_width(label);
}

float retro_ui_font_size()
{
    return ImGui::GetFontSize();
}

// ---------------------------------------------------------------------------
// Drawing in a window.

// The last canvas' top left corner, in screen pixels.
static float g_canvas_x = 0.0f;
static float g_canvas_y = 0.0f;

void retro_ui_canvas(float width, float height)
{
    ImVec2 origin = ImGui::GetCursorScreenPos();
    g_canvas_x = std::floor(origin.x);
    g_canvas_y = std::floor(origin.y);
    ImGui::Dummy(ImVec2(width, height));
}

void retro_ui_canvas_rect(float x, float y, float width, float height, uint32_t argb)
{
    float left = g_canvas_x + x;
    float top = g_canvas_y + y;
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(left, top), ImVec2(left + width, top + height),
                                              retro_ui_color_imgui(argb));
}

void retro_ui_canvas_outline(float x, float y, float width, float height, uint32_t argb, float thickness)
{
    float left = g_canvas_x + x;
    float top = g_canvas_y + y;
    ImGui::GetWindowDrawList()->AddRect(ImVec2(left, top), ImVec2(left + width, top + height),
                                        retro_ui_color_imgui(argb), 0.0f, 0, thickness);
}

void retro_ui_canvas_image(uint64_t texture, float x, float y, float width, float height, float u_max, float v_max,
                           uint32_t argb)
{
    float left = g_canvas_x + x;
    float top = g_canvas_y + y;
    ImGui::GetWindowDrawList()->AddImage(static_cast<ImTextureID>(texture), ImVec2(left, top),
                                         ImVec2(left + width, top + height), ImVec2(0.0f, 0.0f), ImVec2(u_max, v_max),
                                         retro_ui_color_imgui(argb));
}

// ---------------------------------------------------------------------------
// Keys.

static ImGuiKey imgui_key(RetroUiKey key)
{
    // Digits and letters are in order in both enums.
    int32_t index = static_cast<int32_t>(key);
    int32_t digit_0 = static_cast<int32_t>(RetroUiKey::digit_0);
    int32_t letter_a = static_cast<int32_t>(RetroUiKey::a);
    if (index >= letter_a && index <= static_cast<int32_t>(RetroUiKey::z))
    {
        return static_cast<ImGuiKey>(static_cast<int32_t>(ImGuiKey_A) + index - letter_a);
    }
    if (index >= digit_0 && index <= static_cast<int32_t>(RetroUiKey::digit_9))
    {
        return static_cast<ImGuiKey>(static_cast<int32_t>(ImGuiKey_0) + index - digit_0);
    }
    switch (key)
    {
    case RetroUiKey::escape:
        return ImGuiKey_Escape;
    case RetroUiKey::enter:
        return ImGuiKey_Enter;
    case RetroUiKey::space:
        return ImGuiKey_Space;
    case RetroUiKey::tab:
        return ImGuiKey_Tab;
    case RetroUiKey::backspace:
        return ImGuiKey_Backspace;
    default:
        break;
    }
    return ImGuiKey_None;
}

bool retro_ui_key_pressed(RetroUiKey key, bool repeat)
{
    ImGuiKey imgui = imgui_key(key);
    return imgui != ImGuiKey_None && ImGui::IsKeyPressed(imgui, repeat);
}
