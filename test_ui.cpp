// Headless run of the UI: builds the font atlas, runs frames with a fake display, checks draw output.
#include <cstdio>
#include <cstring>
#include "../src/ui.h"
#include "imgui.h"
int main() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    pfm::ui_init_style();
    unsigned char* px; int w, h;
    io.Fonts->GetTexDataAsRGBA32(&px, &w, &h);
    io.DisplaySize = ImVec2(520, 900);
    pfm::State s;
    pfm::SimWx wx; wx.oat = 12; wx.qnh_hpa = 1008; wx.wspd = 10; wx.wdir = 300;
    int frames = 0;
    auto frame = [&]() {
        io.DeltaTime = 1.0f / 60;
        ImGui::NewFrame();
        pfm::ui_draw(s, &wx);
        ImGui::Render();
        ++frames;
        return ImGui::GetDrawData()->TotalVtxCount;
    };
    int v0 = frame(); frame();
    std::printf("empty weight: vtx %d status='%.40s'\n", v0, s.status.c_str());
    std::strcpy(s.icao, "CYYZ"); frame(); frame();
    std::printf("typed CYYZ: apt=%s elev=%d\n", s.apt ? s.apt->icao.c_str() : "none", s.elev_ft);
    s.sel = 2; s.weight = 68000; std::strcpy(s.weight_txt, "68000");
    s.rwy_len = 3389;
    int v1 = frame();
    std::printf("with weight: vtx %d\n", v1);
    std::strcpy(s.icao, "ZZZZ"); frame();
    std::printf("typed ZZZZ: apt=%s status='%.30s'\n", s.apt ? "set" : "none", s.status.c_str());
    s.weight = 90000; frame(); s.weight = 79000; s.rwy_len = 800; frame(); s.auto_wx = false; frame();
    s.qnh_inhg = true; s.flex_mode = false; s.derate = 2; frame();
    std::printf("frames ok: %d\n", frames);
    ImGui::DestroyContext();
}
