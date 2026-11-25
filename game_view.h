#pragma once

// The game view widget: puts the picture on screen. The renderer side
// (frontends-common's video_output) runs the core's frame through the shaders
// and screen effects into a texture and says where it goes (a ShownImage);
// this only draws that texture. Another GUI toolkit reimplements these two.

#include "shown_image.h"

// Behind everything else (the UI draws over it).
void game_view_draw(const ShownImage* image);

// Inside the current window, at the image's position in window pixels (a
// picture in a window: piccoletto's stream).
void game_view_draw_in_window(const ShownImage* image);
