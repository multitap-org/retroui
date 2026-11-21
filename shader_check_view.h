#pragma once

// The question when a shader preset just applied is too slow
// (frontends-common's shader_check.h), in both looks:
//
//   window (piccolo, the desktop look):
//   +- SHADER TOO SLOW -----------------------------------+
//   | CRT-ROYALE RUNS AT 41 FPS. THE GAME NEEDS 60.       |
//   | KEEP IT?                                            |
//   |                          [KEEP] [REVERT (15)]       |
//   +-----------------------------------------------------+
//
//   sheet (piccoletto, the touch look): a card at the bottom over the dimmed
//   screen, the same words wrapped to the phone's width, two big buttons.
//
// The game keeps running behind it, so the preset can be seen while deciding.
// REVERT is chosen by itself when the countdown ends (and by the window's
// close button): a preset too slow to use the menu with is undone without help.

#include <cstdint>

struct ImFont;
struct TouchUi;

constexpr uint64_t SHADER_CHECK_ANSWER_MS = 15000;

enum class ShaderCheckChoice
{
    none,     // still open
    keep,
    revert,
};

// The words, in the frontend's language.
struct ShaderCheckText
{
    const char* title    = "SHADER TOO SLOW";
    const char* speed    = "RUNS AT %.0f FPS. THE GAME NEEDS %.0f.";   // measured fps, target fps
    const char* question = "KEEP IT?";
    const char* keep     = "KEEP";
    const char* revert   = "REVERT (%d)";                              // seconds left
};

// `preset`: the preset's name; measured_fps and target_fps for the text;
// asked_ms / now_ms: when it was asked, for REVERT's countdown.
ShaderCheckChoice shader_check_view_draw_window(ImFont* pixel_font, int32_t scale, const ShaderCheckText* text,
                                                const char* preset, double measured_fps, double target_fps,
                                                uint64_t asked_ms, uint64_t now_ms);
ShaderCheckChoice shader_check_view_draw_sheet(const TouchUi* ui, const ShaderCheckText* text, const char* preset,
                                               double measured_fps, double target_fps, uint64_t asked_ms,
                                               uint64_t now_ms);
