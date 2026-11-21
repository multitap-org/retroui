# retroui

The retro-style UI shared by multitap's frontends (piccolo, piccoletto):

- `pixel_font`: the 5x5 pixel font (drawn from scratch for piccolo), and
  `pixel_font_imgui`, the same font as a Dear ImGui font, crisp at every
  whole-number scale.
- `retro_ui`: the themes (palettes: BLUE, PURPLE, ZSNES, BLACK) and themed
  ImGui widgets: menu bar, windows with bevels, buttons, lists, combos,
  check boxes, scroll bars, text shadows, the theme cursor.
- `pad_picture`: a controller drawn as blocks, one part lit.
- `ports_view`: who plays on which port: tabs that rise from the bottom left
  when a port changes, and the full-screen DEVICES board; `port_indicators.h`
  the port LEDs and ping bars (sizes, colors, thresholds).
- `tools/pixel_font_preview`: a PNG of every glyph; `--check` is a test.

A library: it doesn't build on its own. A frontend adds it as a submodule
(`deps/common/retroui`) after hosts-common and frontends-common, and defines
`imgui` and `SDL3::SDL3`. Targets: `retroui_font` (the font only, no ImGui)
and `retroui`.

MIT License (see LICENSE).
