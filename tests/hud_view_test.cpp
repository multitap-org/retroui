// hud_view's layout: where the state icons and the notification cards go,
// and that nothing overlaps.

#include <cstdio>
#include <cstring>

#include "hud_view.h"
#include "pixel_font.h"

static int g_failures = 0;

static void check(bool ok, const char* what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        g_failures++;
    }
}

static HudCard two_line_card(const char* chip, uint64_t since_ms)
{
    HudCard card = {};
    std::snprintf(card.chip, sizeof(card.chip), "%s", chip);
    std::snprintf(card.title, sizeof(card.title), "%s", "SN30 PRO");
    std::snprintf(card.before, sizeof(card.before), "%s", "HOLD");
    std::snprintf(card.key, sizeof(card.key), "%s", "START");
    std::snprintf(card.after, sizeof(card.after), "%s", "TO REMAP");
    card.since_ms = since_ms;
    return card;
}

static bool overlap(HudRect a, HudRect b)
{
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

int main()
{
    pixel_font_init();
    HudStack* stack = new HudStack{};
    HudArea screen = {};
    screen.right = 640.0f;
    screen.top = 0.0f;
    screen.bottom = 344.0f;   // above the status line's row

    // One card: right-aligned, standing on the bottom.
    HudCard first = two_line_card("PORT 1", 1000);
    check(hud_view_add_card(stack, &first), "add a card");
    HudRect only = hud_view_card_rect(stack, &screen, 0);
    check(only.shown, "one card: shown");
    check(only.x + only.width == 640.0f - HUD_VIEW_MARGIN, "one card: at the right margin");
    check(only.y + only.height == 344.0f, "one card: on the bottom");

    // A newer card goes below, the older one is pushed up; added in either order.
    HudCard newer = two_line_card("PORT 2", 2000);
    hud_view_clear(stack);
    hud_view_add_card(stack, &newer);
    hud_view_add_card(stack, &first);
    HudRect bottom = hud_view_card_rect(stack, &screen, 0);
    HudRect above = hud_view_card_rect(stack, &screen, 1);
    check(bottom.y + bottom.height == 344.0f, "two cards: the newest on the bottom");
    check(above.y + above.height + HUD_VIEW_GAP == bottom.y, "two cards: the older one right above, a gap apart");
    check(!overlap(bottom, above), "two cards: no overlap");

    // One line only: a shorter card.
    HudCard short_card = {};
    std::snprintf(short_card.title, sizeof(short_card.title), "%s", "REWIND: NOTHING RECORDED YET");
    check(hud_view_card_height(&short_card) < hud_view_card_height(&first), "one line: shorter");

    // A long message wraps at a space onto the second line, each part fitting.
    HudCard wrapped = {};
    hud_view_card_text(&wrapped, "REWIND IS OFF (SETTINGS > GENERAL)");
    check(std::strcmp(wrapped.title, "REWIND IS OFF") == 0, "wrap: the first words on the title line");
    check(std::strcmp(wrapped.before, "(SETTINGS > GENERAL)") == 0, "wrap: the rest on the second line");
    check(hud_view_card_height(&wrapped) == hud_view_card_height(&first), "wrap: a two-line card");
    HudCard fits = {};
    hud_view_card_text(&fits, "PAUSED");
    check(fits.before[0] == '\0', "wrap: a short message stays on one line");

    // A small area: the oldest cards don't fit and aren't shown; the newest is.
    HudArea small = screen;
    small.top = 344.0f - 40.0f;
    HudRect small_bottom = hud_view_card_rect(stack, &small, 0);
    HudRect small_above = hud_view_card_rect(stack, &small, 1);
    check(small_bottom.shown && !small_above.shown, "small area: the newest shown, the oldest not");

    // The area's right edge moves (QUICK MENU up): the cards follow it.
    HudArea left_half = screen;
    left_half.right = 320.0f;
    HudRect moved = hud_view_card_rect(stack, &left_half, 0);
    check(moved.x + moved.width == 320.0f - HUD_VIEW_MARGIN, "the right edge moved: the cards follow");

    // A full stack refuses more.
    hud_view_clear(stack);
    for (uint32_t i = 0; i < HUD_VIEW_MAX_CARDS; i++)
    {
        HudCard card = two_line_card("PORT", i);
        hud_view_add_card(stack, &card);
    }
    HudCard extra = two_line_card("PORT 9", 99);
    check(!hud_view_add_card(stack, &extra), "full: refused");

    // Icons: from the top right, right to left in the order added, apart.
    hud_view_clear(stack);
    check(hud_view_add_icon(stack, HudIcon::pause, ""), "add an icon");
    hud_view_add_icon(stack, HudIcon::fast_forward, "x4");
    hud_view_add_icon(stack, HudIcon::rec, nullptr);
    HudRect pause = hud_view_icon_rect(stack, &screen, 0);
    HudRect ff = hud_view_icon_rect(stack, &screen, 1);
    HudRect rec = hud_view_icon_rect(stack, &screen, 2);
    check(pause.x + pause.width == 640.0f - HUD_VIEW_MARGIN && pause.y == HUD_VIEW_MARGIN, "icons: the first at the corner");
    check(pause.width == HUD_VIEW_ICON && rec.width == HUD_VIEW_ICON, "icons: square without a label");
    check(ff.width > HUD_VIEW_ICON, "icons: wider with a label");
    check(ff.x + ff.width + HUD_VIEW_GAP == pause.x && rec.x + rec.width + HUD_VIEW_GAP == ff.x,
          "icons: right to left, a gap apart");

    // The menu bar showing: the icons hang below it.
    HudArea below_bar = screen;
    below_bar.top = 13.0f;
    check(hud_view_icon_rect(stack, &below_bar, 0).y == 13.0f + HUD_VIEW_MARGIN, "icons: below the area's top");

    // Icons and cards never meet on a normal screen.
    HudCard card = two_line_card("PORT 1", 5);
    hud_view_add_card(stack, &card);
    check(!overlap(hud_view_icon_rect(stack, &screen, 0), hud_view_card_rect(stack, &screen, 0)), "icons and cards apart");

    delete stack;
    if (g_failures == 0)
    {
        std::printf("hud view OK\n");
        return 0;
    }
    return 1;
}
