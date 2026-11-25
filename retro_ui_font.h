#pragma once

// The font retroui draws with (the pixel font), as the frontend holds it: a
// handle it only passes back to retroui. With ImGui it is ImGui's font; a
// retroui on another toolkit changes this line, and the frontend's
// signatures stay the same.

struct ImFont;
typedef ImFont RetroUiFont;
