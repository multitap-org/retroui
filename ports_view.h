#pragma once

// Who plays on which port, drawn the same in every frontend (Andrés,
// 2026-11-20; piccoletto's DEVICES look, its port LEDs and ping):
//
//   TABS   one per port, all the same width, at the bottom left. When the
//          frontend reports a change (ports_view_pop) every tab rises to its
//          first line (the port's LEDs, P1, a link's ping) and the ports that
//          changed rise whole (who is on them); about 3 s later they go down.
//          With `peek` they stay at their first line (piccolo's menu).
//          Animated, behind every window; more than 4 overlap.
//   BOARD  full screen over the game: a column per port (OUT first, when
//          shown), scrolled sideways when they don't fit. A click / tap picks
//          a chip, a click on another column asks to move it there, DONE
//          closes. The frontend does the moving (its router, or the host).
//
// Only what is drawn lives here: the frontend fills the columns (rebuilding
// them when who is where changes, never per frame) and says what changed.
// No input router, no network.

#include <cstdint>
#include <string>
#include <vector>

struct ImFont;

constexpr uint32_t PORTS_VIEW_MAX_PORTS = 8;

struct PortsViewChip
{
    uint32_t    key = 0;            // the frontend's name for it (picks, moves)
    std::string label;
    bool        gone = false;       // went away: greyed with OFF until the tabs go down
    bool        yours = false;      // the viewer's own (a bar on its left: a phone's pads)
    bool        has_ping = false;   // played over a link: ping bars (the number on the first line)
    bool        ping_known = false;
    uint32_t    ping_ms = 0;
};

struct PortsViewColumn
{
    std::vector<PortsViewChip> chips;
};

// The words, in the frontend's language (the formats take one %u).
struct PortsViewText
{
    const char* tab = "P%u";
    const char* port = "PORT %u";
    const char* ping = "%u MS";
    const char* nobody = "NOBODY";
    const char* off = "OFF";
    const char* out = "OUT";
    const char* title = "DEVICES";
    const char* done = "DONE";
    const char* help = "";
};

struct PortsView
{
    // Filled by the frontend.
    uint32_t        port_count = 0;      // 0: nothing is drawn
    PortsViewColumn ports[PORTS_VIEW_MAX_PORTS];
    PortsViewColumn out;                 // on no port (the board's first column)
    bool            show_out = true;     // the board shows OUT
    float           chip_height = 15.0f; // retro pixels (taller for touch)

    // The tabs (ports_view keeps these).
    uint64_t popped_ms = 0;                        // when they rose for a change (0 = not up)
    bool     changed[PORTS_VIEW_MAX_PORTS] = {};   // these rise whole
    float    height[PORTS_VIEW_MAX_PORTS] = {};    // risen now (retro pixels; animated)
    float    target[PORTS_VIEW_MAX_PORTS] = {};
    uint64_t target_ms[PORTS_VIEW_MAX_PORTS] = {}; // when it may start toward `target` (staggered)
    uint64_t last_draw_ms = 0;
    float    screen_width = 0.0f;                  // retro pixels, at the last draw (the tabs overlap to fit)

    // The board.
    bool     has_pick = false;
    uint32_t picked = 0;                 // a chip's key
    float    scroll = -1.0f;             // window pixels (-1: placed at once next draw)
    float    scroll_target = 0.0f;
};

// No game: nothing shown, nothing remembered.
void ports_view_reset(PortsView* view);

// Who is where changed: the tabs rise, `changed` ports (port_count of them)
// whole. Another change while they're up adds to it and keeps them up.
void ports_view_pop(PortsView* view, const bool* changed, uint64_t now_ms);

// The tabs are up or moving (the frontend refreshes pings meanwhile).
bool ports_view_tabs_visible(const PortsView* view);

// How far the tabs reach from the left edge, in retro pixels (0 = none).
float ports_view_tabs_width(const PortsView* view);

// The tabs, behind every window. `peek`: at their first line when nothing
// changed; `hidden`: down (the board covers them); `front`: above every
// window (a frontend that draws its screens in a window), else behind them.
void ports_view_draw_tabs(PortsView* view, ImFont* pixel_font, int32_t scale, const PortsViewText* text, bool peek,
                          bool hidden, bool front, uint64_t now_ms);

// The column a chip is in (-1 = OUT), or false.
bool ports_view_find(const PortsView* view, uint32_t key, int32_t* column);

// UP / DOWN on a pad: the next chip in the board's order (wrapping around).
void ports_view_pick_next(PortsView* view, int32_t direction);

// What a click / tap on the board asked for (drawn this frame).
struct PortsViewBoardInput
{
    bool     done = false;
    bool     move = false;     // move `key` to `port` (-1 = OUT)
    uint32_t key = 0;
    int32_t  port = -1;
};

// The board, above everything; a click on a chip picks it.
void ports_view_draw_board(PortsView* view, ImFont* pixel_font, int32_t scale, const PortsViewText* text,
                           PortsViewBoardInput* input);
