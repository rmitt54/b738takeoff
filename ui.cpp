#include "ui.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <string>
#include <vector>

#include "imgui.h"

extern const unsigned int kRoboto_compressed_size;
extern const unsigned int kRoboto_compressed_data[];

namespace pfm {

namespace {

// Palette: a cool slate background with one blue accent and green, amber and red for status.
const ImVec4 C_WINDOW(0.058f, 0.075f, 0.098f, 0.98f);
const ImVec4 C_CARD(0.090f, 0.118f, 0.153f, 1.0f);
const ImVec4 C_FIELD(0.125f, 0.160f, 0.204f, 1.0f);
const ImVec4 C_FIELD_HOT(0.165f, 0.208f, 0.262f, 1.0f);
const ImVec4 C_LINE(0.160f, 0.200f, 0.250f, 1.0f);
const ImVec4 C_TEXT(0.925f, 0.945f, 0.965f, 1.0f);
const ImVec4 C_MUTED(0.545f, 0.610f, 0.680f, 1.0f);
const ImVec4 C_ACCENT(0.310f, 0.640f, 1.000f, 1.0f);
const ImVec4 C_GOOD(0.240f, 0.860f, 0.600f, 1.0f);
const ImVec4 C_WARN(1.000f, 0.710f, 0.330f, 1.0f);
const ImVec4 C_BAD(1.000f, 0.420f, 0.420f, 1.0f);

ImFont* g_body = nullptr;
ImFont* g_small = nullptr;
ImFont* g_title = nullptr;
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

ImU32 u32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }
ImVec4 mix(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, 1.0f);
}

void ctext(const ImVec4& c, const std::string& t) { ImGui::TextColored(c, "%s", t.c_str()); }

void small_text(const ImVec4& c, const std::string& t) {
    if (g_small) ImGui::PushFont(g_small);
    ImGui::TextColored(c, "%s", t.c_str());
    if (g_small) ImGui::PopFont();
}

void wrapped(const ImVec4& c, const std::string& t, bool small = false) {
    if (small && g_small) ImGui::PushFont(g_small);
    ImGui::PushStyleColor(ImGuiCol_Text, c);
    ImGui::TextWrapped("%s", t.c_str());
    ImGui::PopStyleColor();
    if (small && g_small) ImGui::PopFont();
}

// A rounded status label. Draws its own background and text.
void pill(const std::string& text, const ImVec4& c) {
    if (g_small) ImGui::PushFont(g_small);
    ImVec2 ts = ImGui::CalcTextSize(text.c_str());
    ImVec2 pad(11, 5);
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 size(ts.x + pad.x * 2, ts.y + pad.y * 2);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, ImVec2(p0.x + size.x, p0.y + size.y), u32(mix(C_WINDOW, c, 0.22f)), size.y * 0.5f);
    dl->AddRect(p0, ImVec2(p0.x + size.x, p0.y + size.y), u32(mix(C_WINDOW, c, 0.45f)), size.y * 0.5f, 0, 1.0f);
    dl->AddText(ImVec2(p0.x + pad.x, p0.y + pad.y), u32(c), text.c_str());
    ImGui::Dummy(size);
    if (g_small) ImGui::PopFont();
}

void centered_in(float width, const std::string& text, const ImVec4& c) {
    float w = ImGui::CalcTextSize(text.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (width - w) * 0.5f));
    ctext(c, text);
}

// One row of buttons where the chosen one is filled with the accent color.
template <class T>
bool segmented(const char* id, std::initializer_list<std::pair<T, const char*>> options, T& cur) {
    bool changed = false;
    ImGui::PushID(id);
    int i = 0;
    for (const auto& o : options) {
        if (i++) ImGui::SameLine(0, 6);
        bool on = cur == o.first;
        ImGui::PushStyleColor(ImGuiCol_Button, on ? mix(C_FIELD, C_ACCENT, 0.80f) : C_FIELD);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, on ? mix(C_FIELD, C_ACCENT, 0.92f) : C_FIELD_HOT);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, on ? C_ACCENT : C_FIELD_HOT);
        ImGui::PushStyleColor(ImGuiCol_Text, on ? ImVec4(0.03f, 0.06f, 0.10f, 1.0f) : C_MUTED);
        if (ImGui::Button(o.second)) {
            cur = o.first;
            changed = true;
        }
        ImGui::PopStyleColor(4);
    }
    ImGui::PopID();
    return changed;
}

// Form: a muted label on the left, the control filling the right.
bool form_begin(const char* id) {
    if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_SizingFixedFit)) return false;
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, 150.0f);
    ImGui::TableSetupColumn("control", ImGuiTableColumnFlags_WidthStretch);
    return true;
}

void form_row(const char* label) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ctext(C_MUTED, label);
    ImGui::TableSetColumnIndex(1);
}

void fill_width() { ImGui::SetNextItemWidth(-FLT_MIN); }

bool section(const char* title) {
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::PushStyleColor(ImGuiCol_Header, C_CARD);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, C_FIELD);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, C_FIELD);
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
    bool open = ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor(4);
    return open;
}

struct Row { std::string label, value; ImVec4 color; };

void stat_grid(const std::vector<Row>& rows) {
    if (!ImGui::BeginTable("##stats", 2, ImGuiTableFlags_SizingStretchProp)) return;
    ImGui::TableSetupColumn("a", ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn("b", ImGuiTableColumnFlags_WidthStretch, 1.3f);
    for (const auto& r : rows) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ctext(C_MUTED, r.label);
        ImGui::TableSetColumnIndex(1);
        float w = ImGui::CalcTextSize(r.value.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - w));
        ctext(r.color, r.value);
    }
    ImGui::EndTable();
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
    // The runway heading is magnetic. Wind from the simulator is relative to true north, so add the
    // magnetic variation (positive east) to get the true heading. Wind typed in by hand follows the
    // ATIS, which is magnetic, so it is used as is.
    double var = (s.apt && s.apt->has_magvar) ? s.apt->magvar : (has_default_magvar() ? default_magvar() : 0.0);
    double hdg = s.rwy_hdg + (s.auto_wx ? var : 0.0);
    double ang = (s.wdir - hdg) * 3.14159265358979323846 / 180.0;
    c.p.flaps = s.flaps; c.p.ai = s.ai; c.p.packs = s.packs; c.p.derate = s.derate; c.p.v1_max = s.v1_max;
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

// A rounded card holding one speed.
void speed_card(const char* id, const char* label, const std::string& value, const ImVec4& color, float w) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, C_CARD);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::BeginChild(id, ImVec2(w, 128), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Dummy(ImVec2(0, 4));
    if (g_small) ImGui::PushFont(g_small);
    centered_in(w, label, C_MUTED);
    if (g_small) ImGui::PopFont();
    if (g_big) ImGui::PushFont(g_big);
    centered_in(w, value, color);
    if (g_big) ImGui::PopFont();
    if (g_small) ImGui::PushFont(g_small);
    centered_in(w, "KIAS", C_MUTED);
    if (g_small) ImGui::PopFont();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void runway_bar(double frac, const ImVec4& col, const std::string& left, const std::string& right) {
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x, h = 10.0f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, ImVec2(p0.x + w, p0.y + h), u32(C_FIELD), h * 0.5f);
    float fw = std::max(h, static_cast<float>(w * frac));
    dl->AddRectFilled(p0, ImVec2(p0.x + fw, p0.y + h), u32(col), h * 0.5f);
    ImGui::Dummy(ImVec2(w, h + 2));
    ctext(C_MUTED, left);
    ImGui::SameLine();
    float tw = ImGui::CalcTextSize(right.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, ImGui::GetContentRegionAvail().x - tw));
    ctext(C_TEXT, right);
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
    ImGui::SameLine(0, 8);
    pill(fmt("%d%% THRUST", pct), C_MUTED);
    ImGui::SameLine(0, 8);
    pill(fmt("N1 %.1f", r.n1), C_MUTED);
    ImGui::SameLine(0, 8);
    if (W > MTOW) pill("OVER MTOW", C_BAD);
    else if (!r.fieldOk) pill("RUNWAY LIMITED", C_BAD);
    else if (!r.climbOk) pill("CLIMB LIMITED", C_BAD);
    else pill("READY", C_GOOD);
    if (s.flex_mode && !c.flex_usable) wrapped(C_WARN, "Flex is not available here, so the fixed rating is used.", true);

    ImGui::Dummy(ImVec2(0, 4));

    float gap = 10.0f;
    float w = (ImGui::GetContentRegionAvail().x - 2 * gap) / 3.0f;
    speed_card("v1", "V1", show ? fmt("%d", r.V1) : "---", show ? C_ACCENT : C_BAD, w);
    ImGui::SameLine(0, gap);
    speed_card("vr", "VR", show ? fmt("%d", r.VR) : "---", show ? C_TEXT : C_BAD, w);
    ImGui::SameLine(0, gap);
    speed_card("v2", "V2", show ? fmt("%d", r.V2) : "---", show ? C_GOOD : C_BAD, w);
    small_text(C_MUTED, fmt("Flaps %d  |  indicated airspeed", s.flaps));

    if (!show) {
        ImGui::Spacing();
        if (W > MTOW) {
            wrapped(C_BAD, "Weight is above the maximum takeoff weight.");
        } else {
            auto mt = limit_w_both(p, c.T);
            std::string why = !r.fieldOk ? "Runway limited" : fmt("Climb limited (net %.1f%%, need 2.4%%)", r.climbNet);
            if (mt) wrapped(C_BAD, fmt("%s. Heaviest weight that works here: %s.", why.c_str(), kg_text(mt).c_str()));
            else wrapped(C_BAD, fmt("%s at any weight.", why.c_str()));
        }
    }

    ImGui::Dummy(ImVec2(0, 6));

    double frac = p.tora > 0 ? clamp(r.todr / p.tora, 0, 1) : 1.0;
    ImVec4 col = r.todr > p.tora ? C_BAD : (r.todr > 0.92 * p.tora ? C_WARN : C_ACCENT);
    runway_bar(frac, col, "Runway used", fmt("%d%%", rnd(r.todr / std::max(1.0, p.tora) * 100)));

    ImGui::Dummy(ImVec2(0, 4));
    ImVec4 ccol = r.climbNet < 2.4 ? C_BAD : (r.climbNet < 3 ? C_WARN : C_GOOD);
    stat_grid({
        {"Runway needed", fmt_len(r.todr), C_TEXT},
        {"Runway available", fmt_len(p.tora), C_TEXT},
        {"Engine-out climb", fmt("net %.1f%%  (min 2.4%%)", r.climbNet), ccol},
        {"Runway weight limit", kg_text(limit_w(p, c.T, false)), C_TEXT},
        {"Climb weight limit", kg_text(limit_w(p, c.T, true)), C_TEXT},
        {"Headwind / crosswind", fmt("%d kt / %d kt", rnd(p.hw), rnd(std::fabs(p.xw))), C_TEXT},
    });

    if (r.v1floor) wrapped(C_WARN, fmt("V1 is held at the Vmcg floor (%d kt). Runway is critical.", r.vmcg), true);
    if (r.gs > 195) wrapped(C_WARN, "Ground speed at VR is above the tire speed limit.", true);
    if (std::fabs(p.xw) > 33) wrapped(C_WARN, "Crosswind is above the demonstrated 33 kt.", true);
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
    if (section("Aircraft") && form_begin("##aircraft")) {
        form_row("Takeoff weight, kg");
        fill_width();
        if (ImGui::InputTextWithHint("##weight", "e.g. 68000", s.weight_txt, sizeof s.weight_txt,
                                     ImGuiInputTextFlags_CharsDecimal)) {
            std::string digits;
            for (const char* q = s.weight_txt; *q; ++q)
                if (std::isdigit(static_cast<unsigned char>(*q))) digits += *q;
            digits = digits.substr(0, 6);
            if (digits.empty()) s.weight.reset();
            else s.weight = clampi(std::atoi(digits.c_str()), 1, 120000);
        }
        form_row("");
        if (!s.weight) wrapped(C_MUTED, "Leave empty to see weight limits only.", true);
        else wrapped(C_MUTED, fmt("= %d lb", rnd(*s.weight * KG2LB)), true);
        form_row("Flaps");
        segmented<int>("flaps", {{1, "  1  "}, {5, "  5  "}, {15, "  15  "}}, s.flaps);
        form_row("Anti-ice");
        ImGui::Checkbox("Engine anti-ice", &s.ai);
        form_row("Air conditioning");
        int packs = s.packs ? 1 : 0;
        segmented<int>("packs", {{0, "Packs off"}, {1, "Packs on"}}, packs);
        s.packs = packs == 1;
        form_row("");
        wrapped(C_MUTED, "Packs on lowers available thrust about 0.5% (estimate). Packs off is the calibrated case.", true);
        ImGui::EndTable();
    }

    if (section("Thrust") && form_begin("##thrust")) {
        form_row("Rating");
        std::string t0 = fmt("%s  %s", derate_name(0), derate_k(0)), t1 = fmt("%s  %s", derate_name(1), derate_k(1)),
                    t2 = fmt("%s  %s", derate_name(2), derate_k(2));
        segmented<int>("derate", {{0, t0.c_str()}, {1, t1.c_str()}, {2, t2.c_str()}}, s.derate);
        form_row("Takeoff thrust");
        int flex = s.flex_mode ? 1 : 0;
        segmented<int>("flex", {{1, "Flex"}, {0, "No flex"}}, flex);
        s.flex_mode = flex == 1;
        form_row("V1 method");
        int mx = s.v1_max ? 1 : 0;
        segmented<int>("v1", {{0, "Balanced field"}, {1, "Maximum"}}, mx);
        s.v1_max = mx == 1;
        form_row("");
        wrapped(C_MUTED, "Flex uses the highest assumed temperature the runway and climb allow.", true);
        ImGui::EndTable();
    }

    if (section("Airport and runway") && form_begin("##airport")) {
        form_row("ICAO or IATA");
        ImGui::SetNextItemWidth(-92.0f);
        bool enter = ImGui::InputTextWithHint("##icao", "e.g. CYYZ", s.icao, sizeof s.icao,
                                              ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsUppercase |
                                                  ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SameLine(0, 8);
        bool pressed = ImGui::Button("Look up", ImVec2(84, 0));
        lookup(s, enter || pressed);
        form_row("");
        wrapped(C_MUTED, s.status, true);
        if (s.apt) {
            std::vector<std::string> labels;
            for (const auto& e : s.apt->ends)
                labels.push_back(fmt("%s   %d m / %d ft   %03d M", e.id.c_str(), rnd(e.len), rnd(e.len * M2FT), e.mag));
            const char* preview = s.sel >= 0 && s.sel < static_cast<int>(labels.size()) ? labels[s.sel].c_str() : "Select runway";
            form_row("Runway");
            fill_width();
            if (ImGui::BeginCombo("##runway", preview)) {
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
                s.rwy_hdg = e.mag;
            }
        }
        form_row("Elevation, ft");
        fill_width();
        ImGui::InputInt("##elev", &s.elev_ft, 0, 0);
        form_row("Runway available, m");
        fill_width();
        if (ImGui::InputInt("##rwylen", &s.rwy_len, 0, 0)) s.rwy_len = clampi(s.rwy_len, 0, 6000);
        form_row("Runway heading, mag");
        fill_width();
        if (ImGui::InputInt("##hdg", &s.rwy_hdg, 0, 0)) {
            s.rwy_hdg = ((s.rwy_hdg - 1) % 360 + 360) % 360 + 1;
            s.sel = -1;
        }
        form_row("Slope, x0.1% (+ up)");
        fill_width();
        ImGui::InputInt("##slope", &s.slope10, 0, 0);
        form_row("Surface");
        ImGui::Checkbox("Wet runway", &s.wet);
        form_row("");
        wrapped(C_MUTED, "Length is the full paved length. Lower it for intersection takeoffs.", true);
        ImGui::EndTable();
    }

    if (section("Weather") && form_begin("##weather")) {
        form_row("Source");
        ImGui::Checkbox("Use sim weather", &s.auto_wx);
        if (s.auto_wx) {
            form_row("Conditions");
            ctext(C_TEXT, fmt("OAT %d C    Wind %03d / %d kt", s.oat, s.wdir, s.wspd));
        } else {
            form_row("OAT, C");
            fill_width();
            ImGui::InputInt("##oat", &s.oat, 0, 0);
            form_row("QNH unit");
            int unit = s.qnh_inhg ? 1 : 0;
            segmented<int>("qnhunit", {{0, " hPa "}, {1, " inHg "}}, unit);
            s.qnh_inhg = unit == 1;
            if (s.qnh_inhg) {
                form_row("Altimeter, inHg x100");
                fill_width();
                if (ImGui::InputInt("##altim", &s.altim, 0, 0)) {
                    s.altim = clampi(s.altim, 2600, 3200);
                    s.qnh = rnd(s.altim / 100.0 * 33.8639);
                }
            } else {
                form_row("QNH, hPa");
                fill_width();
                if (ImGui::InputInt("##qnh", &s.qnh, 0, 0)) {
                    s.qnh = clampi(s.qnh, 880, 1100);
                    s.altim = rnd(s.qnh / 33.8639 * 100);
                }
            }
            form_row("Wind from, deg");
            fill_width();
            ImGui::InputInt("##wdir", &s.wdir, 0, 0);
            form_row("Wind speed, kt");
            fill_width();
            if (ImGui::InputInt("##wspd", &s.wspd, 0, 0)) s.wspd = std::max(0, s.wspd);
        }
        form_row("");
        small_text(C_MUTED, fmt("QNH %d hPa  =  %.2f inHg  (A%04d)", s.qnh, s.qnh / 33.8639, rnd(s.qnh / 33.8639 * 100)));
        ImGui::EndTable();
    }
}

}  // namespace

void ui_init_style() {
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // Roboto Medium at four sizes. Falls back to the built-in font if loading ever fails.
    ImFontConfig cfg;
    cfg.OversampleH = 3;
    cfg.OversampleV = 2;
    cfg.FontDataOwnedByAtlas = false;
    auto add = [&](float px) {
        return io.Fonts->AddFontFromMemoryCompressedTTF(kRoboto_compressed_data, kRoboto_compressed_size, px, &cfg);
    };
    g_body = add(17.0f);
    g_small = add(14.0f);
    g_title = add(24.0f);
    g_big = add(58.0f);
    if (!g_body) {
        ImFontConfig d;
        g_body = io.Fonts->AddFontDefault(&d);
        g_small = g_title = g_big = nullptr;
    }

    ImGuiStyle& st = ImGui::GetStyle();
    ImGui::StyleColorsDark(&st);
    st.WindowRounding = 0; st.ChildRounding = 12; st.FrameRounding = 8; st.GrabRounding = 8;
    st.PopupRounding = 8; st.ScrollbarRounding = 8; st.TabRounding = 8;
    st.WindowPadding = ImVec2(18, 16);
    st.FramePadding = ImVec2(11, 7);
    st.ItemSpacing = ImVec2(10, 9);
    st.ItemInnerSpacing = ImVec2(8, 6);
    st.CellPadding = ImVec2(6, 5);
    st.ScrollbarSize = 10;
    st.FrameBorderSize = 0; st.WindowBorderSize = 0; st.ChildBorderSize = 0; st.PopupBorderSize = 1;
    st.SeparatorTextBorderSize = 1;
    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg] = C_WINDOW;
    c[ImGuiCol_ChildBg] = C_CARD;
    c[ImGuiCol_PopupBg] = ImVec4(0.085f, 0.110f, 0.145f, 0.99f);
    c[ImGuiCol_Border] = C_LINE;
    c[ImGuiCol_Text] = C_TEXT;
    c[ImGuiCol_TextDisabled] = C_MUTED;
    c[ImGuiCol_FrameBg] = C_FIELD;
    c[ImGuiCol_FrameBgHovered] = C_FIELD_HOT;
    c[ImGuiCol_FrameBgActive] = C_FIELD_HOT;
    c[ImGuiCol_Button] = mix(C_FIELD, C_ACCENT, 0.55f);
    c[ImGuiCol_ButtonHovered] = mix(C_FIELD, C_ACCENT, 0.75f);
    c[ImGuiCol_ButtonActive] = C_ACCENT;
    c[ImGuiCol_CheckMark] = C_ACCENT;
    c[ImGuiCol_Header] = C_CARD;
    c[ImGuiCol_HeaderHovered] = C_FIELD;
    c[ImGuiCol_HeaderActive] = C_FIELD;
    c[ImGuiCol_Separator] = C_LINE;
    c[ImGuiCol_TextSelectedBg] = ImVec4(C_ACCENT.x, C_ACCENT.y, C_ACCENT.z, 0.35f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = C_FIELD_HOT;
    c[ImGuiCol_ScrollbarGrabHovered] = mix(C_FIELD_HOT, C_ACCENT, 0.4f);
    c[ImGuiCol_ScrollbarGrabActive] = C_ACCENT;
    c[ImGuiCol_TableBorderLight] = C_LINE;
}

bool ui_draw(State& s, const SimWx* wx) {
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
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    bool close = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool open = ImGui::Begin("##b738perf", nullptr, flags);
    ImGui::PopStyleVar();
    if (open) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        float W = vp->Size.x, H = vp->Size.y;

        // Header: title on the left (drag it to move the window), close button on the right.
        ImGui::SetCursorPos(ImVec2(18, 10));
        if (g_title) ImGui::PushFont(g_title);
        ctext(C_TEXT, "737-800 Takeoff Performance");
        if (g_title) ImGui::PopFont();
        ImGui::SetCursorPosX(18);
        small_text(C_MUTED, "CFM56-7B26  |  approximate model, not for real-world use");

        const float bs = 30.0f;
        ImVec2 b0(wp.x + W - bs - 14, wp.y + 12), b1(b0.x + bs, b0.y + bs);
        bool hot = ImGui::IsMouseHoveringRect(b0, b1);
        dl->AddRectFilled(b0, b1, u32(hot ? mix(C_FIELD, C_BAD, 0.55f) : C_FIELD), 9.0f);
        ImVec2 mid((b0.x + b1.x) * 0.5f, (b0.y + b1.y) * 0.5f);
        float k = 6.0f;
        ImU32 xc = u32(hot ? C_TEXT : C_MUTED);
        dl->AddLine(ImVec2(mid.x - k, mid.y - k), ImVec2(mid.x + k, mid.y + k), xc, 2.0f);
        dl->AddLine(ImVec2(mid.x - k, mid.y + k), ImVec2(mid.x + k, mid.y - k), xc, 2.0f);
        if (hot && ImGui::IsMouseClicked(0)) close = true;

        dl->AddLine(ImVec2(wp.x, wp.y + kHeaderHeight), ImVec2(wp.x + W, wp.y + kHeaderHeight), u32(C_LINE), 1.0f);

        // Body scrolls with the mouse wheel when the window is too small for everything.
        ImGui::SetCursorPos(ImVec2(0, kHeaderHeight + 1));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("##body", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_None);
        if (!c.has) draw_no_weight(s, c);
        else draw_results(s, c);
        ImGui::Dummy(ImVec2(0, 8));
        draw_inputs(s);
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // Resize grip in the bottom-right corner.
        ImVec2 g(wp.x + W - 6, wp.y + H - 6);
        ImU32 gc = u32(C_MUTED);
        for (int i = 1; i <= 3; ++i)
            dl->AddLine(ImVec2(g.x - 5.0f * i, g.y), ImVec2(g.x, g.y - 5.0f * i), gc, 1.5f);
    }
    ImGui::End();
    return close;
}

}  // namespace pfm
