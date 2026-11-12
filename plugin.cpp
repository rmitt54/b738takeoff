// X-Plane 12 plugin entry points: command, menu item, simulator weather, and the window.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

#include "XPLMDataAccess.h"
#include "XPLMMenus.h"
#include "XPLMPlugin.h"
#include "XPLMUtilities.h"
#include "airports.h"
#include "imgui_xp.h"
#include "model.h"
#include "ui.h"

namespace {

pfm::State g_state;
std::unique_ptr<pfm::XpImguiWindow> g_window;
XPLMCommandRef g_cmd = nullptr;
int g_menu_item = -1;

XPLMDataRef g_oat = nullptr, g_qnh = nullptr, g_wspd = nullptr, g_wdir = nullptr;

XPLMDataRef find_first(std::initializer_list<const char*> names) {
    for (const char* n : names)
        if (XPLMDataRef ref = XPLMFindDataRef(n)) return ref;
    return nullptr;
}

// Reads a float, double, int, or the first element of a float array.
bool read_value(XPLMDataRef ref, double& out) {
    if (!ref) return false;
    int types = XPLMGetDataRefTypes(ref);
    if (types & xplmType_FloatArray) {
        float v = 0;
        if (XPLMGetDatavf(ref, &v, 0, 1) < 1) return false;
        out = v;
        return true;
    }
    if (types & xplmType_Float) { out = XPLMGetDataf(ref); return true; }
    if (types & xplmType_Double) { out = XPLMGetDatad(ref); return true; }
    if (types & xplmType_Int) { out = XPLMGetDatai(ref); return true; }
    return false;
}

pfm::SimWx read_weather() {
    pfm::SimWx w;
    double v;
    if (read_value(g_oat, v)) w.oat = pfm::rnd(v);
    if (read_value(g_qnh, v)) {
        // Pascals, inHg or hPa: tell them apart by size
        double hpa = v > 50000 ? v / 100.0 : (v < 40 ? v * 33.8639 : v);
        w.qnh_hpa = pfm::rnd(hpa);
    }
    if (read_value(g_wspd, v)) w.wspd = std::max(0, pfm::rnd(v));
    if (read_value(g_wdir, v)) w.wdir = ((pfm::rnd(v) % 360) + 360) % 360;
    return w;
}

void toggle_window() {
    if (!g_window) {
        g_window = std::make_unique<pfm::XpImguiWindow>("737-800 Takeoff Performance", 560, 960, [] {
            pfm::SimWx wx = read_weather();
            pfm::ui_draw(g_state, &wx);
        });
    } else {
        g_window->set_visible(!g_window->visible());
    }
}

int command_handler(XPLMCommandRef, XPLMCommandPhase phase, void*) {
    if (phase == xplm_CommandBegin) toggle_window();
    return 1;
}

}  // namespace

PLUGIN_API int XPluginStart(char* out_name, char* out_sig, char* out_desc) {
    std::strcpy(out_name, "737-800 Takeoff Performance");
    std::strcpy(out_sig, "b738.takeoff.performance");
    std::strcpy(out_desc, "V1, VR, V2 with flex and derates");
    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);

    char path[1024] = {0};
    XPLMGetSystemPath(path);
    pfm::set_system_path(path);

    g_cmd = XPLMCreateCommand("b738_takeoff_perf/toggle", "Show or hide the 737-800 takeoff performance window");
    XPLMRegisterCommandHandler(g_cmd, command_handler, 1, nullptr);
    g_menu_item = XPLMAppendMenuItemWithCommand(XPLMFindPluginsMenu(), "737 Takeoff Performance", g_cmd);
    return 1;
}

PLUGIN_API void XPluginStop(void) {
    if (g_cmd) XPLMUnregisterCommandHandler(g_cmd, command_handler, 1, nullptr);
    if (g_menu_item >= 0) XPLMRemoveMenuItem(XPLMFindPluginsMenu(), g_menu_item);
    g_window.reset();
}

PLUGIN_API int XPluginEnable(void) {
    g_oat = find_first({"sim/cockpit2/temperature/outside_air_temp_degc", "sim/weather/temperature_ambient_c"});
    g_qnh = find_first({"sim/weather/aircraft/qnh_pas", "sim/weather/barometer_sealevel_inhg",
                        "sim/weather/region/sealevel_pressure_pas"});
    g_wspd = find_first({"sim/weather/aircraft/wind_speed_kts", "sim/weather/wind_speed_kt"});
    g_wdir = find_first({"sim/weather/aircraft/wind_direction_degt", "sim/weather/wind_direction_degt"});
    return 1;
}

PLUGIN_API void XPluginDisable(void) { g_window.reset(); }

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID, int, void*) {}
