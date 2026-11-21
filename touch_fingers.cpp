#include "touch_fingers.h"

#include <SDL3/SDL.h>

// The mouse's finger id on the desktop: one no touch device hands out.
constexpr uint64_t MOUSE_FINGER = UINT64_MAX;

static TouchFinger* find(TouchFingers* fingers, uint64_t id)
{
    for (int32_t i = 0; i < fingers->count; i++)
    {
        if (fingers->fingers[i].id == id && !fingers->fingers[i].lifted)
        {
            return &fingers->fingers[i];
        }
    }
    return nullptr;
}

static void finger_down(TouchFingers* fingers, uint64_t id, float x, float y)
{
    if (fingers->count >= TOUCH_MAX_FINGERS || find(fingers, id) != nullptr)
    {
        return;
    }
    TouchFinger* finger = &fingers->fingers[fingers->count];
    fingers->count++;
    *finger = TouchFinger{};
    finger->id = id;
    finger->x = x;
    finger->y = y;
    finger->began = true;
}

static void finger_move(TouchFingers* fingers, uint64_t id, float x, float y)
{
    TouchFinger* finger = find(fingers, id);
    if (finger != nullptr)
    {
        finger->x = x;
        finger->y = y;
    }
}

static void finger_up(TouchFingers* fingers, uint64_t id, float x, float y)
{
    TouchFinger* finger = find(fingers, id);
    if (finger != nullptr)
    {
        finger->x = x;
        finger->y = y;
        finger->lifted = true;   // dropped after this frame: a quick tap still presses for one frame
    }
}

void touch_fingers_event(TouchFingers* fingers, const SDL_Event* event, float display_width, float display_height)
{
    switch (event->type)
    {
        case SDL_EVENT_FINGER_DOWN:
            finger_down(fingers, event->tfinger.fingerID, event->tfinger.x * display_width, event->tfinger.y * display_height);
            break;
        case SDL_EVENT_FINGER_MOTION:
            finger_move(fingers, event->tfinger.fingerID, event->tfinger.x * display_width, event->tfinger.y * display_height);
            break;
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED:
            finger_up(fingers, event->tfinger.fingerID, event->tfinger.x * display_width, event->tfinger.y * display_height);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.which != SDL_TOUCH_MOUSEID && event->button.button == SDL_BUTTON_LEFT)
            {
                finger_down(fingers, MOUSE_FINGER, event->button.x, event->button.y);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            if (event->motion.which != SDL_TOUCH_MOUSEID)
            {
                finger_move(fingers, MOUSE_FINGER, event->motion.x, event->motion.y);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.which != SDL_TOUCH_MOUSEID && event->button.button == SDL_BUTTON_LEFT)
            {
                finger_up(fingers, MOUSE_FINGER, event->button.x, event->button.y);
            }
            break;
        default:
            break;
    }
}

void touch_fingers_end_frame(TouchFingers* fingers)
{
    int32_t kept = 0;
    for (int32_t i = 0; i < fingers->count; i++)
    {
        if (fingers->fingers[i].lifted)
        {
            continue;
        }
        fingers->fingers[kept] = fingers->fingers[i];
        fingers->fingers[kept].began = false;
        kept++;
    }
    fingers->count = kept;
    fingers->presses = 0;
}

int32_t touch_fingers_down(const TouchFingers* fingers)
{
    int32_t down = 0;
    for (int32_t i = 0; i < fingers->count; i++)
    {
        down += fingers->fingers[i].lifted ? 0 : 1;
    }
    return down;
}
