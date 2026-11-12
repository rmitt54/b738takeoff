#pragma once
#include "XPLMDefs.h"
typedef int XPLMDataTypeID; enum { xplmType_Unknown = 0, xplmType_Int = 1, xplmType_Float = 2, xplmType_Double = 4, xplmType_FloatArray = 8, xplmType_IntArray = 16, xplmType_Data = 32 };
extern "C" { XPLMDataRef XPLMFindDataRef(const char*); XPLMDataTypeID XPLMGetDataRefTypes(XPLMDataRef);
int XPLMGetDatai(XPLMDataRef); float XPLMGetDataf(XPLMDataRef); double XPLMGetDatad(XPLMDataRef);
int XPLMGetDatavf(XPLMDataRef, float*, int, int); }
