#include "shader_check_view.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "imgui.h"

#include "retro_ui.h"
#include "touch_ui.h"

ShaderCheckChoice shader_check_view_draw_window(ImFont* pixel_font, int32_t scale, const ShaderCheckText* text,
                                                const char* preset, double measured_fps, double target_fps,
                                                uint64_t asked_ms, uint64_t now_ms)
{
    ShaderCheckChoice choice = ShaderCheckChoice::none;
    float block = static_cast<float>(scale);
    const ImGuiIO& io = ImGui::GetIO();

    retro_ui_push_theme(pixel_font, scale);
    // 300 retro pixels wide (never wider than the window), as tall as its text.
    float width = 300.0f * block;
    width = width < io.DisplaySize.x - 8.0f * block ? width : io.DisplaySize.x - 8.0f * block;
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowFocus();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
    char title[200] = {};
    std::snprintf(title, sizeof(title), "%s###shader_check", text->title);
    if (ImGui::Begin(title, nullptr, flags))
    {
        retro_ui_begin_window(scale);
        if (retro_ui_close_button())
        {
            choice = ShaderCheckChoice::revert;
        }
        ImGui::PushTextWrapPos(0.0f);
        char speed[200] = {};
        std::snprintf(speed, sizeof(speed), text->speed, measured_fps, target_fps);
        ImGui::Text("%s %s", preset, speed);
        ImGui::TextUnformatted(text->question);
        ImGui::PopTextWrapPos();
        ImGui::Spacing();

        // KEEP and REVERT at the right; REVERT counts down.
        int seconds = retro_ui_seconds_left(now_ms, asked_ms, SHADER_CHECK_ANSWER_MS);
        char revert[128] = {};
        std::snprintf(revert, sizeof(revert), text->revert, seconds);
        float padding = ImGui::GetStyle().FramePadding.x * 2.0f;
        float right = ImGui::CalcTextSize(text->keep).x + ImGui::CalcTextSize(revert).x + padding * 2.0f +
                      ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - right);
        ImGui::PushID("keep");
        bool keep_clicked = retro_ui_button(text->keep);
        ImGui::PopID();
        if (keep_clicked)
        {
            choice = ShaderCheckChoice::keep;
        }
        ImGui::SameLine();
        if (retro_ui_countdown_button(text->revert, seconds, "revert") ||
            (now_ms >= asked_ms && now_ms - asked_ms >= SHADER_CHECK_ANSWER_MS))
        {
            choice = ShaderCheckChoice::revert;
        }
        retro_ui_end_window();
    }
    ImGui::End();
    retro_ui_pop_theme();
    return choice;
}

// ---------------------------------------------------------------------------
// The touch sheet (retro pixels)
// ---------------------------------------------------------------------------

constexpr float    SHEET_MARGIN  = 8.0f;
constexpr float    SHEET_PADDING = 8.0f;
constexpr float    SHEET_MAX     = 300.0f;   // a phone on its side gets a card, not a strip
constexpr float    SHEET_BUTTON  = 24.0f;    // 48 dp
constexpr float    SHEET_GAP     = 4.0f;
constexpr float    LINE_GAP      = 2.0f;
constexpr uint32_t SHEET_DIM     = 0x59000000u;   // over the screen behind (35%, as the quick menu)

// `text` broken at spaces into lines no wider than `width`.
static void wrap(const TouchUi* ui, const std::string* text, float width, std::vector<std::string>* lines)
{
    std::string line;
    size_t start = 0;
    while (start < text->size())
    {
        size_t end = text->find(' ', start);
        end = end == std::string::npos ? text->size() : end;
        std::string word = text->substr(start, end - start);
        start = end + 1;
        std::string longer = line.empty() ? word : line + " " + word;
        if (!line.empty() && touch_ui_text_width(ui, longer.c_str(), TouchText::normal) > width)
        {
            lines->push_back(line);
            line = word;
        }
        else
        {
            line = longer;
        }
    }
    if (!line.empty())
    {
        lines->push_back(line);
    }
}

ShaderCheckChoice shader_check_view_draw_sheet(const TouchUi* ui, const ShaderCheckText* text, const char* preset,
                                               double measured_fps, double target_fps, uint64_t asked_ms,
                                               uint64_t now_ms)
{
    ShaderCheckChoice choice = ShaderCheckChoice::none;
    const RetroUiPalette* palette = &ui->palette;
    float screen_width = touch_ui_width(ui);
    float screen_height = touch_ui_height(ui);
    float width = screen_width - 2.0f * SHEET_MARGIN;
    width = std::floor(width < SHEET_MAX ? width : SHEET_MAX);
    float inner = width - 2.0f * SHEET_PADDING;

    char speed[200] = {};
    std::snprintf(speed, sizeof(speed), text->speed, measured_fps, target_fps);
    std::string words = std::string(preset) + " " + speed + " " + text->question;
    std::vector<std::string> lines;
    wrap(ui, &words, inner, &lines);
    float line = touch_ui_text_height(ui, TouchText::normal) + LINE_GAP;
    float title_height = touch_ui_text_height(ui, TouchText::big) + SHEET_GAP;
    float height = 2.0f * SHEET_PADDING + title_height + static_cast<float>(lines.size()) * line + SHEET_GAP + SHEET_BUTTON;
    float x = std::floor((screen_width - width) * 0.5f);
    float y = std::floor(screen_height - SHEET_MARGIN - height);

    touch_ui_begin_overlay(ui, "##shader_check_sheet");
    touch_ui_rect(ui, 0.0f, 0.0f, screen_width, screen_height, SHEET_DIM);
    touch_ui_rect(ui, x, y, width, height, palette->panel_face);
    float text_x = x + SHEET_PADDING;
    float text_y = y + SHEET_PADDING;
    touch_ui_text(ui, text_x, text_y, text->title, palette->text_highlight, TouchText::big);
    text_y += title_height;
    for (size_t i = 0; i < lines.size(); i++)
    {
        touch_ui_text(ui, text_x, text_y, lines[i].c_str(), palette->text, TouchText::normal);
        text_y += line;
    }
    text_y += SHEET_GAP;

    // KEEP, and REVERT (the default: it happens by itself) counting down.
    float half = std::floor((inner - SHEET_GAP) * 0.5f);
    if (touch_ui_button(ui, "##shader_check_keep", text_x, text_y, half, SHEET_BUTTON, text->keep, TouchButton::normal,
                        TouchText::normal))
    {
        choice = ShaderCheckChoice::keep;
    }
    int seconds = retro_ui_seconds_left(now_ms, asked_ms, SHADER_CHECK_ANSWER_MS);
    char revert[128] = {};
    std::snprintf(revert, sizeof(revert), text->revert, seconds);
    if (touch_ui_button(ui, "##shader_check_revert", text_x + inner - half, text_y, half, SHEET_BUTTON, revert,
                        TouchButton::primary, TouchText::normal) ||
        (now_ms >= asked_ms && now_ms - asked_ms >= SHADER_CHECK_ANSWER_MS))
    {
        choice = ShaderCheckChoice::revert;
    }
    touch_ui_end_screen();
    return choice;
}
