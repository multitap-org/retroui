// retro_ui_controls.h drawn with ImGui and the retro theme.

#include "retro_ui_controls.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "imgui.h"
#include "imgui_internal.h"   // BringWindowToDisplayFront

#include "pixel_font_imgui.h"
#include "port_indicators.h"
#include "retro_ui.h"

// File state, as ImGui's own (an immediate-mode GUI keeps one current
// window, line and ID stack): it is only touched between a begin and its end
// in one frame, on the UI thread.
//
// Room kept on the right of the controls: the row's end button (for every
// control in the row) and a reserved button (for the next control only).
static float g_row_reserved = 0.0f;
static float g_next_reserved = 0.0f;
// The next control's width set by retro_ui_next_width (0: none).
static float g_next_width = 0.0f;

static float button_width(const char* label)
{
    return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

static float spacing()
{
    return ImGui::GetStyle().ItemSpacing.x;
}

// The width the next control fills, and the reserved button (or the width
// set for it) is used up.
static float take_control_width()
{
    float width = ImGui::GetContentRegionAvail().x - g_row_reserved - g_next_reserved;
    if (g_next_width > 0.0f)
    {
        width = g_next_width;
    }
    g_next_reserved = 0.0f;
    g_next_width = 0.0f;
    return width;
}

void retro_ui_next_width(float width)
{
    g_next_width = width;
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
        bool clip = row->clip_label && row->label_width > 0.0f;
        if (clip)
        {
            ImVec2 corner = ImGui::GetCursorScreenPos();
            ImGui::PushClipRect(corner, ImVec2(corner.x + row->label_width - spacing(), corner.y + ImGui::GetFrameHeight()),
                                true);
        }
        ImGui::TextUnformatted(row->label);
        if (clip)
        {
            ImGui::PopClipRect();
        }
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

bool retro_ui_id_button(const char* id, const char* label)
{
    char imgui_label[300] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###%s", label, id);
    return retro_ui_button(imgui_label);
}

bool retro_ui_move_button(const char* id, bool up)
{
    return ImGui::ArrowButton(id, up ? ImGuiDir_Up : ImGuiDir_Down);
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

void retro_ui_dim_label(const char* text, const char* help)
{
    ImGui::AlignTextToFramePadding();
    retro_ui_dim_text(text, help);
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

float retro_ui_check_width(const char* label)
{
    return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(label).x;
}

bool retro_ui_number_field(const char* id, int32_t* value, int32_t step)
{
    return retro_ui_number_field_steps(id, value, step, 8);
}

bool retro_ui_number_field_steps(const char* id, int32_t* value, int32_t step, int32_t fast_step)
{
    ImGui::PushID(id);
    int number = static_cast<int>(*value);
    ImGui::SetNextItemWidth(take_control_width());
    retro_ui_push_field_text();
    bool changed = ImGui::InputInt("##number", &number, step > 0 ? step : 1, static_cast<int>(fast_step));
    retro_ui_pop_field_text();
    ImGui::PopID();
    *value = static_cast<int32_t>(number);
    return changed;
}

float retro_ui_number_field_width(const char* widest)
{
    // The text with its padding, and the - and + buttons (a control's height
    // square each) with theirs.
    return ImGui::CalcTextSize(widest).x + ImGui::GetFrameHeight() * 2.0f + ImGui::GetStyle().FramePadding.x * 4.0f;
}

bool retro_ui_slider(const char* id, float* value, float min, float max, const char* format)
{
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(take_control_width());
    bool changed = ImGui::SliderFloat("##slider", value, min, max, format);
    ImGui::PopID();
    return changed;
}

bool retro_ui_slider_int(const char* id, int32_t* value, int32_t min, int32_t max, const char* format)
{
    ImGui::PushID(id);
    ImGui::SetNextItemWidth(take_control_width());
    int number = static_cast<int>(*value);
    bool changed = ImGui::SliderInt("##slider", &number, static_cast<int>(min), static_cast<int>(max), format);
    *value = static_cast<int32_t>(number);
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

// The code boxes' field: an edit field's filtering, and its caret kept at
// the end, so a click or an arrow key can't send the next digit into the
// middle while the boxes edge the last one (they show no caret of their own).
static int code_boxes_callback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
    {
        data->CursorPos = data->BufTextLen;
        data->SelectionStart = data->BufTextLen;
        data->SelectionEnd = data->BufTextLen;
        return 0;
    }
    return edit_field_callback(data);
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
    if (field->read_only)
    {
        flags |= ImGuiInputTextFlags_ReadOnly;
    }
    if (field->decimal)
    {
        flags |= ImGuiInputTextFlags_CharsDecimal;
    }
    if (field->uppercase)
    {
        flags |= ImGuiInputTextFlags_CharsUppercase;
    }
    // The options aren't changed through UserData; Dear ImGui only takes a void*.
    void* user_data = const_cast<RetroUiEditField*>(field);
    retro_ui_push_field_text();
    bool entered = false;
    if (field->hint != nullptr)
    {
        entered = ImGui::InputTextWithHint("##edit", field->hint, buffer, buffer_size, flags, edit_field_callback, user_data);
    }
    else
    {
        entered = ImGui::InputText("##edit", buffer, buffer_size, flags, edit_field_callback, user_data);
    }
    retro_ui_pop_field_text();
    ImGui::PopID();
    return entered;
}

// The code boxes' metrics: window pixels per retro pixel, and the bigger
// font's size (one whole step up: the pixel font stays sharp).
static float code_block()
{
    float block = std::floor(ImGui::GetFontSize() / PIXEL_FONT_IMGUI_UNIT);
    return block < 1.0f ? 1.0f : block;
}

static float code_font_size()
{
    return PIXEL_FONT_IMGUI_UNIT * (code_block() + 1.0f);
}

float retro_ui_code_boxes_height()
{
    return code_font_size() + code_block() * 6.0f;
}

bool retro_ui_code_boxes(const char* id, char* digits, size_t digits_size, const RetroUiCodeBoxes* boxes)
{
    uint32_t groups = boxes->groups > 0 ? boxes->groups : 1;
    uint32_t group_digits = boxes->group_digits > 0 ? boxes->group_digits : 1;
    uint32_t total = groups * group_digits;
    float block = code_block();
    float big = code_font_size();
    float width = take_control_width();
    float height = retro_ui_code_boxes_height();
    ImGui::PushID(id);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    bool entered = false;
    bool typing = false;
    if (boxes->editable)
    {
        // One field over all the boxes, invisible (its frame, text and caret):
        // it takes the keys, the clicks and the gamepad's focus (and the
        // touch keyboard), and the boxes drawn over it show what it holds.
        if (boxes->focus)
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImVec4 clear = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, clear);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, clear);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, clear);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, clear);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, clear);
        ImGui::PushStyleColor(ImGuiCol_InputTextCursor, clear);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(ImGui::GetStyle().FramePadding.x, std::floor((height - ImGui::GetFontSize()) * 0.5f)));
        ImGui::SetNextItemWidth(width);
        RetroUiEditField field = {};
        field.digits_only = true;
        field.max_characters = total;
        void* user_data = &field;
        entered = ImGui::InputText("##code", digits, digits_size,
                                   ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackEdit |
                                       ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackAlways,
                                   code_boxes_callback, user_data);
        typing = ImGui::IsItemActive();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(6);
    }
    else
    {
        ImGui::Dummy(ImVec2(width, height));
    }
    // The boxes, over it. GetColorU32 takes the style's alpha: greyed out
    // inside retro_ui_begin_disabled, like the controls around.
    const RetroUiPalette* palette = retro_ui_palette();
    ImU32 face = ImGui::GetColorU32(retro_ui_color_vec4(palette->field_face));
    ImU32 edge = ImGui::GetColorU32(retro_ui_color_vec4(palette->highlight));
    ImU32 edge_typing = ImGui::GetColorU32(retro_ui_color_vec4(palette->text_highlight));
    ImU32 text = ImGui::GetColorU32(retro_ui_color_vec4(palette->field_text));
    ImU32 missing = ImGui::GetColorU32(retro_ui_color_vec4(palette->text_disabled));
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    float gap = spacing();
    float column = std::floor((width - gap * static_cast<float>(groups - 1)) / static_cast<float>(groups));
    float digit_width = font->CalcTextSizeA(big, 1.0e6f, 0.0f, "0").x;
    float pitch = digit_width + block * 2.0f;   // the digits a little apart, "_" too
    float group_width = pitch * static_cast<float>(group_digits) - block * 2.0f;
    size_t length = std::strlen(digits);
    uint32_t next = length < total ? static_cast<uint32_t>(length) : total - 1;   // where typing goes
    for (uint32_t g = 0; g < groups; g++)
    {
        float x = origin.x + (column + gap) * static_cast<float>(g);
        float this_width = g + 1 < groups ? column : width - (column + gap) * static_cast<float>(groups - 1);
        ImVec2 min = ImVec2(x, origin.y);
        ImVec2 max = ImVec2(x + this_width, origin.y + height);
        draw_list->AddRectFilled(min, max, face);
        bool here = typing && next / group_digits == g;
        float thickness = here ? block * 2.0f : block;
        ImU32 outline = here ? edge_typing : edge;
        draw_list->AddRectFilled(min, ImVec2(max.x, min.y + thickness), outline);
        draw_list->AddRectFilled(ImVec2(min.x, max.y - thickness), max, outline);
        draw_list->AddRectFilled(min, ImVec2(min.x + thickness, max.y), outline);
        draw_list->AddRectFilled(ImVec2(max.x - thickness, min.y), max, outline);
        float digit_x = std::floor(x + (this_width - group_width) * 0.5f);
        float digit_y = std::floor(origin.y + (height - big) * 0.5f);
        for (uint32_t d = 0; d < group_digits; d++)
        {
            size_t index = static_cast<size_t>(g * group_digits + d);
            char glyph[2] = {index < length ? digits[index] : '_', '\0'};
            draw_list->AddText(font, big, ImVec2(digit_x + pitch * static_cast<float>(d), digit_y),
                               index < length ? text : missing, glyph);
        }
    }
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

bool retro_ui_choice_item_repick(uint32_t index, const char* label, bool selected)
{
    ImGui::PushID(static_cast<int>(index));
    bool picked = ImGui::Selectable(label, selected);
    ImGui::PopID();
    return picked;
}

void retro_ui_end_choice()
{
    ImGui::EndCombo();
    ImGui::PopID();
}

int32_t retro_ui_equal_buttons(const RetroUiRowButton* buttons, uint32_t count, float width)
{
    if (count == 0)
    {
        return -1;
    }
    float row = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
    float gaps = spacing() * static_cast<float>(count - 1);
    // Whole pixels; the last column takes what rounding left, so the row
    // ends exactly at `width` (rows of other counts end there too).
    float column = std::floor((row - gaps) / static_cast<float>(count));
    int32_t pressed = -1;
    for (uint32_t i = 0; i < count; i++)
    {
        if (i > 0)
        {
            ImGui::SameLine();
        }
        float this_width = i + 1 < count ? column : row - gaps - column * static_cast<float>(count - 1);
        retro_ui_begin_disabled(!buttons[i].enabled);
        char imgui_label[300] = {};
        if (buttons[i].id != nullptr)
        {
            std::snprintf(imgui_label, sizeof(imgui_label), "%s###%s", buttons[i].label, buttons[i].id);
        }
        else
        {
            // No id: the column number keeps it apart from its neighbours.
            std::snprintf(imgui_label, sizeof(imgui_label), "%s###equal_%u", buttons[i].label, i);
        }
        if (retro_ui_button_sized(imgui_label, this_width, 0.0f))
        {
            pressed = static_cast<int32_t>(i);
        }
        if (buttons[i].help != nullptr)
        {
            retro_ui_disabled_item_tooltip(buttons[i].help);
        }
        retro_ui_end_disabled();
    }
    return pressed;
}

float retro_ui_equal_buttons_width(const RetroUiRowButton* buttons, uint32_t count, uint32_t columns)
{
    float widest = 0.0f;
    for (uint32_t i = 0; i < count; i++)
    {
        float width = button_width(buttons[i].label);
        widest = width > widest ? width : widest;
    }
    return columns > 0 ? widest * static_cast<float>(columns) + spacing() * static_cast<float>(columns - 1) : 0.0f;
}

// A tile's picture and label, over the button just drawn (its label hidden).
static void draw_tile_picture(const RetroUiTile* tile, const char* label)
{
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    const ImGuiStyle& style = ImGui::GetStyle();
    float pad = std::floor(style.FramePadding.y);
    float left = std::floor(min.x) + pad;
    float top = std::floor(min.y) + pad;
    float area_width = std::floor(max.x) - pad - left;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(ImVec2(left, top), ImVec2(left + area_width, top + tile->picture_height),
                             retro_ui_color_imgui(0xFF000000u));
    bool disabled = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    const RetroUiPalette* palette = retro_ui_palette();
    if (tile->texture != 0)
    {
        float x = std::floor(left + (area_width - tile->image_width) * 0.5f);
        float y = std::floor(top + (tile->picture_height - tile->image_height) * 0.5f);
        // Bilinear for this picture only, then back to the window's nearest
        // (the pixel font's).
        const ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
        draw_list->AddImage(static_cast<ImTextureID>(tile->texture), ImVec2(x, y),
                            ImVec2(x + tile->image_width, y + tile->image_height), ImVec2(0.0f, 0.0f),
                            ImVec2(tile->u_max, tile->v_max));
        draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);
    }
    else if (tile->empty != nullptr)
    {
        ImVec2 text = ImGui::CalcTextSize(tile->empty);
        draw_list->AddText(ImVec2(std::floor(left + (area_width - text.x) * 0.5f),
                                  std::floor(top + (tile->picture_height - text.y) * 0.5f)),
                           retro_ui_color_imgui(palette->text_disabled), tile->empty);
    }
    // The label in the room left under the picture, centered.
    const char* end = ImGui::FindRenderedTextEnd(label);
    ImVec2 text = ImGui::CalcTextSize(label, end);
    float below = top + tile->picture_height;
    float x = std::floor(min.x + (max.x - min.x - text.x) * 0.5f);
    float y = std::floor(below + (max.y - pad - below - text.y) * 0.5f);
    draw_list->AddText(ImVec2(x, y), retro_ui_color_imgui(disabled ? palette->text_disabled : palette->button_text),
                       label, end);
}

bool retro_ui_tile(const char* id, const char* label, const RetroUiTile* tile)
{
    ImGui::PushID(id);
    if (tile->face != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, retro_ui_color_vec4(tile->face));
    }
    if (tile->focus)
    {
        ImGui::SetKeyboardFocusHere();
    }
    bool chosen = false;
    if (tile->picture_height > 0.0f)
    {
        // The button without its label: the picture and the label go over it.
        chosen = retro_ui_button_sized("##tile", tile->width, tile->height) || ImGui::IsItemFocused();
        draw_tile_picture(tile, label);
    }
    else
    {
        chosen = retro_ui_button_sized(label, tile->width, tile->height) || ImGui::IsItemFocused();
    }
    if (tile->chosen && tile->edge > 0.0f)
    {
        // The edge last, over the face and the picture (a frame border
        // would be painted over by them), in the theme's light color.
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        float edge = std::floor(tile->edge) > 1.0f ? std::floor(tile->edge) : 1.0f;
        ImU32 color = ImGui::GetColorU32(retro_ui_color_vec4(retro_ui_palette()->button_light));
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(min, ImVec2(max.x, min.y + edge), color);
        draw_list->AddRectFilled(ImVec2(min.x, max.y - edge), max, color);
        draw_list->AddRectFilled(min, ImVec2(min.x + edge, max.y), color);
        draw_list->AddRectFilled(ImVec2(max.x - edge, min.y), max, color);
    }
    if (tile->face != 0)
    {
        ImGui::PopStyleColor();
    }
    ImGui::PopID();
    return chosen;
}

// retro_ui_end_table pops the padding retro_ui_begin_table pushed (only when it's open).
bool retro_ui_begin_table(const char* id, const RetroUiColumn* columns, uint32_t count, float cell_padding)
{
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(cell_padding, ImGui::GetStyle().CellPadding.y));
    if (!ImGui::BeginTable(id, static_cast<int>(count), ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::PopStyleVar();
        return false;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        if (columns[i].width > 0.0f)
        {
            ImGui::TableSetupColumn(columns[i].label, ImGuiTableColumnFlags_WidthFixed, columns[i].width);
        }
        else
        {
            ImGui::TableSetupColumn(columns[i].label, ImGuiTableColumnFlags_WidthStretch);
        }
    }
    ImGui::TableHeadersRow();
    return true;
}

bool retro_ui_table_row(uint32_t index, const char* label, bool selected)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(static_cast<int>(index));
    char imgui_label[512] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###row", label);
    bool clicked = ImGui::Selectable(imgui_label, selected, ImGuiSelectableFlags_SpanAllColumns);
    ImGui::PopID();
    return clicked;
}

void retro_ui_table_cell(const char* text)
{
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(text);
}

void retro_ui_end_table()
{
    ImGui::EndTable();
    ImGui::PopStyleVar();
}

bool retro_ui_begin_modal(const RetroUiModal* modal, bool open)
{
    char title[300] = {};
    std::snprintf(title, sizeof(title), "%s###%s", modal->title != nullptr ? modal->title : "", modal->id);
    if (open && !ImGui::IsPopupOpen(title))
    {
        ImGui::OpenPopup(title);
    }
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(modal->width, modal->height), ImGuiCond_Appearing);
    return ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);
}

void retro_ui_close_modal()
{
    ImGui::CloseCurrentPopup();
}

void retro_ui_end_modal()
{
    ImGui::EndPopup();
}

RetroUiAnswer retro_ui_question(const RetroUiQuestion* question, bool asking)
{
    RetroUiModal modal = {};
    modal.id = question->id;
    modal.title = question->title;
    modal.width = question->width;
    RetroUiAnswer answer = RetroUiAnswer::none;
    if (retro_ui_begin_modal(&modal, asking))
    {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(question->text);
        ImGui::PopTextWrapPos();
        ImGui::Spacing();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))   // (B on a gamepad sends Escape while a popup is up)
        {
            answer = RetroUiAnswer::no;
        }
        if (retro_ui_button(question->yes))
        {
            answer = RetroUiAnswer::yes;
        }
        ImGui::SameLine();
        if (retro_ui_button(question->no))
        {
            answer = RetroUiAnswer::no;
        }
        if (answer != RetroUiAnswer::none)
        {
            retro_ui_close_modal();
        }
        retro_ui_end_modal();
    }
    return answer;
}

bool retro_ui_choice_opened()
{
    return ImGui::IsWindowAppearing();   // the current window is the choice's list
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

bool retro_ui_list_item_detail(uint32_t index, const char* label, bool selected, const char* detail, bool dim)
{
    ImGui::PushID(static_cast<int>(index));
    if (dim)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    }
    float row_x = ImGui::GetCursorPosX();
    char imgui_label[512] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###item", label);
    bool clicked = ImGui::Selectable(imgui_label, selected);
    // The cursor is back at the row's start, on the next line: the width
    // left from there reaches the contents' right edge.
    ImGui::SameLine(row_x + ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(detail).x);
    ImGui::TextUnformatted(detail);
    if (dim)
    {
        ImGui::PopStyleColor();
    }
    ImGui::PopID();
    return clicked;
}

float retro_ui_list_height(uint32_t rows)
{
    return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
}

RetroUiClick retro_ui_list_item_click(uint32_t index, const char* label, bool selected)
{
    ImGui::PushID(static_cast<int>(index));
    char imgui_label[512] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###item", label);
    RetroUiClick click = RetroUiClick::none;
    if (ImGui::Selectable(imgui_label, selected, ImGuiSelectableFlags_AllowDoubleClick))
    {
        click = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ? RetroUiClick::twice : RetroUiClick::once;
    }
    ImGui::PopID();
    return click;
}

// The clipped list being drawn (one at a time).
static ImGuiListClipper g_clipper;

void retro_ui_begin_clipped(uint32_t count)
{
    g_clipper.Begin(static_cast<int>(count));
}

bool retro_ui_clipped_step(uint32_t* first, uint32_t* end)
{
    if (!g_clipper.Step())
    {
        return false;   // Step ended the clipper
    }
    *first = static_cast<uint32_t>(g_clipper.DisplayStart);
    *end = static_cast<uint32_t>(g_clipper.DisplayEnd);
    return true;
}

int32_t retro_ui_end_buttons(const RetroUiEndButton* buttons, uint32_t count, bool same_line)
{
    float width = 0.0f;
    bool first = true;
    for (uint32_t i = 0; i < count; i++)
    {
        if (buttons[i].shown)
        {
            width += button_width(buttons[i].label) + (first ? 0.0f : spacing());
            first = false;
        }
    }
    float x = ImGui::GetContentRegionMax().x - width;
    if (same_line)
    {
        ImGui::SameLine(x);
    }
    else
    {
        ImGui::SetCursorPosX(x);
    }
    int32_t pressed = -1;
    first = true;
    for (uint32_t i = 0; i < count; i++)
    {
        if (!buttons[i].shown)
        {
            continue;
        }
        if (!first)
        {
            ImGui::SameLine();
        }
        first = false;
        retro_ui_begin_disabled(!buttons[i].enabled);
        ImGui::PushID(buttons[i].id);
        if (retro_ui_button(buttons[i].label))
        {
            pressed = static_cast<int32_t>(i);
        }
        ImGui::PopID();
        retro_ui_end_disabled();
    }
    return pressed;
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
    if (!window->scrolls)
    {
        flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
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
        if (window->focus_on_open && ImGui::IsWindowAppearing())
        {
            ImGui::SetWindowFocus();
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

bool retro_ui_window_appearing()
{
    return ImGui::IsWindowAppearing();
}

bool retro_ui_window_focused()
{
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
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

bool retro_ui_section_header_open(const char* id, const char* label, bool open)
{
    ImGui::SetNextItemOpen(open, ImGuiCond_Always);
    return retro_ui_section_header(id, label, false);
}

// Tabs split the bar's full width equally (Andrés, testing round 1). ImGui
// lays a bar out on its first tab, before the others are submitted, so the
// count is the one the bar had on its last frame: the tabs submitted then
// (ImGui drops the others on that layout). A bar's first frame has no count
// and its tabs take their labels' width for that one frame. Bars nest (a
// tab with tabs of its own): one entry per open bar.
constexpr int32_t TAB_BAR_DEPTH_MAX = 8;
struct TabBarLayout
{
    float    width = 0.0f;   // the bar's width, at its begin
    uint32_t count = 0;      // its tabs last frame (0: unknown)
    uint32_t next = 0;       // the next tab's index this frame
};
static TabBarLayout g_tab_bars[TAB_BAR_DEPTH_MAX] = {};
static int32_t g_tab_bar_depth = 0;

bool retro_ui_begin_tabs(const char* id)
{
    float width = ImGui::GetContentRegionAvail().x;
    if (!ImGui::BeginTabBar(id))
    {
        return false;
    }
    if (g_tab_bar_depth < TAB_BAR_DEPTH_MAX)
    {
        TabBarLayout* layout = &g_tab_bars[g_tab_bar_depth];
        ImGuiTabBar* bar = ImGui::GetCurrentTabBar();
        layout->width = width;
        layout->count = 0;
        layout->next = 0;
        for (int i = 0; bar != nullptr && i < bar->Tabs.Size; i++)
        {
            if (bar->Tabs[i].LastFrameVisible >= bar->PrevFrameVisible && bar->PrevFrameVisible >= 0)
            {
                layout->count++;
            }
        }
    }
    g_tab_bar_depth++;
    return true;
}

void retro_ui_end_tabs()
{
    ImGui::EndTabBar();
    g_tab_bar_depth--;
}

bool retro_ui_begin_tab(const char* id, const char* label, bool select)
{
    char imgui_label[300] = {};
    std::snprintf(imgui_label, sizeof(imgui_label), "%s###%s", label, id);
    if (g_tab_bar_depth > 0 && g_tab_bar_depth <= TAB_BAR_DEPTH_MAX)
    {
        TabBarLayout* layout = &g_tab_bars[g_tab_bar_depth - 1];
        uint32_t index = layout->next;
        layout->next++;
        if (layout->count > 0 && index < layout->count)
        {
            // Whole pixels; the last tab takes what rounding left, so the bar
            // ends exactly at its right edge.
            float gaps = ImGui::GetStyle().ItemInnerSpacing.x * static_cast<float>(layout->count - 1);
            float column = std::floor((layout->width - gaps) / static_cast<float>(layout->count));
            float this_width = index + 1 < layout->count
                                   ? column
                                   : layout->width - gaps - column * static_cast<float>(layout->count - 1);
            ImGui::SetNextItemWidth(this_width > 1.0f ? this_width : 1.0f);
        }
    }
    return ImGui::BeginTabItem(imgui_label, nullptr, select ? ImGuiTabItemFlags_SetSelected : 0);
}

void retro_ui_end_tab()
{
    ImGui::EndTabItem();
}

// retro_ui_end_scroll_area needs to know whether its begin showed the contents.
constexpr int32_t SCROLL_AREA_DEPTH_MAX = 8;
static bool g_scroll_area_visible[SCROLL_AREA_DEPTH_MAX] = {};
static int32_t g_scroll_area_depth = 0;

// Both push the child's background color (ARGB; 0: none); retro_ui_end_scroll_area pops it.
static bool begin_scroll_area(const char* id, float width, float height, uint32_t background)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, retro_ui_color_vec4(background));
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

bool retro_ui_begin_scroll_area(const char* id, float width, float height)
{
    return begin_scroll_area(id, width, height, 0x00000000u);
}

bool retro_ui_begin_scroll_field(const char* id, float width, float height)
{
    return begin_scroll_area(id, width, height, retro_ui_palette()->field_face);
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

void retro_ui_label_colored(uint32_t argb, const char* text)
{
    ImGui::AlignTextToFramePadding();
    retro_ui_text_colored(argb, text);
}

void retro_ui_text_fit(const char* text, float width)
{
    if (ImGui::CalcTextSize(text).x <= width)
    {
        ImGui::TextUnformatted(text);
        return;
    }
    const char* ellipsis = "\xE2\x80\xA6";
    std::string cut = text;
    while (!cut.empty() && ImGui::CalcTextSize((cut + ellipsis).c_str()).x > width)
    {
        cut.pop_back();
        while (!cut.empty() && (static_cast<unsigned char>(cut.back()) & 0xC0u) == 0x80u)
        {
            cut.pop_back();   // the rest of a UTF-8 character
        }
    }
    ImGui::TextUnformatted((cut + ellipsis).c_str());
    retro_ui_item_tooltip(text);
}

void retro_ui_label_fit(const char* text, float width)
{
    ImGui::AlignTextToFramePadding();
    retro_ui_text_fit(text, width);
}

void retro_ui_same_line()
{
    ImGui::SameLine();
}

void retro_ui_same_line_at(float x)
{
    ImGui::SameLine(x);
}

void retro_ui_same_line_gap(float gap)
{
    ImGui::SameLine(0.0f, gap);
}

void retro_ui_set_cursor_x(float x)
{
    ImGui::SetCursorPosX(x);
}

void retro_ui_down_to(float y)
{
    if (ImGui::GetCursorPosY() < y)
    {
        ImGui::SetCursorPosY(y);
    }
}

void retro_ui_space(float width, float height)
{
    ImGui::Dummy(ImVec2(width, height));
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

void retro_ui_indent_by(float width)
{
    ImGui::Indent(width);
}

void retro_ui_unindent_by(float width)
{
    ImGui::Unindent(width);
}

void retro_ui_scroll_here()
{
    ImGui::SetScrollHereY(0.5f);
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

void retro_ui_disabled_item_tooltip(const char* text)
{
    // A disabled item needs AllowWhenDisabled for its tooltip.
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        retro_ui_tooltip(text);
    }
}

bool retro_ui_item_pointed()
{
    return ImGui::IsItemHovered() || (ImGui::IsItemFocused() && ImGui::GetIO().NavVisible);
}

bool retro_ui_item_clicked()
{
    return ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemActivated();
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

float retro_ui_height_left()
{
    return ImGui::GetContentRegionAvail().y;
}

float retro_ui_cursor_y()
{
    return ImGui::GetCursorPosY();
}

float retro_ui_cursor_x()
{
    return ImGui::GetCursorPosX();
}

float retro_ui_right_edge()
{
    return ImGui::GetWindowContentRegionMax().x;
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

float retro_ui_bottom_line_y()
{
    return ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y - ImGui::GetFrameHeight();
}

void retro_ui_window_size(float* width, float* height)
{
    ImVec2 size = ImGui::GetWindowSize();
    *width = size.x;
    *height = size.y;
}

double retro_ui_seconds()
{
    return ImGui::GetTime();
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

void retro_ui_canvas_image_smooth(uint64_t texture, float x, float y, float width, float height, float u_max,
                                  float v_max, uint32_t argb)
{
    // Bilinear for this image only, then back to the window's nearest (the
    // pixel font's: retro_ui_begin_window).
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
    retro_ui_canvas_image(texture, x, y, width, height, u_max, v_max, argb);
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerNearest, nullptr);
}

void retro_ui_canvas_text(float x, float y, const char* text, uint32_t argb)
{
    ImGui::GetWindowDrawList()->AddText(ImVec2(g_canvas_x + x, g_canvas_y + y), retro_ui_color_imgui(argb), text);
}

void retro_ui_window_rect(float x, float y, float width, float height, uint32_t argb)
{
    ImVec2 window = ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(window.x + x, window.y + y),
                                              ImVec2(window.x + x + width, window.y + y + height),
                                              retro_ui_color_imgui(argb));
}

void retro_ui_port_leds(uint32_t count, int32_t lit, float block)
{
    ImVec2 at = ImGui::GetCursorScreenPos();
    float line = ImGui::GetTextLineHeight();
    float top = at.y + std::floor((line - PORT_LED_SIZE * block) * 0.5f);
    float width = port_indicators_draw_leds(at.x, top, block, count, lit, retro_ui_palette()->panel_dark);
    ImGui::Dummy(ImVec2(width, line));
}

void retro_ui_ping_bars(bool known, uint32_t ping_ms, float block)
{
    ImVec2 at = ImGui::GetCursorScreenPos();
    port_indicators_draw_ping(at.x, at.y + ImGui::GetFrameHeight() - block * 2.0f, block, known, ping_ms,
                              retro_ui_palette()->panel_dark);
    ImGui::Dummy(ImVec2(port_indicators_ping_width(block), ImGui::GetFrameHeight()));
}

void retro_ui_hold_ring(float progress, uint32_t argb)
{
    char ring[4] = {};
    pixel_font_imgui_spinner(progress, ring);   // a glyph of the pixel font
    if (ring[0] != '\0')
    {
        ImGui::SameLine();
        ImGui::TextColored(retro_ui_color_vec4(argb), "%s", ring);
    }
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
