// pad_layouts_place: where the touch pad goes around a picture (piccolo's
// on-screen pad and piccoletto's REMOTE PLAY share it).

#include <cstdio>

#include "pad_layouts.h"

static int g_failures = 0;

static void check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        g_failures++;
    }
}

int main()
{
    PadLayouts* layouts = new PadLayouts{};
    pad_layouts_init(layouts);

    // A phone on its side (422 x 195 retro pixels): over the picture, at the bottom.
    PadPlacement side = pad_layouts_place(layouts, 422.0f, 195.0f, 4.0f / 3.0f, 4.0f);
    const PadLayout* landscape = pad_layouts_get(layouts, PadLayoutId::remote_overlay_landscape);
    check(side.id == PadLayoutId::remote_overlay_landscape && !side.below, "landscape: the overlay");
    check(side.frame.y + landscape->height * side.frame.dp_scale <= 195.0f + 0.01f &&
              side.frame.y + landscape->height * side.frame.dp_scale >= 194.0f,
          "landscape: down at the bottom");

    // Upright with a 4:3 picture (195 x 422): the pad fits under it.
    PadPlacement upright = pad_layouts_place(layouts, 195.0f, 422.0f, 4.0f / 3.0f, 12.0f);
    check(upright.id == PadLayoutId::remote_below && upright.below, "portrait 4:3: below the picture");
    check(upright.picture_height == 146.0f, "portrait 4:3: the picture keeps 195 / (4/3), whole pixels");
    check(upright.frame.y >= upright.picture_height + 12.0f, "portrait 4:3: under the picture and the gap");
    check(upright.frame.dp_scale >= PAD_MIN_BELOW_SCALE, "portrait 4:3: big enough");

    // Upright with a picture taller than 2:3 (1:2): too little room, over the picture.
    PadPlacement tall = pad_layouts_place(layouts, 195.0f, 422.0f, 0.5f, 4.0f);
    check(tall.id == PadLayoutId::remote_overlay_portrait && !tall.below, "portrait 1:2: the overlay");
    check(tall.picture_height == 422.0f, "portrait 1:2: the picture keeps the screen");

    // The default alpha follows the setting unless EDIT LAYOUT set one.
    PadLayout* overlay = pad_layouts_get(layouts, PadLayoutId::remote_overlay_portrait);
    pad_layout_set_default_alpha(overlay, 0.3f);
    check(overlay->alpha == 0.3f && overlay->default_alpha == 0.3f, "alpha: follows the setting");
    overlay->alpha = 0.8f;
    overlay->alpha_set = true;
    pad_layout_set_default_alpha(overlay, 0.4f);
    check(overlay->alpha == 0.8f && overlay->default_alpha == 0.4f, "alpha: EDIT LAYOUT's wins");

    delete layouts;
    if (g_failures == 0)
    {
        std::printf("pad placement OK\n");
        return 0;
    }
    return 1;
}
