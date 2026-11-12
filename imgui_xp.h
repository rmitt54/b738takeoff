// A Dear ImGui window inside X-Plane: an XPLM floating window that draws ImGui with fixed-function
// OpenGL (through XPLM's texture and state calls) and forwards mouse and keyboard input.
#pragma once
#include <functional>

#include "XPLMDisplay.h"

struct ImGuiContext;

namespace pfm {

class XpImguiWindow {
public:
    XpImguiWindow(const char* title, int width, int height, std::function<void()> build);
    ~XpImguiWindow();
    XpImguiWindow(const XpImguiWindow&) = delete;
    XpImguiWindow& operator=(const XpImguiWindow&) = delete;

    void set_visible(bool v);
    bool visible() const;

private:
    static void draw_cb(XPLMWindowID, void* ref);
    static int mouse_cb(XPLMWindowID, int x, int y, XPLMMouseStatus status, void* ref);
    static int right_mouse_cb(XPLMWindowID, int x, int y, XPLMMouseStatus status, void* ref);
    static void key_cb(XPLMWindowID, char key, XPLMKeyFlags flags, char vkey, void* ref, int losing_focus);
    static XPLMCursorStatus cursor_cb(XPLMWindowID, int x, int y, void* ref);
    static int wheel_cb(XPLMWindowID, int x, int y, int wheel, int clicks, void* ref);

    void draw();
    void upload_fonts();
    void render(int left, int top);
    void mouse_event(int x, int y, XPLMMouseStatus status, int button);

    XPLMWindowID win_ = nullptr;
    ImGuiContext* ctx_ = nullptr;
    std::function<void()> build_;
    unsigned font_tex_ = 0;
    float last_time_ = 0;
    bool left_down_ = false;
};

}  // namespace pfm
