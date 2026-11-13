// 737-800 takeoff performance model (approximate, calibrated). Pure C++, no X-Plane dependency.
#pragma once
#include <cmath>
#include <optional>

namespace pfm {

constexpr double MTOW = 79016.0;  // kg, 737-800 (calibration reference)
constexpr double KG2LB = 2.20462;
constexpr double M2FT = 3.28084;

double clamp(double x, double a, double b);

// Airframe / engine variants. The model is calibrated on the -800 (CFM56-7B26, 26.3 klbf);
// the others share the wing, so they differ here by max weight and engine thrust only.
struct Variant {
    const char* name;
    const char* engine;
    double mtow;     // kg
    double full_k;   // takeoff thrust, thousand lbf per engine
    const char* label;  // rating label for full thrust
};
constexpr int NUM_VARIANTS = 4;
const Variant& variant_info(int v);

struct Atmo { double pa, delta, sigma, tref; };

struct Params {
    int flaps = 5;           // 1, 5 or 15
    bool ai = false;         // engine anti-ice
    bool packs = false;      // air conditioning packs on (off matches the calibration data)
    int variant = 1;         // 0 = 737-700, 1 = 737-800, 2 = 737-900, 3 = 737-900ER
    int derate = 0;          // 0 = full (26K), 1 = 24K, 2 = 22K
    bool v1_max = false;     // false = balanced field, true = maximum (stop limited)
    double oat = 15;         // C
    double hw = 0;           // headwind component, kt (negative = tailwind)
    double xw = 0;           // crosswind component, kt
    double slope = 0;        // percent, positive = uphill
    bool wet = false;
    double tora = 3000;      // runway available, m
    Atmo atm{0, 1, 1, 30};
};

struct Result {
    int V1 = 0, VR = 0, V2 = 0, vmcg = 0;
    double todr = 0, margin = 0, tf = 0, tfRel = 0, n1 = 0;
    bool v1floor = false;
    double gs = 0;
    bool fieldOk = false;
    double climbNet = 0;
    bool climbOk = false, ok = false;
};

// Round half up, like the Python, Lua and web versions.
inline int rnd(double x) { return static_cast<int>(std::floor(x + 0.5)); }

Atmo atmo(double elev_ft, double qnh_hpa, double oat);
Result model(const Params& p, double W, std::optional<double> T);
void flex_range(const Params& p, int& lo, int& hi);
std::optional<int> find_max_flex(const Params& p, double W, int lo, int hi);
// Heaviest weight passing one check; nullopt = nothing limits below MTOW, 0 = nothing works.
std::optional<double> limit_w(const Params& p, std::optional<double> T, bool climb_check);
// Heaviest weight passing both checks; nullopt = no weight works.
std::optional<double> limit_w_both(const Params& p, std::optional<double> T);

const char* derate_name(int d);       // TO / TO 1 / TO 2
const char* derate_flex_name(int d);  // D-TO / D-TO 1 / D-TO 2
const char* derate_k(int variant, int d);  // 26K / 24K / 22K (27K on the -900ER)
double mtow_of(int variant);

}  // namespace pfm
