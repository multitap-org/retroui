// The game view widget drawn with ImGui (game_view.h).

#include "game_view.h"

#include "imgui.h"

static void draw(ImDrawList* draw_list, const ShownImage* image)
{
    // The filter applies to this image only: back to ImGui's default after it.
    const ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    bool nearest = image->filter == ShownImageFilter::nearest;
    draw_list->AddCallback(nearest ? platform_io.DrawCallback_SetSamplerNearest : platform_io.DrawCallback_SetSamplerLinear,
                           nullptr);
    draw_list->AddImage(ImTextureRef(static_cast<ImTextureID>(image->texture)), ImVec2(image->x, image->y),
                        ImVec2(image->x + image->width, image->y + image->height), ImVec2(image->u_min, image->v_min),
                        ImVec2(image->u_max, image->v_max));
    draw_list->AddCallback(platform_io.DrawCallback_SetSamplerLinear, nullptr);
}

void game_view_draw(const ShownImage* image)
{
    draw(ImGui::GetBackgroundDrawList(), image);
}

void game_view_draw_in_window(const ShownImage* image)
{
    draw(ImGui::GetWindowDrawList(), image);
}
