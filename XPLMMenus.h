#pragma once
#include "XPLMDefs.h"
extern "C" { XPLMMenuID XPLMFindPluginsMenu(void); int XPLMAppendMenuItemWithCommand(XPLMMenuID, const char*, XPLMCommandRef); void XPLMRemoveMenuItem(XPLMMenuID, int); }
