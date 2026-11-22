// retro_ui_scale_for_window: the block for a window, either way up.

#include <cstdint>
#include <cstdio>

#include "retro_ui.h"

static int g_failures = 0;

static void check_scale(float width, float height, int32_t expected, const char* what)
{
    int32_t scale = retro_ui_scale_for_window(width, height);
    if (scale != expected)
    {
        std::printf("FAILED: %s: %.0f x %.0f gave %d, expected %d\n", what, static_cast<double>(width),
                    static_cast<double>(height), scale, expected);
        g_failures++;
    }
}

int main()
{
    check_scale(1920.0f, 1080.0f, 3, "1080p window");
    check_scale(1280.0f, 720.0f, 2, "720p window");
    check_scale(3840.0f, 2160.0f, 6, "4K window");
    check_scale(640.0f, 480.0f, 1, "small window");
    check_scale(300.0f, 200.0f, 1, "never below 1");
    // A 1080 x 2340 phone (Galaxy S22, pixels on Android): 3 either way up.
    check_scale(2340.0f, 1080.0f, 3, "phone sideways");
    check_scale(1080.0f, 2340.0f, 3, "phone upright");
    // A wide window that's short: the height decides.
    check_scale(2560.0f, 720.0f, 2, "wide and short");
    if (g_failures != 0)
    {
        return 1;
    }
    std::printf("scale OK\n");
    return 0;
}
