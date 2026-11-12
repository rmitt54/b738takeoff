#include "imgui_xp.h"

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstdint>

#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include "imgui.h"
#include "ui.h"

#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace pfm {

namespace {

// X-Plane virtual key codes (XPLMDefs.h), written as numbers to avoid macro name differences.
ImGuiKey map_vkey(unsigned char vk) {
    switch (vk) {
        case 0x08: return ImGuiKey_Backspace;
        case 0x09: return ImGuiKey_Tab;
        case 0x0D: return ImGuiKey_Enter;
        case 0x6C: return ImGuiKey_KeypadEnter;
        case 0x1B: return ImGuiKey_Escape;
        case 0x20: return ImGuiKey_Space;
        case 0x21: return ImGuiKey_PageUp;
        case 0x22: return ImGuiKey_PageDown;
        case 0x23: return ImGuiKey_End;
        case 0x24: return ImGuiKey_Home;
        case 0x25: return ImGuiKey_LeftArrow;
        case 0x26: return ImGuiKey_UpArrow;
        case 0x27: return ImGuiKey_RightArrow;
        case 0x28: return ImGuiKey_DownArrow;
        case 0x2D: return ImGuiKey_Insert;
        case 0x2E: return ImGuiKey_Delete;
        case 0x41: return ImGuiKey_A;
        case 0x43: return ImGuiKey_C;
        case 0x56: return ImGuiKey_V;
        case 0x58: return ImGuiKey_X;
        case 0x59: return ImGuiKey_Y;
        case 0x5A: return ImGuiKey_Z;
        default: return ImGuiKey_None;
    }
}

}  // namespace

XpImguiWindow::XpImguiWindow(const char* title, int width, int height, std::function<bool()> build)
    : build_(std::move(build)) {
    ctx_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx_);
    ui_init_style();

    int l, t, r, b;
    XPLMGetScreenBoundsGlobal(&l, &t, &r, &b);
    XPLMCreateWindow_t p;
    p.structSize = sizeof(p);
    p.left = l + 80;
    p.top = t - 80;
    p.right = p.left + width;
    p.bottom = p.top - height;
    if (p.bottom < b + 40) p.bottom = b + 40;
    p.visible = 1;
    p.drawWindowFunc = draw_cb;
    p.handleMouseClickFunc = mouse_cb;
    p.handleRightClickFunc = right_mouse_cb;
    p.handleKeyFunc = key_cb;
    p.handleCursorFunc = cursor_cb;
    p.handleMouseWheelFunc = wheel_cb;
    p.refcon = this;
    // The window draws its own title bar, close button and resize grip (see ui.cpp).
    p.decorateAsFloatingWindow = xplm_WindowDecorationSelfDecorated;
    p.layer = xplm_WindowLayerFloatingWindows;
    win_ = XPLMCreateWindowEx(&p);
    XPLMSetWindowTitle(win_, title);
    XPLMSetWindowResizingLimits(win_, 460, 520, 1400, 2000);
    last_time_ = XPLMGetElapsedTime();
}

XpImguiWindow::~XpImguiWindow() {
    if (win_) XPLMDestroyWindow(win_);
    if (ctx_) {
        ImGui::SetCurrentContext(ctx_);
        ImGui::DestroyContext(ctx_);
    }
}

void XpImguiWindow::set_visible(bool v) {
    XPLMSetWindowIsVisible(win_, v ? 1 : 0);
    if (v) XPLMBringWindowToFront(win_);
}

bool XpImguiWindow::visible() const { return win_ && XPLMGetWindowIsVisible(win_) != 0; }

void XpImguiWindow::upload_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
    int id = 0;
    XPLMGenerateTextureNumbers(&id, 1);
    XPLMBindTexture2d(id, 0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    font_tex_ = static_cast<unsigned>(id);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)id);
}

void XpImguiWindow::draw() {
    ImGui::SetCurrentContext(ctx_);
    ImGuiIO& io = ImGui::GetIO();
    if (!font_tex_) upload_fonts();

    int l, t, r, b;
    XPLMGetWindowGeometry(win_, &l, &t, &r, &b);
    io.DisplaySize = ImVec2(static_cast<float>(r - l), static_cast<float>(t - b));
    float now = XPLMGetElapsedTime();
    io.DeltaTime = now - last_time_ > 1e-4f ? now - last_time_ : 1e-4f;
    last_time_ = now;

    int mx, my;
    XPLMGetMouseLocationGlobal(&mx, &my);
    bool inside = mx >= l && mx < r && my > b && my <= t;
    if (inside || left_down_) io.AddMousePosEvent(static_cast<float>(mx - l), static_cast<float>(t - my));
    else io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);

    ImGui::NewFrame();
    bool close = build_ ? build_() : false;
    ImGui::Render();
    render(l, t);
    if (close) {
        if (XPLMHasKeyboardFocus(win_)) XPLMTakeKeyboardFocus(nullptr);
        XPLMSetWindowIsVisible(win_, 0);
        return;
    }

    // Take the keyboard only while a text box is active, and give it back afterwards.
    if (io.WantTextInput) {
        if (!XPLMHasKeyboardFocus(win_)) XPLMTakeKeyboardFocus(win_);
    } else if (XPLMHasKeyboardFocus(win_)) {
        XPLMTakeKeyboardFocus(nullptr);
    }
}

void XpImguiWindow::render(int left, int top) {
    ImDrawData* dd = ImGui::GetDrawData();
    if (!dd || dd->CmdListsCount == 0) return;

    int vp[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, vp);
    int sw = 0, sh = 0;
    XPLMGetScreenSize(&sw, &sh);
    float sx = (sw > 0 && vp[2] > 0) ? static_cast<float>(vp[2]) / sw : 1.0f;
    float sy = (sh > 0 && vp[3] > 0) ? static_cast<float>(vp[3]) / sh : 1.0f;

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    XPLMSetGraphicsState(0, 1, 0, 0, 1, 0, 0);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    // Other plugins may leave their own matrices behind. Never trust the incoming
    // state: set up our own projection (main window only; popped-out/VR windows get
    // theirs from X-Plane), a clean texture matrix, and a clean modelview.
    const bool own_projection = win_ && !XPLMWindowIsPoppedOut(win_) && !XPLMWindowIsInVR(win_) && sw > 0 && sh > 0;
    if (own_projection) {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, sw, 0.0, sh, -1.0, 1.0);
    }
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glLoadIdentity();
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    // ImGui has y pointing down from the window's top-left; X-Plane has y pointing up.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(static_cast<float>(left), static_cast<float>(top), 0.0f);
    glScalef(1.0f, -1.0f, 1.0f);

    for (int n = 0; n < dd->CmdListsCount; ++n) {
        const ImDrawList* list = dd->CmdLists[n];
        const ImDrawVert* vtx = list->VtxBuffer.Data;
        const ImDrawIdx* idx = list->IdxBuffer.Data;
        glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), reinterpret_cast<const char*>(vtx) + offsetof(ImDrawVert, pos));
        glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), reinterpret_cast<const char*>(vtx) + offsetof(ImDrawVert, uv));
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), reinterpret_cast<const char*>(vtx) + offsetof(ImDrawVert, col));
        for (int i = 0; i < list->CmdBuffer.Size; ++i) {
            const ImDrawCmd* cmd = &list->CmdBuffer[i];
            if (cmd->UserCallback) continue;
            float x0 = cmd->ClipRect.x, y0 = cmd->ClipRect.y, x1 = cmd->ClipRect.z, y1 = cmd->ClipRect.w;
            if (x1 <= x0 || y1 <= y0) continue;
            glScissor(static_cast<int>((left + x0) * sx), static_cast<int>((top - y1) * sy),
                      static_cast<int>((x1 - x0) * sx), static_cast<int>((y1 - y0) * sy));
            XPLMBindTexture2d(static_cast<int>((intptr_t)cmd->GetTexID()), 0);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cmd->ElemCount),
                           sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx + cmd->IdxOffset);
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    if (own_projection) {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
    glPopClientAttrib();
    glPopAttrib();
}

void XpImguiWindow::mouse_event(int x, int y, XPLMMouseStatus status, int button) {
    ImGui::SetCurrentContext(ctx_);
    ImGuiIO& io = ImGui::GetIO();
    int l, t, r, b;
    XPLMGetWindowGeometry(win_, &l, &t, &r, &b);

    // Dragging the header moves the window; dragging the bottom-right grip resizes it.
    if (button == 0) {
        if (status == xplm_MouseDown) {
            float lx = static_cast<float>(x - l), ly = static_cast<float>(t - y);
            float w = static_cast<float>(r - l), h = static_cast<float>(t - b);
            drag_mode_ = 0;
            if (lx >= w - kGripSize && ly >= h - kGripSize) drag_mode_ = 2;
            else if (ly < kHeaderHeight && lx < w - kCloseZoneWidth) drag_mode_ = 1;
            if (drag_mode_) {
                drag_gx_ = x; drag_gy_ = y;
                drag_l_ = l; drag_t_ = t; drag_r_ = r; drag_b_ = b;
                left_down_ = true;
                return;
            }
        } else if (drag_mode_) {
            if (status == xplm_MouseDrag) {
                int dx = x - drag_gx_, dy = y - drag_gy_;
                if (drag_mode_ == 1) {
                    XPLMSetWindowGeometry(win_, drag_l_ + dx, drag_t_ + dy, drag_r_ + dx, drag_b_ + dy);
                } else {
                    int nr = std::max(drag_r_ + dx, drag_l_ + 460);
                    int nb = std::min(drag_b_ + dy, drag_t_ - 360);
                    XPLMSetWindowGeometry(win_, drag_l_, drag_t_, nr, nb);
                }
            } else if (status == xplm_MouseUp) {
                drag_mode_ = 0;
                left_down_ = false;
            }
            return;
        }
    }

    io.AddMousePosEvent(static_cast<float>(x - l), static_cast<float>(t - y));
    if (status == xplm_MouseDown) io.AddMouseButtonEvent(button, true);
    else if (status == xplm_MouseUp) io.AddMouseButtonEvent(button, false);
    if (button == 0) left_down_ = status != xplm_MouseUp;
}

void XpImguiWindow::draw_cb(XPLMWindowID, void* ref) { static_cast<XpImguiWindow*>(ref)->draw(); }

int XpImguiWindow::mouse_cb(XPLMWindowID, int x, int y, XPLMMouseStatus status, void* ref) {
    static_cast<XpImguiWindow*>(ref)->mouse_event(x, y, status, 0);
    return 1;
}

int XpImguiWindow::right_mouse_cb(XPLMWindowID, int x, int y, XPLMMouseStatus status, void* ref) {
    static_cast<XpImguiWindow*>(ref)->mouse_event(x, y, status, 1);
    return 1;
}

void XpImguiWindow::key_cb(XPLMWindowID, char key, XPLMKeyFlags flags, char vkey, void* ref, int losing_focus) {
    auto* self = static_cast<XpImguiWindow*>(ref);
    ImGui::SetCurrentContext(self->ctx_);
    ImGuiIO& io = ImGui::GetIO();
    if (losing_focus) {
        io.ClearInputKeys();
        return;
    }
    bool down = (flags & xplm_DownFlag) != 0;
    bool ctrl = (flags & xplm_ControlFlag) != 0;
    io.AddKeyEvent(ImGuiMod_Ctrl, ctrl);
    io.AddKeyEvent(ImGuiMod_Shift, (flags & xplm_ShiftFlag) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (flags & xplm_OptionAltFlag) != 0);
    ImGuiKey k = map_vkey(static_cast<unsigned char>(vkey));
    if (k != ImGuiKey_None) io.AddKeyEvent(k, down);
    unsigned char c = static_cast<unsigned char>(key);
    if (down && !ctrl && c >= 32 && c < 127) io.AddInputCharacter(c);
}

XPLMCursorStatus XpImguiWindow::cursor_cb(XPLMWindowID, int, int, void*) { return xplm_CursorArrow; }

int XpImguiWindow::wheel_cb(XPLMWindowID, int, int, int wheel, int clicks, void* ref) {
    auto* self = static_cast<XpImguiWindow*>(ref);
    ImGui::SetCurrentContext(self->ctx_);
    ImGuiIO& io = ImGui::GetIO();
    if (wheel == 0) io.AddMouseWheelEvent(0.0f, static_cast<float>(clicks));
    else io.AddMouseWheelEvent(static_cast<float>(clicks), 0.0f);
    return 1;
}

}  // namespace pfm
