#include "model.h"

#include <algorithm>
#include <cmath>

namespace pfm {

namespace {
constexpr double DIST_K = 1.0326;
constexpr double TAIR = 11.0;
constexpr double SLOPE = 0.0085;
constexpr double MAX_REDUCTION = 0.25;
constexpr int MAX_ASSUMED = 70;
constexpr double PACKS_LOSS = 0.005;  // thrust lost with packs on; an estimate

double bleed_loss(const Params& p) { return (p.ai ? 0.015 : 0.0) + (p.packs ? PACKS_LOSS : 0.0); }

double kv2(int f) { return f == 1 ? 18.4 : (f == 15 ? 17.5 : 17.78); }
double flapf(int f) { return f == 1 ? 1.07 : (f == 15 ? 0.967 : 1.0); }
double ldr(int f) { return f == 1 ? 12.8 : (f == 15 ? 11.0 : 12.2); }
double derate_factor(int d) { return d == 1 ? 24.0 / 26.3 : (d == 2 ? 22.0 / 26.3 : 1.0); }
}  // namespace

double clamp(double x, double a, double b) { return std::min(b, std::max(a, x)); }

const char* derate_name(int d) { return d == 1 ? "TO 1" : (d == 2 ? "TO 2" : "TO"); }
const char* derate_flex_name(int d) { return d == 1 ? "D-TO 1" : (d == 2 ? "D-TO 2" : "D-TO"); }
const char* derate_k(int d) { return d == 1 ? "24K" : (d == 2 ? "22K" : "26K"); }

Atmo atmo(double elev_ft, double qnh, double oat) {
    Atmo a;
    a.pa = elev_ft + (1013.25 - qnh) * 27;
    a.delta = std::pow(1 - 6.8756e-6 * a.pa, 5.2559);
    double theta = (273.15 + oat) / 288.15;
    a.sigma = a.delta / theta;
    a.tref = 30 - 1.98 * a.pa / 1000;
    return a;
}

Result model(const Params& p, double W, std::optional<double> T) {
    const Atmo& a = p.atm;
    double tw = W / 1000.0;
    int V2 = rnd(kv2(p.flaps) * std::sqrt(tw));
    int gap = p.flaps == 15 ? 8 : rnd(clamp(3 + 3 * (79.016 - tw) / 34, 3, 6));
    int VR = V2 - gap;
    double ai_loss = bleed_loss(p);
    double t_use = T ? *T : p.oat;
    double tf = clamp(derate_factor(p.derate) - SLOPE * std::max(0.0, t_use - a.tref) - ai_loss, 0.5, 1);
    double tf_full = clamp(1 - SLOPE * std::max(0.0, p.oat - a.tref) - ai_loss, 0.5, 1);
    double vtas = VR / std::sqrt(a.sigma);
    double weff = p.hw >= 0 ? 0.5 * p.hw : 1.5 * p.hw;
    double wind_f = std::pow(clamp((vtas - weff) / vtas, 0.6, 1.6), 2);
    double dbase = 2350 * DIST_K * std::pow(W / MTOW, 1.9);
    double todr = dbase * std::pow(a.sigma, -1.2) / tf * flapf(p.flaps) *
                  clamp(1 + 0.07 * p.slope, 0.7, 1.5) * wind_f * (p.wet ? 1.07 : 1.0);
    double margin = p.tora / todr;
    // One engine out, second segment: gear up, takeoff flaps, V2.
    double thrust_lb = 26300 * 0.82 * std::pow(a.delta, 0.85) * tf;
    double climb_net = (thrust_lb / (W * KG2LB) - 1 / ldr(p.flaps) - 0.008) * 100;
    bool climb_ok = climb_net >= 2.4;
    int vmcg = rnd(85 + 23 * tf * std::sqrt(a.sigma));
    // V1 is the highest speed from which a rejected takeoff still stops on the runway.
    double accel = std::max(0.3, 2 * 117000 * 0.88 * tf * std::pow(a.delta, 0.85) / W - 0.4 - 9.81 * p.slope / 100);
    double decel = p.wet ? 2.6 : 3.6;
    double kk = 1 / (2 * accel) + 1 / (2 * decel);
    double vg_stop = (-2 + std::sqrt(4 + 4 * kk * p.tora)) / (2 * kk);
    double v1_stop = (vg_stop * 1.94384 + weff) * std::sqrt(a.sigma);
    // Balanced field V1.
    double vrg = std::max(10.0, vtas - weff) * 0.51444;
    double oei = std::max(0.3, 117000 * 0.88 * tf * std::pow(a.delta, 0.85) / W - 0.6 - 9.81 * p.slope / 100);
    double c2 = 1 / (2 * decel) + 1 / (2 * oei);
    double c0 = vrg * vrg / (2 * oei) + vrg * TAIR;
    double vg_bal = (-2 + std::sqrt(4 + 4 * c2 * c0)) / (2 * c2);
    double v1_bal = (std::min(vg_bal, vrg) * 1.94384 + weff) * std::sqrt(a.sigma);
    double v1_pick = p.v1_max ? v1_stop : std::min(v1_bal, v1_stop);
    int V1 = rnd(std::min<double>(VR, v1_pick));
    bool floor_hit = V1 < vmcg;
    V1 = std::min(std::max(V1, vmcg), VR);

    Result r;
    r.V1 = V1; r.VR = VR; r.V2 = V2; r.vmcg = vmcg;
    r.todr = todr; r.margin = margin; r.tf = tf; r.tfRel = tf / tf_full;
    r.n1 = 98.7 * std::sqrt((p.oat + 273.15) / 287.15) * std::pow(r.tfRel, 1.0 / 3.0);
    r.v1floor = floor_hit; r.gs = vtas - p.hw;
    r.fieldOk = margin >= 1; r.climbNet = climb_net; r.climbOk = climb_ok;
    r.ok = r.fieldOk && climb_ok;
    return r;
}

void flex_range(const Params& p, int& lo, int& hi) {
    lo = static_cast<int>(std::ceil(std::max(p.oat, p.atm.tref)));
    double room = derate_factor(p.derate) - (1 - MAX_REDUCTION) - bleed_loss(p);
    hi = static_cast<int>(std::floor(std::min<double>(MAX_ASSUMED, p.atm.tref + room / SLOPE)));
}

std::optional<int> find_max_flex(const Params& p, double W, int lo, int hi) {
    for (int T = hi; T >= lo; --T)
        if (model(p, W, T).ok) return T;
    return std::nullopt;
}

namespace {
template <class Pred>
double bisect(Pred ok) {
    double a = 35000.0, b = MTOW;
    for (int i = 0; i < 30; ++i) {
        double m = (a + b) / 2;
        if (ok(m)) a = m; else b = m;
    }
    return a;
}
}  // namespace

std::optional<double> limit_w(const Params& p, std::optional<double> T, bool climb_check) {
    auto ok = [&](double w) { Result r = model(p, w, T); return climb_check ? r.climbOk : r.fieldOk; };
    if (ok(MTOW)) return std::nullopt;
    if (!ok(35000)) return 0.0;
    return bisect(ok);
}

std::optional<double> limit_w_both(const Params& p, std::optional<double> T) {
    auto ok = [&](double w) { return model(p, w, T).ok; };
    if (ok(MTOW)) return MTOW;
    if (!ok(35000)) return std::nullopt;
    return bisect(ok);
}

}  // namespace pfm
