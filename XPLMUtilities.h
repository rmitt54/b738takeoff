#pragma once
#include "XPLMDefs.h"
typedef int XPLMCommandPhase; enum { xplm_CommandBegin = 0, xplm_CommandContinue = 1, xplm_CommandEnd = 2 };
typedef int (*XPLMCommandCallback_f)(XPLMCommandRef, XPLMCommandPhase, void*);
extern "C" {
void XPLMGetSystemPath(char*);
int XPLMEnableFeature(const char*, int);
float XPLMGetElapsedTime(void);
XPLMCommandRef XPLMCreateCommand(const char*, const char*);
void XPLMRegisterCommandHandler(XPLMCommandRef, XPLMCommandCallback_f, int, void*);
void XPLMUnregisterCommandHandler(XPLMCommandRef, XPLMCommandCallback_f, int, void*);
}
