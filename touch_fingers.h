#pragma once

// The fingers on the screen (multi-touch), for what must follow every finger
// at once: the pads (several parts held together, a stick owned by the
// finger that went down on it) and EDIT LAYOUT's pinch. The rest of the UI
// (buttons, lists) goes through ImGui, which only sees one finger (SDL turns
// the first one into the mouse); that's enough there.
//
// Fed with SDL's finger events. On the desktop the mouse is a finger (while
// its left button is down); the mouse events SDL makes from real touches
// are ignored here, those fingers already came as finger events.

#include <cstdint>

union SDL_Event;

constexpr int32_t TOUCH_MAX_FINGERS = 10;

struct TouchFinger
{
    uint64_t    id     = 0;
    float       x      = 0.0f;      // ImGui's display coordinates (window pixels)
    float       y      = 0.0f;
    bool        began  = false;     // went down since the last frame
    bool        lifted = false;     // came up since the last frame (still counts this frame)
    const char* owner  = nullptr;   // the pad part that took it (a stick), by its id; nullptr = free
};

struct TouchFingers
{
    TouchFinger fingers[TOUCH_MAX_FINGERS] = {};
    int32_t     count   = 0;
    int32_t     presses = 0;   // pad parts newly pressed this frame (for VIBRATE ON PRESS)
};

// One SDL event; `display_width` / `display_height` are ImGui's display size
// (finger positions come normalized 0..1).
void touch_fingers_event(TouchFingers* fingers, const SDL_Event* event, float display_width, float display_height);

// After the frame is drawn: drops the lifted fingers, clears `began` and `presses`.
void touch_fingers_end_frame(TouchFingers* fingers);

// The fingers down now (lifted ones not counted).
int32_t touch_fingers_down(const TouchFingers* fingers);
