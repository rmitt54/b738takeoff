#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include "../src/airports.h"
#include "../src/model.h"
using namespace pfm;
int main(int, char** argv) {
    std::ifstream f(argv[1]);
    std::string line; int n = 0, bad = 0;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        double v[24];
        for (double& x : v) ss >> x;
        Params p; p.flaps = (int)v[1]; p.derate = (int)v[2]; p.v1_max = v[3] > 0; p.ai = v[4] > 0;
        p.oat = v[5]; p.hw = v[6]; p.slope = v[7]; p.wet = v[8] > 0; p.tora = v[9];
        p.atm = atmo(v[10], v[11], p.oat);
        int lo, hi; flex_range(p, lo, hi);
        auto mf = find_max_flex(p, v[0], lo, hi);
        Result r = model(p, v[0], (v[12] > 0 && mf) ? std::optional<double>(*mf) : std::nullopt);
        auto lw = limit_w(p, std::nullopt, false), lc = limit_w(p, std::nullopt, true), lb = limit_w_both(p, std::nullopt);
        double got[11] = {(double)r.V1, (double)r.VR, (double)r.V2, r.todr, r.climbNet, (double)lo, (double)hi,
                          mf ? *mf : -999.0, lw ? *lw : -1, lc ? *lc : -1, lb ? *lb : -1};
        bool ok = true;
        for (int i = 0; i < 11; ++i)
            if (std::fabs(got[i] - v[13 + i]) > 1e-6 * std::max(1.0, std::fabs(v[13 + i]))) { ok = false; std::printf("  field %d: %.9g vs %.9g\n", i, got[i], v[13 + i]); }
        ++n; if (!ok) ++bad;
    }
    std::printf("model cases %d, mismatches %d\n", n, bad);
    // Airports
    for (const char* c : {"CYYZ", "YYZ", "EGGD", "CYUL", "ZZZZ", "BRS"}) {
        const Airport* a = find_airport(c);
        if (!a) { std::printf("%s: not found\n", c); continue; }
        std::printf("%s: %s / %s  %s, %s  %d ft  %zu ends; first %s %.0f m mag %d tru %d slope %.2f\n", c, a->icao.c_str(), a->iata.c_str(),
                    a->name.c_str(), a->city.c_str(), a->elevFt, a->ends.size(), a->ends[0].id.c_str(), a->ends[0].len, a->ends[0].mag, a->ends[0].tru, a->ends[0].slope);
    }
    // EGGD case from the calibration
    Params p; p.flaps = 15; p.oat = 7; p.tora = 2011; p.atm = atmo(0, 1013, 7);
    Result r = model(p, 74500, std::nullopt);
    auto lw = limit_w_both(p, std::nullopt);
    std::printf("EGGD-like: %d/%d/%d limit %d\n", r.V1, r.VR, r.V2, lw ? rnd(*lw) : -1);
    return bad != 0;
}
