# retroui: the retro-style UI shared by multitap's frontends (piccolo,
# piccoletto): the 5x5 pixel font, the themes (palettes), themed Dear ImGui
# widgets, the controller picture, the ports view (tabs, DEVICES board) and the
# touch UI with the touch pads (piccoletto's screens; both frontends' pads).
# A library: it doesn't build alone. The frontend adds hosts-common and
# frontends-common first (pad_picture draws frontends-common's input layouts)
# and defines imgui and SDL3::SDL3.

# The font alone (no ImGui, no SDL): tools that only measure or draw text.
add_library(retroui_font STATIC
    pixel_font.cpp
)
target_include_directories(retroui_font PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
multitap_set_project_flags(retroui_font)

multitap_require(retroui imgui SDL3::SDL3 frontend_common)

add_library(retroui STATIC
    pixel_font_imgui.cpp
    retro_ui.cpp
    retro_ui_controls.cpp
    retro_ui_theme.cpp
    shader_check_view.cpp
    pad_picture.cpp
    ports_view.cpp
    port_indicators.cpp
    dvr_view.cpp
    hud_view.cpp
    touch_ui.cpp
    touch_fingers.cpp
    retropad.cpp
    pad_layout.cpp
    pad_layouts.cpp
    pad_editor.cpp
)
target_include_directories(retroui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(retroui PUBLIC retroui_font imgui SDL3::SDL3 frontend_common)
multitap_set_project_flags(retroui)

# Development tool: writes a PNG of every glyph + sample text, to judge the font by eye.
add_executable(pixel_font_preview tools/pixel_font_preview.cpp)
target_link_libraries(pixel_font_preview PRIVATE retroui_font)
multitap_set_project_flags(pixel_font_preview)

if(MULTITAP_BUILD_TESTS)
    # Table well formed, no character defined twice, no two glyphs identical.
    add_test(NAME retroui_pixel_font COMMAND pixel_font_preview --check)
    set_tests_properties(retroui_pixel_font PROPERTIES PASS_REGULAR_EXPRESSION "table OK")

    # Where the touch pad goes around a picture (landscape, below, over a tall one).
    add_executable(retroui_pad_placement_test tests/pad_placement_test.cpp)
    target_link_libraries(retroui_pad_placement_test PRIVATE retroui)
    multitap_set_project_flags(retroui_pad_placement_test)
    add_test(NAME retroui_pad_placement COMMAND retroui_pad_placement_test)

    # Every layout's parts apart and on screen; the d-pad's eight directions.
    add_executable(retroui_pad_layouts_test tests/pad_layouts_test.cpp)
    target_link_libraries(retroui_pad_layouts_test PRIVATE retroui)
    multitap_set_project_flags(retroui_pad_layouts_test)
    add_test(NAME retroui_pad_layouts COMMAND retroui_pad_layouts_test)

    # The block for a window, either way up.
    add_executable(retroui_scale_test tests/scale_test.cpp)
    target_link_libraries(retroui_scale_test PRIVATE retroui)
    multitap_set_project_flags(retroui_scale_test)
    add_test(NAME retroui_scale COMMAND retroui_scale_test)

    # The HUD's stacks: icons top right, cards bottom right, nothing overlapping.
    add_executable(retroui_hud_view_test tests/hud_view_test.cpp)
    target_link_libraries(retroui_hud_view_test PRIVATE retroui)
    multitap_set_project_flags(retroui_hud_view_test)
    add_test(NAME retroui_hud_view COMMAND retroui_hud_view_test)
endif()
