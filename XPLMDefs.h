// Compile-check stand-ins that mirror the real X-Plane SDK declarations used by this plugin.
#pragma once
#define PLUGIN_API extern "C" __attribute__((visibility("default")))
typedef void* XPLMWindowID; typedef void* XPLMDataRef; typedef void* XPLMCommandRef; typedef int XPLMPluginID;
typedef void* XPLMMenuID;
