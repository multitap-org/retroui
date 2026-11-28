// continue_view_picture_size / _card_picture_size / _cover_picture_size: the CONTINUE
// card's picture at the aspect the game was played at, inside the room it has.

#include <cstdint>
#include <cmath>
#include <cstdio>

#include "continue_view.h"

static int g_failures = 0;

static void check_size(uint32_t aspect_width, uint32_t aspect_height, float max_width, float max_height,
                       float expected_width, float expected_height, const char* what)
{
    float width = 0.0f;
    float height = 0.0f;
    continue_view_picture_size(aspect_width, aspect_height, max_width, max_height, &width, &height);
    if (width != expected_width || height != expected_height)
    {
        std::printf("FAILED: %s: %u:%u in %.0f x %.0f gave %.0f x %.0f, expected %.0f x %.0f\n", what, aspect_width,
                    aspect_height, static_cast<double>(max_width), static_cast<double>(max_height),
                    static_cast<double>(width), static_cast<double>(height), static_cast<double>(expected_width),
                    static_cast<double>(expected_height));
        g_failures++;
    }
}

static void check_card(uint32_t aspect_width, uint32_t aspect_height, float card_width, float max_height,
                       float expected_width, float expected_height, const char* what)
{
    ContinueViewCard card = {};
    card.aspect_width = aspect_width;
    card.aspect_height = aspect_height;
    card.picture_max_height = max_height;
    float width = 0.0f;
    float height = 0.0f;
    continue_view_card_picture_size(&card, card_width, &width, &height);
    float dw = width - expected_width;
    float dh = height - expected_height;
    if (dw < -0.01f || dw > 0.01f || dh < -0.01f || dh > 0.01f)
    {
        std::printf("FAILED: %s: %u:%u on a %.2f card (max %.2f) gave %.2f x %.2f, expected %.2f x %.2f\n", what,
                    aspect_width, aspect_height, static_cast<double>(card_width), static_cast<double>(max_height),
                    static_cast<double>(width), static_cast<double>(height), static_cast<double>(expected_width),
                    static_cast<double>(expected_height));
        g_failures++;
    }
}

static void check_cover(uint32_t aspect_width, uint32_t aspect_height, float max_width, float max_height,
                        float expected_width, float expected_height, const char* what)
{
    ContinueViewCard card = {};
    card.aspect_width = aspect_width;
    card.aspect_height = aspect_height;
    float width = 0.0f;
    float height = 0.0f;
    continue_view_cover_picture_size(&card, max_width, max_height, &width, &height);
    float dw = width - expected_width;
    float dh = height - expected_height;
    if (dw < -0.01f || dw > 0.01f || dh < -0.01f || dh > 0.01f)
    {
        std::printf("FAILED: %s: %u:%u in %.2f x %.2f gave %.2f x %.2f, expected %.2f x %.2f\n", what, aspect_width,
                    aspect_height, static_cast<double>(max_width), static_cast<double>(max_height),
                    static_cast<double>(width), static_cast<double>(height), static_cast<double>(expected_width),
                    static_cast<double>(expected_height));
        g_failures++;
    }
}

int main()
{
    // The panel's room: 160 x 108 retro pixels.
    check_size(4, 3, 160.0f, 108.0f, 144.0f, 108.0f, "4:3 fills the height");
    check_size(292, 224, 160.0f, 108.0f, 141.0f, 108.0f, "SNES at 8:7 pixels, as recorded (292 x 224)");
    check_size(160, 144, 160.0f, 108.0f, 120.0f, 108.0f, "Game Boy 10:9");
    check_size(16, 9, 160.0f, 108.0f, 160.0f, 90.0f, "16:9 is too wide: full width, shorter");
    check_size(240, 160, 160.0f, 108.0f, 160.0f, 107.0f, "GBA 3:2: full width, rounded height");
    check_size(3, 4, 160.0f, 108.0f, 81.0f, 108.0f, "a tall picture (vertical shooter)");
    check_size(0, 0, 160.0f, 108.0f, 144.0f, 108.0f, "unknown aspect: 4:3");
    check_size(320, 0, 160.0f, 108.0f, 144.0f, 108.0f, "half an aspect: 4:3");
    // At scale 3 (window pixels): the same shape, three times bigger.
    check_size(4, 3, 480.0f, 324.0f, 432.0f, 324.0f, "4:3 at scale 3");
    // The phone's card: the card's whole width, any size.
    check_card(160, 144, 171.0f, 0.0f, 171.0f, 153.9f, "phone Game Boy: the card's width, a fractional height");
    check_card(4, 3, 171.0f, 0.0f, 171.0f, 128.25f, "phone 4:3");
    check_card(0, 0, 120.0f, 0.0f, 120.0f, 90.0f, "phone, unknown aspect: 4:3");
    check_card(4, 3, 171.0f, 200.0f, 171.0f, 128.25f, "a cap taller than the picture changes nothing");
    check_card(4, 3, 171.0f, 60.0f, 80.0f, 60.0f, "sideways, capped: shorter and narrower");
    check_card(256, 384, 180.0f, 0.0f, 120.0f, 180.0f, "DS (2:3): no taller than the card is wide, centered");
    check_card(1, 1, 150.0f, 0.0f, 150.0f, 150.0f, "square: just fits");

    // The phone on its side: the picture as big as the room.
    check_cover(160, 144, 300.0f, 200.0f, 222.22f, 200.0f, "Game Boy: the room's height");
    check_cover(256, 384, 300.0f, 200.0f, 133.33f, 200.0f, "DS: the room's height, narrow");
    check_cover(16, 9, 300.0f, 200.0f, 300.0f, 168.75f, "16:9 wider than the room: its width, shorter");
    check_cover(0, 0, 300.0f, 201.0f, 268.0f, 201.0f, "unknown: 4:3");
    check_size(4, 3, 0.5f, 0.5f, 1.0f, 1.0f, "never below one pixel");

    // The panel's sizes come from the room (fill the screen).
    struct GeometryCase
    {
        int32_t scale;
        float top;
        float screen_width;
        float screen_height;
        ContinueViewLayout layout;
        float width;            // all window pixels
        float height;
        float picture_width;    // the 4:3 box
        float picture_height;
        float tile_width;
        float tile_height;
        const char* what;
    };
    const GeometryCase geometries[] = {
        {2, 26.0f, 1280.0f, 720.0f, ContinueViewLayout::side_shelf, 1152.0f, 624.0f, 402.0f, 302.0f, 268.0f, 200.0f,
         "720p: 576 x 312 retro px, picture 201 x 151, tiles 134 x 100"},
        {3, 39.0f, 1920.0f, 1080.0f, ContinueViewLayout::side_shelf, 1728.0f, 936.0f, 603.0f, 453.0f, 402.0f, 300.0f,
         "1080p: the same retro sizes, x3"},
        {1, 13.0f, 854.0f, 480.0f, ContinueViewLayout::side_list, 768.0f, 420.0f, 418.0f, 314.0f, 96.0f, 72.0f,
         "480p: the picture with the words under it, the list with small pictures"},
        {3, 39.0f, 1080.0f, 1920.0f, ContinueViewLayout::stacked_shelf, 972.0f, 1305.0f, 924.0f, 693.0f, 213.0f, 159.0f,
         "a tall screen: stacked, the picture across"},
    };
    for (size_t i = 0; i < sizeof(geometries) / sizeof(geometries[0]); i++)
    {
        const GeometryCase* c = &geometries[i];
        ContinueViewPanelGeometry g;
        continue_view_panel_geometry(c->scale, true, c->top, c->screen_width, c->screen_height, &g);
        if (g.layout != c->layout || g.width != c->width || g.height != c->height ||
            g.picture_width != c->picture_width || g.picture_height != c->picture_height ||
            g.tile_width != c->tile_width || g.tile_height != c->tile_height)
        {
            std::printf("FAILED: geometry: %s: %.0f x %.0f, picture %.0f x %.0f, tile %.0f x %.0f\n", c->what,
                        static_cast<double>(g.width), static_cast<double>(g.height),
                        static_cast<double>(g.picture_width), static_cast<double>(g.picture_height),
                        static_cast<double>(g.tile_width), static_cast<double>(g.tile_height));
            g_failures++;
        }
    }
    {
        // A vertical game in the 720p card's box: its height, narrower.
        ContinueViewPanelGeometry g;
        continue_view_panel_geometry(2, true, 26.0f, 1280.0f, 720.0f, &g);
        ContinueViewCard card = {};
        card.aspect_width = 3;
        card.aspect_height = 4;
        float width = 0.0f;
        float height = 0.0f;
        continue_view_panel_picture_size(&card, &g, &width, &height);
        if (width != 227.0f || height != 302.0f)
        {
            std::printf("FAILED: vertical picture in the box: %.0f x %.0f\n", static_cast<double>(width),
                        static_cast<double>(height));
            g_failures++;
        }
        // No other games: the card alone takes the room.
        continue_view_panel_geometry(2, false, 26.0f, 1280.0f, 720.0f, &g);
        if (g.tile_width != 0.0f || g.picture_height <= 302.0f)
        {
            std::printf("FAILED: no shelf: tile %.0f, picture height %.0f\n", static_cast<double>(g.tile_width),
                        static_cast<double>(g.picture_height));
            g_failures++;
        }
    }
    // Every window shape: the panel stays on the screen under the bar, at
    // least the share it was meant to take one way, nothing empty or negative.
    for (float screen_width = 480.0f; screen_width <= 3840.0f; screen_width += 37.0f)
    {
        for (float screen_height = 360.0f; screen_height <= 2160.0f; screen_height += 41.0f)
        {
            float fit_width = std::floor(screen_width / 640.0f);
            float fit_height = std::floor(screen_height / 360.0f);
            if (screen_height > screen_width)
            {
                fit_width = std::floor(screen_width / 360.0f);
                fit_height = std::floor(screen_height / 640.0f);
            }
            float fit = fit_width < fit_height ? fit_width : fit_height;
            int32_t scale = fit >= 1.0f ? static_cast<int32_t>(fit) : 1;
            float top = 13.0f * static_cast<float>(scale);
            for (int32_t shelf = 0; shelf < 2; shelf++)
            {
                ContinueViewPanelGeometry g;
                continue_view_panel_geometry(scale, shelf == 1, top, screen_width, screen_height, &g);
                bool inside = g.x >= 0.0f && g.y >= top && g.x + g.width <= screen_width &&
                              g.y + g.height <= screen_height;
                bool filled = g.width >= 0.6f * screen_width || g.height >= 0.6f * (screen_height - top);
                bool sane = g.picture_width >= 1.0f && g.picture_height >= 1.0f && g.words_width >= 1.0f &&
                            (shelf == 0 || (g.tile_width >= 1.0f && g.tile_height >= 1.0f));
                if (!inside || !filled || !sane)
                {
                    std::printf("FAILED: %.0f x %.0f at x%d (shelf %d): panel %.0f,%.0f %.0f x %.0f%s%s%s\n",
                                static_cast<double>(screen_width), static_cast<double>(screen_height), scale, shelf,
                                static_cast<double>(g.x), static_cast<double>(g.y), static_cast<double>(g.width),
                                static_cast<double>(g.height), inside ? "" : " off the screen",
                                filled ? "" : " too small", sane ? "" : " a size of zero");
                    g_failures++;
                }
            }
        }
    }

    // The layout decision.
    struct LayoutCase
    {
        float screen_width;
        float screen_height;
        int32_t scale;
        ContinueViewLayout expected;
        const char* what;
    };
    const LayoutCase layouts[] = {
        {1280.0f, 720.0f, 2, ContinueViewLayout::side_shelf, "720p at x2: shelf under the card"},
        {1920.0f, 1080.0f, 3, ContinueViewLayout::side_shelf, "1080p: shelf"},
        {1280.0f, 719.0f, 1, ContinueViewLayout::side_list, "under 720 tall, wide: the list at the right"},
        {800.0f, 600.0f, 1, ContinueViewLayout::side_list, "800x600: the list"},
        {489.0f, 400.0f, 1, ContinueViewLayout::side_shelf, "not wide enough for the list: shelf"},
        {490.0f, 400.0f, 1, ContinueViewLayout::side_list, "just wide enough: list"},
        {600.0f, 900.0f, 1, ContinueViewLayout::stacked_shelf, "taller than wide: stacked"},
        {400.0f, 500.0f, 1, ContinueViewLayout::stacked_shelf, "a small tall window: stacked, never the list"},
        {800.0f, 600.0f, 2, ContinueViewLayout::side_shelf, "scale 2 (the user's choice): shelf"},
        {720.0f, 720.0f, 1, ContinueViewLayout::side_shelf, "square, 720 tall: shelf"},
    };
    for (size_t i = 0; i < sizeof(layouts) / sizeof(layouts[0]); i++)
    {
        const LayoutCase* c = &layouts[i];
        if (continue_view_panel_layout(c->screen_width, c->screen_height, c->scale) != c->expected)
        {
            std::printf("FAILED: layout: %s\n", c->what);
            g_failures++;
        }
    }

    if (g_failures == 0)
    {
        std::printf("continue view: all checks passed\n");
        return 0;
    }
    return 1;
}
