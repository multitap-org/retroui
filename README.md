# retroui

The retro-style UI shared by multitap's frontends (piccolo, piccoletto):

- `pixel_font`: the 5x5 pixel font (drawn from scratch for piccolo), and
  `pixel_font_imgui`, the same font as a Dear ImGui font, crisp at every
  whole-number scale. `retro_ui_font.h` hands the font around without ImGui.
- `retro_ui`: the themes (palettes: BLUE, PURPLE, ZSNES, BLACK) and the
  frame of the UI: menu bar, windows with bevels, the theme cursor, text
  shadows, the scale (`retro_ui_scale_for_window`: the largest whole scale
  that still leaves 640x360). `retro_ui_theme`: the palette as named keys,
  for settings and theme files.
- `retro_ui_controls`: what a window draws, with no ImGui in the signatures
  (plain ids, labels, values, ARGB colors): rows with a label column, a
  marker and an END button; buttons, rows of equal-width buttons, checkboxes,
  number, text and color fields, sliders, choices, lists, tables, questions
  and modals, tabs that split the window's width, collapsible sections, code
  boxes, tiles with a picture, a canvas. A frontend's windows
  draw only with these, so another GUI toolkit would reimplement this file
  and `game_view`.
- `game_view`: puts the game's picture on screen (frontends-common's
  `ShownImage`: a texture over a rectangle).
- `hud_view`: the HUD cards over the game; `dvr_view`: the rewind DVR's
  strip; `shader_check_view`: the question when a shader preset is too slow.
- `pad_picture`: a controller drawn as blocks, one part lit.
- `ports_view`: who plays on which port: tabs that rise from the bottom left
  when a port changes, and the full-screen DEVICES board; `port_indicators.h`
  the port LEDs and ping bars (sizes, colors, thresholds).
- `touch_ui`, `touch_fingers`, `pad_layout(s)`, `pad_editor`, `retropad`: the
  touch UI (piccoletto's screens) and the touch pads of both frontends, with
  EDIT LAYOUT.
- `tools/pixel_font_preview`: a PNG of every glyph; `--check` is a test.

A library: it doesn't build on its own. A frontend adds it as a submodule
(`deps/common/retroui`) after hosts-common and frontends-common, and defines
`imgui` and `SDL3::SDL3`. Targets: `retroui_font` (the font only, no ImGui)
and `retroui`.

MIT License (see LICENSE).
