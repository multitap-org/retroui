// continue_view_picture_size / continue_view_card_picture_size: the CONTINUE
// card's picture at the aspect the game was played at, inside the room it has.

#include <cstdint>
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
    check_size(4, 3, 0.5f, 0.5f, 1.0f, 1.0f, "never below one pixel");

    if (g_failures == 0)
    {
        std::printf("continue view: all checks passed\n");
        return 0;
    }
    return 1;
}
