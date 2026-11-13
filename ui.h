// ImGui user interface for the takeoff performance window. No X-Plane dependency.
#pragma once
#include <optional>
#include <string>

#include "airports.h"
#include "model.h"

namespace pfm {

// Live values from the simulator. Missing values are left empty.
struct SimWx {
    std::optional<int> oat, qnh_hpa, wspd, wdir;
};

struct State {
    // Aircraft
    int variant = 1;                     // 0 = 737-700, 1 = 737-800, 2 = 737-900, 3 = 737-900ER
    std::optional<int> weight;           // empty field = no weight yet
    char weight_txt[12] = "";
    int flaps = 5;
    bool ai = false;
    bool packs = false;   // air conditioning packs on
    // Thrust
    int derate = 0;
    bool flex_mode = true;
    bool v1_max = false;
    // Airport and runway
    char icao[8] = "";
    std::string last_code;
    const Airport* apt = nullptr;
    int sel = -1;
    int elev_ft = 0;
    int rwy_len = 3000;
    int rwy_hdg = 270;
    int slope10 = 0;
    bool wet = false;
    std::string status = "Type an ICAO or IATA code, for example CYYZ, then press Enter.";
    // Weather
    bool auto_wx = true;
    int oat = 15;
    int qnh = 1013;
    int altim = 2992;   // inHg x 100
    bool qnh_inhg = false;
    int wdir = 270;
    int wspd = 0;
};

// Call once after ImGui::CreateContext(). Sets the theme and loads the two fonts.
void ui_init_style();
// Window chrome shared with the X-Plane window code: a header you can drag, a close button in its
// right end, and a grip in the bottom-right corner for resizing. Sizes are in pixels.
constexpr float kHeaderHeight = 58.0f;
constexpr float kCloseZoneWidth = 60.0f;
constexpr float kGripSize = 26.0f;

// Call every frame between ImGui::NewFrame() and ImGui::Render().
// Returns true when the close button was clicked.
bool ui_draw(State& s, const SimWx* wx);

}  // namespace pfm
