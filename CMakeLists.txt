# retroui: the retro-style UI shared by multitap's frontends (piccolo,
# piccoletto): the 5x5 pixel font, the themes (palettes), themed Dear ImGui
# widgets and the controller picture.
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
    pad_picture.cpp
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
endif()
