#include "ui.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <initializer_list>
#include <cstdio>
#include <string>
#include <vector>

#include "imgui.h"

namespace pfm {

namespace {

const ImVec4 C_TEXT(0.90f, 0.94f, 0.97f, 1.0f);
const ImVec4 C_MUTED(0.56f, 0.64f, 0.70f, 1.0f);
const ImVec4 C_ACCENT(0.36f, 0.67f, 1.00f, 1.0f);
const ImVec4 C_GOOD(0.32f, 0.83f, 0.55f, 1.0f);
const ImVec4 C_WARN(0.96f, 0.65f, 0.32f, 1.0f);
const ImVec4 C_BAD(1.00f, 0.43f, 0.39f, 1.0f);

ImFont* g_big = nullptr;

std::string fmt(const char* f, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, f);
    std::vsnprintf(buf, sizeof buf, f, ap);
    va_end(ap);
    return buf;
}

int clampi(int v, int a, int b) { return std::min(b, std::max(a, v)); }

void ctext(const ImVec4& c, const std::string& t) { ImGui::TextColored(c, "%s", t.c_str()); }

void wrapped(const ImVec4& c, const std::string& t) {
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::TextWrapped("%s", t.c_str());
    ImGui::PopStyleColor();
}

void pill(const std::string& text, const ImVec4& c) {
    ImVec4 bg(c.x * 0.30f, c.y * 0.30f, c.z * 0.30f, 1.0f);
    ImGui::PushID(text.c_str());
    ImGui::PushStyleColor(ImGuiCol_Button, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, bg);
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::SmallButton(text.c_str());
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    ImGui::PopID();
}

void centered(const std::string& text, const ImVec4& c) {
    float w = ImGui::CalcTextSize(text.c_str()).x;
    float off = (ImGui::GetColumnWidth() - w) / 2.0f;
    if (off > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
    ctext(c, text);
}

void big_centered(const std::string& text, const ImVec4& c) {
    if (g_big) ImGui::PushFont(g_big);
    centered(text, c);
    if (g_big) ImGui::PopFont();
}

bool section(const char* title) {
    return ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
}

template <class T>
bool radio_row(const char* label, std::initializer_list<std::pair<T, const char*>> options, T& cur) {
    bool changed = false;
    ctext(C_MUTED, label);
    for (const auto& o : options) {
        ImGui::SameLine();
        std::string id = fmt("%s##%s%d", o.second, label, static_cast<int>(o.first));
        if (ImGui::RadioButton(id.c_str(), cur == o.first)) {
            cur = o.first;
            changed = true;
        }
    }
    return changed;
}

struct Row { std::string label, value; ImVec4 color; };

void stat_grid(const std::vector<Row>& rows) {
    ImGui::Columns(2, "stats", false);
    for (const auto& r : rows) {
        ctext(C_MUTED, r.label);
        ImGui::NextColumn();
        ctext(r.color, r.value);
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
}

std::string fmt_len(double m) { return fmt("%d m / %d ft", rnd(m), rnd(m * M2FT)); }

std::string kg_text(std::optional<double> x) {
    return x ? fmt("%d kg", rnd(*x / 10.0) * 10) : std::string("no limit");
}

struct Calc {
    Params p;
    std::optional<double> W;
    std::optional<double> T;
    bool flex_usable = false;
    bool has = false;
    Result r;
};

Calc compute(const State& s) {
    Calc c;
    double ang = (s.wdir - s.rwy_hdg) * 3.14159265358979323846 / 180.0;
    c.p.flaps = s.flaps; c.p.ai = s.ai; c.p.derate = s.derate; c.p.v1_max = s.v1_max;
    c.p.oat = s.oat; c.p.slope = s.slope10 / 10.0; c.p.wet = s.wet; c.p.tora = s.rwy_len;
    c.p.hw = s.wspd * std::cos(ang);
    c.p.xw = s.wspd * std::sin(ang);
    c.p.atm = atmo(s.elev_ft, s.qnh, s.oat);
    if (!s.weight) return c;
    c.has = true;
    double W = *s.weight;
    c.W = W;
    int lo, hi;
    flex_range(c.p, lo, hi);
    std::optional<int> mf = hi > lo ? find_max_flex(c.p, W, lo, hi) : std::nullopt;
    c.flex_usable = mf && *mf > lo;
    if (s.flex_mode && c.flex_usable) c.T = *mf;
    c.r = model(c.p, W, c.T);
    return c;
}

void draw_no_weight(const State&, const Calc& c) {
    const Params& p = c.p;
    pill("ENTER WEIGHT", C_WARN);
    ImGui::Spacing();
    wrapped(C_MUTED, "Type the takeoff weight below to get V1, VR, V2 and the flex temperature. "
                     "Until then, these are the weight limits for the fixed rating selected.");
    ImGui::Spacing();
    stat_grid({
        {"Runway available", fmt_len(p.tora), C_TEXT},
        {"Runway weight limit", kg_text(limit_w(p, std::nullopt, false)), C_TEXT},
        {"Climb weight limit", kg_text(limit_w(p, std::nullopt, true)), C_TEXT},
        {"Headwind / crosswind", fmt("%d kt / %d kt", rnd(p.hw), rnd(std::fabs(p.xw))), C_TEXT},
    });
}

void draw_results(const State& s, const Calc& c) {
    const Params& p = c.p;
    const Result& r = c.r;
    double W = *c.W;
    bool show = W <= MTOW && r.ok;
    int pct = rnd(r.tfRel * 100);

    if (c.T) pill(fmt("%s  %d C", derate_flex_name(s.derate), rnd(*c.T)), C_ACCENT);
    else pill(fmt("%s  %s", derate_name(s.derate), derate_k(s.derate)), C_ACCENT);
    ImGui::SameLine();
    pill(fmt("%d%% THRUST", pct), C_MUTED);
    ImGui::SameLine();
    if (W > MTOW) pill("OVER MTOW", C_BAD);
    else if (!r.fieldOk) pill("RUNWAY LIMITED", C_BAD);
    else if (!r.climbOk) pill("CLIMB LIMITED", C_BAD);
    else pill("READY", C_GOOD);
    if (s.flex_mode && !c.flex_usable) ctext(C_WARN, "Flex is not available here, so the fixed rating is used.");

    ImGui::Spacing();

    ImGui::Columns(3, "speeds", false);
    struct Cell { const char* label; int v; ImVec4 col; };
    for (const Cell& cell : {Cell{"V1", r.V1, C_ACCENT}, Cell{"VR", r.VR, C_TEXT}, Cell{"V2", r.V2, C_GOOD}}) {
        centered(cell.label, C_MUTED);
        big_centered(show ? fmt("%d", cell.v) : std::string("---"), show ? cell.col : C_BAD);
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
    ctext(C_MUTED, fmt("knots indicated  |  flaps %d", s.flaps));

    if (!show) {
        ImGui::Spacing();
        if (W > MTOW) {
            ctext(C_BAD, "Weight is above the maximum takeoff weight.");
        } else {
            auto mt = limit_w_both(p, c.T);
            std::string why = !r.fieldOk ? "Runway limited" : fmt("Climb limited (net %.1f%%, need 2.4%%)", r.climbNet);
            if (mt) wrapped(C_BAD, fmt("%s. Heaviest weight that works here: %s.", why.c_str(), kg_text(mt).c_str()));
            else wrapped(C_BAD, fmt("%s at any weight.", why.c_str()));
        }
    }

    ImGui::Spacing();

    double frac = p.tora > 0 ? clamp(r.todr / p.tora, 0, 1) : 1.0;
    ImVec4 col = r.todr > p.tora ? C_BAD : (r.todr > 0.92 * p.tora ? C_WARN : C_ACCENT);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
    ImGui::ProgressBar(static_cast<float>(frac), ImVec2(-1, 18), fmt("%d%% of runway", rnd(frac * 100)).c_str());
    ImGui::PopStyleColor();

    ImVec4 ccol = r.climbNet < 2.4 ? C_BAD : (r.climbNet < 3 ? C_WARN : C_GOOD);
    stat_grid({
        {"Runway needed", fmt_len(r.todr), C_TEXT},
        {"Runway available", fmt_len(p.tora), C_TEXT},
        {"Engine-out climb", fmt("net %.1f%%  (min 2.4%%)", r.climbNet), ccol},
        {"Runway weight limit", kg_text(limit_w(p, c.T, false)), C_TEXT},
        {"Climb weight limit", kg_text(limit_w(p, c.T, true)), C_TEXT},
        {"Headwind / crosswind", fmt("%d kt / %d kt", rnd(p.hw), rnd(std::fabs(p.xw))), C_TEXT},
    });

    if (r.v1floor) ctext(C_WARN, fmt("V1 is held at the Vmcg floor (%d kt). Runway is critical.", r.vmcg));
    if (r.gs > 195) ctext(C_WARN, "Ground speed at VR is above the tire speed limit.");
    if (std::fabs(p.xw) > 33) ctext(C_WARN, "Crosswind is above the demonstrated 33 kt.");
}

void apply_airport(State& s, const Airport* apt) {
    s.apt = apt;
    s.sel = -1;
    s.elev_ft = apt->elevFt;
    s.status = fmt("%s%s  %s, %s  (elevation %d ft, %s data)", apt->icao.c_str(),
                   apt->iata.empty() ? "" : (" / " + apt->iata).c_str(), apt->name.c_str(), apt->city.c_str(),
                   apt->elevFt, apt->src.c_str());
}

void lookup(State& s, bool force) {
    std::string code;
    for (const char* q = s.icao; *q; ++q)
        if (std::isalnum(static_cast<unsigned char>(*q))) code += static_cast<char>(std::toupper(static_cast<unsigned char>(*q)));
    // Search when the code changes (as soon as it is recognized), or on Enter / the button.
    if (code == s.last_code && !force) return;
    s.last_code = code;
    const Airport* apt = (code.size() == 3 || code.size() == 4) ? find_airport(code) : nullptr;
    if (apt) {
        apply_airport(s, apt);
    } else if (force || code.size() == 4) {
        s.apt = nullptr;
        s.sel = -1;
        if (code.size() != 3 && code.size() != 4)
            s.status = "Enter a 3-letter IATA or 4-letter ICAO code.";
        else
            s.status = fmt("%s not found. Searched the built-in database and %d of %d X-Plane apt.dat files. "
                           "Enter the values below by hand.", code.c_str(), apt_files_opened(), apt_files_searched());
    }
}

void draw_inputs(State& s) {
    if (section("Aircraft")) {
        if (ImGui::InputText("Weight kg", s.weight_txt, sizeof s.weight_txt, ImGuiInputTextFlags_CharsDecimal)) {
            std::string digits;
            for (const char* q = s.weight_txt; *q; ++q)
                if (std::isdigit(static_cast<unsigned char>(*q))) digits += *q;
            digits = digits.substr(0, 6);
            if (digits.empty()) s.weight.reset();
            else s.weight = clampi(std::atoi(digits.c_str()), 1, 120000);
        }
        if (!s.weight) ctext(C_MUTED, "Leave empty to see weight limits only.");
        else ctext(C_MUTED, fmt("= %d lb", rnd(*s.weight * KG2LB)));
        radio_row<int>("Flaps", {{1, "1"}, {5, "5"}, {15, "15"}}, s.flaps);
        ImGui::Checkbox("Engine anti-ice", &s.ai);
    }

    if (section("Thrust")) {
        std::string t0 = fmt("%s %s", derate_name(0), derate_k(0)), t1 = fmt("%s %s", derate_name(1), derate_k(1)),
                    t2 = fmt("%s %s", derate_name(2), derate_k(2));
        radio_row<int>("Fixed derate", {{0, t0.c_str()}, {1, t1.c_str()}, {2, t2.c_str()}}, s.derate);
        int flex = s.flex_mode ? 1 : 0;
        radio_row<int>("Takeoff thrust", {{1, "Flex (assumed temp)"}, {0, "No flex"}}, flex);
        s.flex_mode = flex == 1;
        ctext(C_MUTED, "Flex uses the highest assumed temperature the runway and climb allow.");
        int mx = s.v1_max ? 1 : 0;
        radio_row<int>("V1 method", {{0, "Balanced field"}, {1, "Maximum"}}, mx);
        s.v1_max = mx == 1;
    }

    if (section("Airport and runway")) {
        bool enter = ImGui::InputText("ICAO or IATA", s.icao, sizeof s.icao,
                                      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsUppercase |
                                          ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SameLine();
        bool pressed = ImGui::Button("Look up");
        lookup(s, enter || pressed);
        wrapped(C_MUTED, s.status);
        if (s.apt) {
            std::vector<std::string> labels;
            for (const auto& e : s.apt->ends)
                labels.push_back(fmt("%s   %d m / %d ft   %03d", e.id.c_str(), rnd(e.len), rnd(e.len * M2FT),
                                     s.auto_wx ? e.tru : e.mag));
            const char* preview = s.sel >= 0 && s.sel < static_cast<int>(labels.size()) ? labels[s.sel].c_str() : "Select runway";
            if (ImGui::BeginCombo("Runway", preview)) {
                for (int i = -1; i < static_cast<int>(labels.size()); ++i) {
                    const char* text = i < 0 ? "Select runway" : labels[i].c_str();
                    if (ImGui::Selectable(text, s.sel == i)) {
                        s.sel = i;
                        if (i >= 0) {
                            const RunwayEnd& e = s.apt->ends[i];
                            s.rwy_len = rnd(e.len);
                            s.slope10 = rnd(e.slope * 10);
                        }
                    }
                }
                ImGui::EndCombo();
            }
            if (s.sel >= 0) {
                const RunwayEnd& e = s.apt->ends[s.sel];
                s.rwy_hdg = s.auto_wx ? e.tru : e.mag;
            }
        }
        ImGui::InputInt("Elevation ft", &s.elev_ft, 10, 100);
        if (ImGui::InputInt("Runway avail m", &s.rwy_len, 50, 500)) s.rwy_len = clampi(s.rwy_len, 0, 6000);
        if (ImGui::InputInt("Runway heading", &s.rwy_hdg, 1, 10)) {
            s.rwy_hdg = ((s.rwy_hdg - 1) % 360 + 360) % 360 + 1;
            s.sel = -1;
        }
        ImGui::InputInt("Slope x0.1% (+up)", &s.slope10, 1, 5);
        ImGui::Checkbox("Wet runway", &s.wet);
        ctext(C_MUTED, "Length is the full paved length. Lower it for intersection takeoffs.");
    }

    if (section("Weather")) {
        ImGui::Checkbox("Use sim weather", &s.auto_wx);
        if (s.auto_wx) {
            ctext(C_TEXT, fmt("OAT %d C    Wind %03d / %d kt", s.oat, s.wdir, s.wspd));
        } else {
            ImGui::InputInt("OAT C", &s.oat, 1, 5);
            int unit = s.qnh_inhg ? 1 : 0;
            radio_row<int>("QNH unit", {{0, "hPa"}, {1, "inHg"}}, unit);
            s.qnh_inhg = unit == 1;
            if (s.qnh_inhg) {
                if (ImGui::InputInt("Altimeter A (inHg x100)", &s.altim, 1, 10)) {
                    s.altim = clampi(s.altim, 2600, 3200);
                    s.qnh = rnd(s.altim / 100.0 * 33.8639);
                }
            } else {
                if (ImGui::InputInt("QNH hPa", &s.qnh, 1, 10)) {
                    s.qnh = clampi(s.qnh, 880, 1100);
                    s.altim = rnd(s.qnh / 33.8639 * 100);
                }
            }
            ImGui::InputInt("Wind from", &s.wdir, 1, 10);
            if (ImGui::InputInt("Wind kt", &s.wspd, 1, 5)) s.wspd = std::max(0, s.wspd);
        }
        ctext(C_TEXT, fmt("QNH %d hPa  =  %.2f inHg  (A%04d)", s.qnh, s.qnh / 33.8639, rnd(s.qnh / 33.8639 * 100)));
    }
}

}  // namespace

void ui_init_style() {
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImFontConfig cfg;
    cfg.SizePixels = 15.0f;
    io.Fonts->AddFontDefault(&cfg);
    ImFontConfig big;
    big.SizePixels = 46.0f;
    g_big = io.Fonts->AddFontDefault(&big);

    ImGuiStyle& st = ImGui::GetStyle();
    ImGui::StyleColorsDark(&st);
    st.WindowRounding = 0; st.FrameRounding = 5; st.GrabRounding = 5; st.PopupRounding = 5;
    st.WindowPadding = ImVec2(12, 10);
    st.FramePadding = ImVec2(7, 5);
    st.ItemSpacing = ImVec2(8, 7);
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.10f, 0.13f, 0.97f);
    c[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.13f, 0.17f, 0.98f);
    c[ImGuiCol_Text] = C_TEXT;
    c[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.18f, 0.23f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.25f, 0.32f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.25f, 0.32f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.15f, 0.36f, 0.60f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.46f, 0.74f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.46f, 0.74f, 1.0f);
    c[ImGuiCol_CheckMark] = C_ACCENT;
    c[ImGuiCol_Header] = ImVec4(0.14f, 0.22f, 0.31f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.30f, 0.42f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.30f, 0.42f, 1.0f);
    c[ImGuiCol_Separator] = ImVec4(0.20f, 0.28f, 0.36f, 1.0f);
}

void ui_draw(State& s, const SimWx* wx) {
    if (s.auto_wx && wx) {
        if (wx->oat) s.oat = *wx->oat;
        if (wx->qnh_hpa) { s.qnh = *wx->qnh_hpa; s.altim = rnd(*wx->qnh_hpa / 33.8639 * 100); }
        if (wx->wspd) s.wspd = *wx->wspd;
        if (wx->wdir) s.wdir = *wx->wdir;
    }
    Calc c = compute(s);

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##b738perf", nullptr, flags)) {
        ImGui::SetWindowFontScale(1.2f);
        ctext(C_TEXT, "737-800 TAKEOFF PERFORMANCE");
        ImGui::SetWindowFontScale(1.0f);
        ctext(C_MUTED, "CFM56-7B26  |  approximate model, not for real-world use");
        ImGui::Separator();
        if (!c.has) draw_no_weight(s, c);
        else draw_results(s, c);
        ImGui::Spacing();
        draw_inputs(s);
    }
    ImGui::End();
}

}  // namespace pfm
