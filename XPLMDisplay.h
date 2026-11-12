#pragma once
#include "XPLMDefs.h"
typedef int XPLMMouseStatus; enum { xplm_MouseDown = 1, xplm_MouseDrag = 2, xplm_MouseUp = 3 };
typedef int XPLMKeyFlags; enum { xplm_ShiftFlag = 1, xplm_OptionAltFlag = 2, xplm_ControlFlag = 4, xplm_DownFlag = 8, xplm_UpFlag = 16 };
typedef int XPLMCursorStatus; enum { xplm_CursorDefault = 0, xplm_CursorHidden = 1, xplm_CursorArrow = 2, xplm_CursorCustom = 3 };
typedef int XPLMWindowDecoration; enum { xplm_WindowDecorationNone = 0, xplm_WindowDecorationRoundRectangle = 1, xplm_WindowDecorationSelfDecorated = 2, xplm_WindowDecorationSelfDecoratedResizable = 3 };
typedef int XPLMWindowLayer; enum { xplm_WindowLayerFlightOverlay = 0, xplm_WindowLayerFloatingWindows = 1, xplm_WindowLayerModal = 2, xplm_WindowLayerGrowlNotifications = 3 };
typedef void (*XPLMDrawWindow_f)(XPLMWindowID, void*);
typedef void (*XPLMHandleKey_f)(XPLMWindowID, char, XPLMKeyFlags, char, void*, int);
typedef int (*XPLMHandleMouseClick_f)(XPLMWindowID, int, int, XPLMMouseStatus, void*);
typedef XPLMCursorStatus (*XPLMHandleCursor_f)(XPLMWindowID, int, int, void*);
typedef int (*XPLMHandleMouseWheel_f)(XPLMWindowID, int, int, int, int, void*);
typedef struct {
  int structSize; int left; int top; int right; int bottom; int visible;
  XPLMDrawWindow_f drawWindowFunc; XPLMHandleMouseClick_f handleMouseClickFunc; XPLMHandleKey_f handleKeyFunc;
  XPLMHandleCursor_f handleCursorFunc; XPLMHandleMouseWheel_f handleMouseWheelFunc; void* refcon;
  XPLMWindowDecoration decorateAsFloatingWindow; XPLMWindowLayer layer; XPLMHandleMouseClick_f handleRightClickFunc;
} XPLMCreateWindow_t;
extern "C" {
XPLMWindowID XPLMCreateWindowEx(XPLMCreateWindow_t*);
void XPLMDestroyWindow(XPLMWindowID);
void XPLMGetScreenSize(int*, int*);
void XPLMGetScreenBoundsGlobal(int*, int*, int*, int*);
void XPLMGetMouseLocationGlobal(int*, int*);
void XPLMGetWindowGeometry(XPLMWindowID, int*, int*, int*, int*);
int XPLMGetWindowIsVisible(XPLMWindowID);
void XPLMSetWindowIsVisible(XPLMWindowID, int);
void XPLMSetWindowTitle(XPLMWindowID, const char*);
void XPLMSetWindowResizingLimits(XPLMWindowID, int, int, int, int);
void XPLMTakeKeyboardFocus(XPLMWindowID);
int XPLMHasKeyboardFocus(XPLMWindowID);
void XPLMBringWindowToFront(XPLMWindowID);
}
