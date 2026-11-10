// The Statistics Overlay (F9, [video] stats_overlay): a see-through panel at the left of the
// screen showing how the game is running and on what. It only draws the lines and graphs it is
// given (the framework gathers the figures: CD3DApplication::DrawStatsOverlay), so it can't
// change the game.
#pragma once
#include "ui_draw.h"
#include <string>
#include <vector>

class Device;

// A heading and its rows. A row with an empty label puts its value across the whole line (long
// text such as the graphics card's name); `dim` greys a value that is only a note.
struct StatsGroup {
    struct Row {
        std::string label, value;
        bool dim = false;
    };
    std::string title;
    std::vector<Row> rows;
};

// The graphs (O2): the last frames drawn, oldest first. One column a frame, as many as the graph
// is wide (the newest at the right).
struct StatsGraphs {
    std::vector<float> frameMs;           // time since the frame before
    std::vector<float> alpha;             // the blend each was drawn at (1 with Smooth motion off)
    std::vector<unsigned char> ticks;     // ticks each ran
    float targetMs = 0;                   // the frame time to expect (a line; 0 = none)
    std::string target;                   // what that is ("refresh", "limit", "tick")
    float scaleMs = 20;                   // the frame time at the top of its graph
    bool smooth = false;                  // Smooth motion on (the blend means something)
};

class StatsOverlay {
public:
    // Draws the panel at the view's left edge from `top` (in the 480-high space) down. Its shape
    // follows the view's width, never a fixed aspect ratio: narrower than kTwoColumnView (4:3 to
    // about 16:10), the groups of both lists are stacked in one column; wider (about 16:9 and up),
    // `left` and `right` sit side by side. The graphs go below the figures, across the panel; only
    // where they don't fit (`full` in one column) do they get a small panel of their own beside
    // it, from `sideTop` (below the HUD's top-right panel).
    void Render(Device* d, float top, float sideTop, bool full, const std::string& header,
                const std::string& subheader, const std::vector<StatsGroup>& left,
                const std::vector<StatsGroup>& right, const StatsGraphs& graphs);

    static constexpr float kTwoColumnView = 800.f;  // the view's width (640 at 4:3, 853 at 16:9)

private:
    void Column(float x, float y, const std::vector<StatsGroup>& groups);
    void Graphs(float x, float y, float width, const StatsGraphs& g);
    void Panel(float x0, float y0, float x1, float y1);
    void OutlinedText(float x, float y, const std::string& s, UIColor c);
    static float Height(const std::vector<StatsGroup>& groups);
    UIDraw ui_;
};
